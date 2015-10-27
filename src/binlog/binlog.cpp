/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file binlog.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/23 03:07:55
 * @brief 
 **/

#include "binlog.h"
#include <algorithm>
#include "util/log.h"
#include "util/crc32c.h"
#include "util/zmalloc.h"

namespace store {

/***************************************************************************
 * BinLog
 **************************************************************************/

//BinLog::BinLog(bool read_only, uint64_t dump_interval_ms,
               //bool use_active_list, 
               //uint64_t single_file_limit, uint32_t max_data_len)
BinLog::BinLog(const BinLogOptions &options)
    : _inited(false),
      _read_only(options.read_only),
      _big_file(options.read_only, options.single_file_limit),
      _binlog_writer(NULL),
      _first_binlog_id(0),
      _last_binlog_id(0),
      _next_binlog_id(0),
      _use_active_list(options.use_active_list),
      _active_list(NULL),
      _dump_interval_ms(options.dump_interval_ms),
      _force_truncate_dirty(false)
{
    uint32_t max_data_len = options.max_data_len;
    if (options.max_data_len == 0) {
        max_data_len = DEFAULT_MAX_DATA_LEN;
    }
    if (max_data_len > MAX_DATA_LEN) {
        log_warning("invalid max_data_len(%u), max:%u, will use default(%u)",
            options.max_data_len, MAX_DATA_LEN,  DEFAULT_MAX_DATA_LEN);
        _max_total_len = DEFAULT_MAX_DATA_LEN + BINLOG_OVER_HEAD_LEN;
    } else {
        _max_total_len = max_data_len + BINLOG_OVER_HEAD_LEN;
    }
    if (_use_active_list) {
        _active_list = new (std::nothrow) ActiveBinlogList();
        ASSERT(_active_list);
    }
}

BinLog::~BinLog() {
    if (_inited) {
        close();
    }
}

#include <sys/time.h>
uint64_t BinLog::current_time()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
    //return EventLoop::current_time();
}

uint32_t BinLog::calc_checksum(const binlog_head_t *head, const char *data, int data_len) {
    ASSERT(head);
    ASSERT(data);
    ASSERT(data_len >= 0);
    const int calc_offset = (char*)&(head->binlog_id) - (char*)head; //TOTEST
    uint32_t checksum = crc32c::value((char*)head + calc_offset, BINLOG_HEAD_LEN - calc_offset);
    checksum = crc32c::extend(checksum, data, data_len);
    checksum = crc32c::mask(checksum);
    return checksum;
}

int BinLog::open(const std::string &file_dir, const std::string &file_name) {
    if (_inited) {
        log_fatal("BinLog is already inited");
        return BINLOG_INNER_INITED;
    }

    // 打开BigFile
    int ret = _big_file.open(file_dir, file_name);
    if (ret != 0) {
        log_fatal("big_file open failed. ret:%d", ret);
        return ret;
    }

    // 获取最小和最大binlog_id，以及截断可能的脏数据
    if (_big_file.cur_pos() == 0) {
        _first_binlog_id = _last_binlog_id = _next_binlog_id = 0;
    } else {
        ret = scan_data();
        if (ret != 0) {
            log_fatal("scan binlog to get first and last binlog_id failed. ret:%d", ret);
            return ret;
        }
        ret = check_end();
        if (ret != 0) {
            log_fatal("check the end of binlog failed. ret:%d", ret);
            return ret;
        }
    }

    log_notice("binlog open finished. read_only:%s "
        "first_binlog_id:0x%lX, last_binlog_id:0x%lX, next_binlog_id:0x%lX",
        (_read_only ? "true" : "false"), _first_binlog_id, _last_binlog_id, _next_binlog_id);
    _inited = true;
    return 0;
}

int BinLog::scan_data() {
    BinLogReader *reader = fetch_reader();
    if (!reader) {
        log_fatal("fetch_reader failed");
        return BINLOG_FETCH_ERROR;
    }

    int ret = 0;

    // 检查第一条binlog
    uint64_t binlog_id = _big_file.first_pos();
    ret = reader->read(binlog_id);
    if (ret != 0) {
        log_fatal("scan first binlog(0x%lX) failed. ret:%d", binlog_id, ret);
        put_back_reader(reader);
        return ret;
    }
    _first_binlog_id = binlog_id;

    // 从checkpoint开始检查到最后一条binlog
    bool found = false;
    binlog_id = _big_file.checkpoint_binlog_id();
    while (binlog_id < _big_file.cur_pos()) {
        ret = reader->read(binlog_id);
        if (ret != 0) {
            log_trace("detect scan: binlog(0x%lX) not valid. ret:%d", binlog_id, ret);
            break;
        }
        found = true;
        _last_binlog_id = binlog_id;
        _next_binlog_id = reader->next_binlog_id();
        binlog_id = _next_binlog_id;
    }
    if (!found) {
        log_fatal("bad checkpoint. can't scan from checkpoint(0x%lX)", _big_file.checkpoint_binlog_id());
        put_back_reader(reader);
        return BINLOG_DATA_BAD_CHECKPOINT;
    }

    ASSERT(_last_binlog_id >= _first_binlog_id);
    ASSERT(_next_binlog_id > 0);
    log_notice("scan_data first_binlog_id:0x%lX, last_binlog_id:0x%lX, next_binlog_id:0x%lX",
        _first_binlog_id, _last_binlog_id, _next_binlog_id);

    put_back_reader(reader);
    return 0;
}

int BinLog::check_end() {
    if (_big_file.cur_pos() == _next_binlog_id) {
        return 0;
    }
    if (_big_file.cur_pos() < _next_binlog_id) {
        log_fatal("binlog end not match. next_pos:0x%lX, next_binlog_id:0x%lX",
            _big_file.cur_pos(), _next_binlog_id);
        return BINLOG_DATA_CORRUPTED;
    }

    int ret = 0;

    if (_read_only) {
        // 只读模式不理会末尾
        log_fatal("binlog file may have dirty data at the end. "
            "continue in reading only mode. pos:0x%lX", _next_binlog_id);
        return 0;
    }

    if (is_safe_truncate()) {
        // 自动截断末尾的最后一条脏数据
        log_notice("automatically truncate last dirty data. pos:0x%lX to 0x%lX",
            _next_binlog_id, _big_file.cur_pos());
        ret = truncate_end();
        if (ret != 0) {
            log_fatal("truncate_end failed. ret:%d", ret);
            return ret;
        }
        log_notice("truncat data succeed at 0x%lX", _next_binlog_id);
        return 0;
    } else if (_force_truncate_dirty) {
        // 截断末尾的脏数据
        log_warning("forcely truncate dirty data from 0x%lX to 0x%lX",
            _next_binlog_id, _big_file.cur_pos());
        ret = truncate_end();
        if (ret != 0) {
            log_fatal("truncate_end failed. ret:%d", ret);
            return ret;
        }
        log_warning("truncat data succeed at 0x%lX", _next_binlog_id);
        return 0;
    } else {
        // 报错退出
        log_fatal("binlog file may have dirty data at the end. pos:0x%lX",
            _next_binlog_id);
        return BINLOG_DATA_LAST_CORRUPTED;
    }
}

bool BinLog::is_safe_truncate() {
    BinLogReader *reader = fetch_reader();
    if (!reader) {
        log_fatal("fetch_reader failed");
        return false;
    }

    uint32_t idx1       = _big_file.parse_idx(_next_binlog_id);
    //uint64_t offset1    = _big_file.parse_offset(_next_binlog_id); 
    uint32_t idx2       = _big_file.parse_idx(_big_file.cur_pos());
    uint64_t offset2    = _big_file.parse_offset(_big_file.cur_pos());
    // 严格判断，检查是否只有最后一条脏数据
    // 仅当cur_pos和_next_binlog_id在同一文件
    if (idx1 == idx2 || (idx1+1 == idx2 && offset2 == 0)) {
        // 并且读取错误为数据不足
        int ret = reader->read(_next_binlog_id);
        if (ret == BINLOG_NOT_FOUND 
            || ret == BINLOG_FILE_EOF 
            || ret == BINLOG_DATA_READ_LESS) 
        {
            put_back_reader(reader);
            return true;
        }
    }
    put_back_reader(reader);
    return false;
}

int BinLog::truncate_end() {
    BinLogWriter *writer = this->writer();
    if (!writer) {
        log_fatal("get writer failed");
        return BINLOG_FETCH_ERROR;
    }
    int ret = writer->truncate(_next_binlog_id);
    if (ret != 0) {
        log_fatal("truncate to pos(0x%lX) failed. ret:%d",
            _next_binlog_id, ret);
        return ret;
    }
    return 0;
}

int BinLog::close() {
    if (!_inited) {
        return BINLOG_INNER_NOT_INITED;
    }
    if (_active_list) {
        delete _active_list;
        _active_list = NULL;
    }
    if (_binlog_writer) {
        _binlog_writer->close();
        delete _binlog_writer;
        _binlog_writer = NULL;
    }
    _big_file.close();
    _inited = false;
    return 0;
}

BinLogWriter *BinLog::writer() {
    if (_read_only) {
        log_warning("read_only");
        return NULL;
    }

    if (_binlog_writer) {
        return _binlog_writer;
    }
    {
        MutexHolder holder(_mutex);
        if (_binlog_writer) {
            return _binlog_writer;
        }

        BigFileWriter *big_file_writer = _big_file.writer();
        if (!big_file_writer) {
            log_fatal("BigFileReader get writer failed");
            return NULL;
        }

        BinLogWriter *writer = new (std::nothrow) BinLogWriter(*this, *big_file_writer, _dump_interval_ms);
        ASSERT(writer);
        int ret = writer->open();
        if (ret != 0) {
            log_fatal("BinLogWriter open failed. ret:%d", ret);
            delete writer;
            return NULL;
        }

        _binlog_writer = writer;
        return _binlog_writer;
    }
}

BinLogReader *BinLog::fetch_reader(bool use_checksum) {
    BigFileReader *big_file_reader = _big_file.fetch_reader();
    if (!big_file_reader) {
        log_warning("BigFile fetch_reader failed");
        return NULL;
    }

    BinLogReader *reader = new (std::nothrow) BinLogReader(*this, *big_file_reader, use_checksum);
    ASSERT(reader);
    int ret = reader->open();
    if (ret != 0) {
        log_warning("BinLogReader open failed. ret:%d", ret);
        _big_file.put_back_reader(big_file_reader);
        delete reader;
        return NULL;
    }

    return reader;
}

int BinLog::put_back_reader(BinLogReader *reader) {
    ASSERT(reader);
    _big_file.put_back_reader(reader->big_file_reader());
    reader->close();
    delete reader;
    return 0;
}

void BinLog::active_binlog_add(binlog_id_seq_t seq) {
    if (!_inited) {
        log_fatal("BinLog is not inited");
        return;
    }
    if (_use_active_list) {
        //MutexHolder holder(_d_mutex);
        WLockHolder holder(_rwlock);
        _active_list->add(seq);
        log_debug("active_binlog_add binlog_id:0x%lX, size:%u", seq.cur, _active_list->size());
    }
}

void BinLog::active_binlog_mark_done(uint64_t id) {
    if (!_inited) {
        log_fatal("BinLog is not inited");
        return;
    }
    if (_use_active_list) {
        //MutexHolder holder(_d_mutex);
        WLockHolder holder(_rwlock);
        _active_list->remove(id);
        log_debug("active_binlog_mark_done binlog_id:0x%lX", id);
    }
}

int BinLog::active_binlog_list_size() {
    return _active_list->size();
}

int BinLog::active_binlog_list_max_size() {
    return _active_list->max_size();
}

uint64_t BinLog::max_mark_done_binlog_id() {
    if (!_inited) {
        log_fatal("BinLog is not inited");
        return 0;
    }
    
    //MutexHolder holder(_d_mutex);
    RLockHolder holder(_rwlock);
    if (_use_active_list) { 
        binlog_id_seq_t ret = _active_list->min();
        return (ret.cur == 0 ? _last_binlog_id : ret.prev);
    } else {
        return _last_binlog_id;
    }
}

void BinLog::debug_info(uint64_t &single_file_limit, int &first_idx, int &cur_idx,
    uint64_t &first_pos, uint64_t &cur_pos)
{
    single_file_limit   = _big_file.single_file_limit();
    first_idx           = _big_file.first_idx();
    cur_idx             = _big_file.cur_idx();
    first_pos           = _big_file.first_pos();
    cur_pos             = _big_file.cur_pos();
}

/***************************************************************************
 * ActiveBinlogList
 **************************************************************************/

void BinLog::ActiveBinlogList::remove(uint64_t binlog_id) {
    if (_list.empty()) {
        log_warning("binlog list is empty when removing 0x%lX", binlog_id);
        return;
    }
    bool found = false;
    std::list<binlog_id_seq_t>::iterator end = _list.end();
    for (std::list<binlog_id_seq_t>::iterator i = _list.begin();
        i != end; 
        ++i) 
    {
        if (i->cur == binlog_id) {
            //i->cur = 0; // 标记为0，等待下次被顶到front时再删除
            _list.erase(i);
            _list_size--;
            found = true;
            break;
        } else if (i->cur > binlog_id) {
            break;
        }
    }
    if (!found) {
        log_warning("binlog_id(0x%lX) not found when removing it.", binlog_id);
    }
}

/***************************************************************************
 * BinLogWriter
 **************************************************************************/

BinLogWriter::BinLogWriter(BinLog &binlog, BigFileWriter &big_file_writer,
    uint64_t dump_interval_ms)
    : _inited(false),
      _binlog(binlog),
      _big_file_writer(big_file_writer),
      _last_dump_timestamp_us(0),
      _dump_interval_ms(dump_interval_ms)
{
    memset(&_tail, BINLOG_MAGIC_TAIL, sizeof(_tail));
}

BinLogWriter::~BinLogWriter() {
    if (_inited) {
        close();
    }
}

int BinLogWriter::open() {
    if (_inited) {
        log_fatal("BinLogWriter is already inited");
        return BINLOG_INNER_INITED;
    }
    _inited = true;
    return 0;
}

int BinLogWriter::close() {
    if (!_inited) {
        return BINLOG_INNER_NOT_INITED;
    }
    _inited = false;
    return 0;
}

int BinLogWriter::do_write(uint64_t timestamp_us, const char *data, uint32_t data_len, 
    binlog_id_seq_t *out_seq, binlog_head_t *out_head)
{
    int ret = 0;

    // 必须确保和已有的数据能够对接上
    const uint64_t expect_binlog_id = _binlog.next_binlog_id();
    if (_big_file_writer.big_file().cur_pos() != expect_binlog_id) {
        log_fatal("writing pos is not match. pos:0x%lX, next_binlog_id:0x%lX",
            _big_file_writer.big_file().cur_pos(), expect_binlog_id);
        return BINLOG_DATA_CORRUPTED;
    }
    const uint64_t prev_binlog_id = _binlog.last_binlog_id();
    ASSERT(BinLog::calc_align_offset(expect_binlog_id) == 0);

    // 将数据按4字节对齐，使得下一条连续追加的magic_head在4字节
    // 末尾多写一个BINLOG_ALIGNMENT，作为结尾的标示，确保数据不以0结尾
    const uint32_t padding = BinLog::calc_align_padding(BINLOG_HEAD_LEN + data_len) + BINLOG_ALIGNMENT;
    const uint32_t total_len = BINLOG_HEAD_LEN + data_len + padding;
    log_debug("start writing binlog. expect_binlog_id:0x%lX, total_len:%u, "
        "head:%u, data_len:%u, padding:%u",
        expect_binlog_id, total_len, BINLOG_HEAD_LEN, data_len, padding);
    if (total_len > _binlog.max_total_len()) {
        log_warning("total_len is beyond the limit. total_len:%u, limit:%u",
            total_len, _binlog.max_total_len());
        return BINLOG_DATA_LEN_LIMIT;
    }

    // 组装头部
    binlog_head_t head;
    memset(&head, 0, BINLOG_HEAD_LEN);
    head.magic_head        = BINLOG_MAGIC_HEAD;
    head.binlog_id         = expect_binlog_id;
    head.prev_binlog_id    = _binlog.last_binlog_id();
    head.timestamp_us      = timestamp_us;
    head.total_len         = total_len;
    head.data_len          = data_len;
    head.checksum          = BinLog::calc_checksum(&head, data, data_len); // 计算checksum

    struct iovec vec[3];
    vec[0].iov_base = &head;
    vec[0].iov_len  = BINLOG_HEAD_LEN;
    vec[1].iov_base = (void*)data;
    vec[1].iov_len  = data_len;
    vec[2].iov_base = _tail;
    vec[2].iov_len  = padding;

    uint64_t actual_binlog_id;
    uint32_t writen_len;
    ret = _big_file_writer.appendv(vec, 3, &actual_binlog_id, &writen_len);
    if (ret != 0) {
        log_fatal("BigFileWriter append failed. ret:%d", ret);
        return ret;
    }
    ASSERT(writen_len == total_len);
    ASSERT(actual_binlog_id == expect_binlog_id);

    out_seq->prev = prev_binlog_id;
    out_seq->cur = actual_binlog_id;
    out_seq->next = _big_file_writer.big_file().incr_pos(actual_binlog_id, total_len);
    if (out_head) {
        *out_head = head;
    }

    _binlog.set_last_binlog_id(out_seq->cur);
    _binlog.set_next_binlog_id(out_seq->next);

    log_trace("BinLogWriter append/write finished. cur:0x%lX, prev:0x%lX, next:0x%lX",
        out_seq->cur, out_seq->prev, out_seq->next);

    // 添加到active_binlog列表
    _binlog.active_binlog_add(*out_seq);

    try_dump_checkpoint();

    return 0;
}

int BinLogWriter::append(const char *data, uint32_t data_len,
    binlog_id_seq_t *out_seq, binlog_head_t *out_head)
{
    ASSERT(data);

    int ret = 0;
    if (!_inited) {
        log_fatal("BinLogWriter is not inited");
        return BINLOG_INNER_NOT_INITED;
    }

    MutexHolder holder(_mutex);
    // NEXT 写合并，评估下是否有提升

    ret = do_write(BinLog::current_time(), data, data_len, out_seq, out_head);
    if (ret != 0) {
        log_fatal("BinLogWriter append failed. data_len:%u", data_len);
        return ret;
    }

    return 0;
}

int BinLogWriter::write(uint64_t binlog_id, uint64_t timestamp_us, 
    const char *data, uint32_t data_len, binlog_id_seq_t *out_seq,
    binlog_head_t *out_head)
{
    ASSERT(data);

    int ret = 0;
    if (!_inited) {
        log_fatal("BinLogWriter is not inited");
        return BINLOG_INNER_NOT_INITED;
    }

    MutexHolder holder(_mutex);

    if (binlog_id < _binlog.next_binlog_id()) {
        log_fatal("should not happend. binlog_id(0x%lX) is already writen. next:0x%lX", binlog_id, _binlog.next_binlog_id());
        return BINLOG_DATA_CORRUPTED;
    }
    // 如果自己不是下一个binlog，就等待
    if (_binlog.next_binlog_id() != binlog_id) {
        Task task(_mutex);
        task.binlog_id = binlog_id;
        _task_list.push_back(&task);

        while (_binlog.next_binlog_id() != binlog_id) {
            log_debug("write task waiting. binlog_id:0x%lX. list_size:%lu",
                binlog_id, _task_list.size());
            task.cond_var.wait(); // will unlock _mutex and wait
        }

        // 轮到了，同时也拿到了_mutex
        // 把自己从list中删除
        std::deque<Task*>::iterator iter = std::find(_task_list.begin(), _task_list.end(), &task);
        if (iter == _task_list.end()) {
            log_warning("task(%p) not found in task_list", &task);
        } else {
            _task_list.erase(iter);
        }
    }

    log_debug("write task resumed. binlog_id:0x%lX", binlog_id);
    ret = do_write(timestamp_us, data, data_len, out_seq, out_head);
    if (ret != 0) {
        log_fatal("BinLogWriter write failed. binlog_id:0x%lX, data_len:%u", binlog_id, data_len);
        return ret;
    }

    // 唤醒下一个
    const uint64_t next_binlog_id = out_seq->next;
    log_debug("will signal next binlog_id:0x%lX", next_binlog_id);
    for (std::deque<Task*>::iterator iter = _task_list.begin();
        iter != _task_list.end();
        ++iter)
    {
        if ((*iter)->binlog_id == next_binlog_id) {
            (*iter)->cond_var.signal();
            break;
        }
    }

    return 0;
}

void BinLogWriter::do_dump_checkpoint(uint64_t id, uint64_t timestamp_us) {
    int ret = _big_file_writer.big_file().dump_checkpoint(id);
    if (ret != 0) {
        log_fatal("dump checkpoint(0x%lX) failed.", id);
    }
    log_trace("BinLog dump checkpoint(0x%lX) finished. last_timestamp_us:0x%lX",
        id, _last_dump_timestamp_us);
    _last_dump_timestamp_us = timestamp_us;
}

void BinLogWriter::dump_checkpoint(uint64_t id) {
    uint64_t timestamp_us = BinLog::current_time();
    return do_dump_checkpoint(id, timestamp_us);
}

void BinLogWriter::try_dump_checkpoint() {
    uint64_t timestamp_us = BinLog::current_time();
    if (timestamp_us <= _last_dump_timestamp_us + _dump_interval_ms * 1000) {
        return;
    }
    return dump_checkpoint(_binlog.max_mark_done_binlog_id());
}

int BinLogWriter::truncate(uint64_t pos) {
    if (!_inited) {
        log_fatal("BinLogWriter is not inited");
        return BINLOG_INNER_NOT_INITED;
    }

    return _big_file_writer.truncate(pos);
}

/***************************************************************************
 * BinLogReader
 **************************************************************************/

BinLogReader::BinLogReader(BinLog &binlog, BigFileReader &big_file_reader, bool use_checksum)
    : _inited(false),
      _binlog(binlog),
      _big_file_reader(big_file_reader),
      _use_checksum(use_checksum),
      _buf(NULL),
      _buf_len(0),
      _head(NULL)
{
    memset(&_seq, 0, sizeof(_seq));
}

BinLogReader::~BinLogReader() {
    if (_inited) {
        close();
    }
}

int BinLogReader::open() {
    if (_inited) {
        log_fatal("BinLogReader is already inited");
        return BINLOG_INNER_INITED;
    }
    _inited = true;
    return 0;
}

int BinLogReader::close() {
    if (!_inited) {
        return 0;
    }
    if (_buf) {
        zfree(_buf);
        _buf = NULL;
        _buf_len = 0;
        _head = NULL;
    }
    _inited = false;
    return 0;
}

void BinLogReader::reserve_buf(uint32_t len) {
    ASSERT(len <= _binlog.max_total_len());
    if (len <= _buf_len) {
        return;
    }
    _buf = (char*)zrealloc(_buf, sizeof(char*) * len);
    _buf_len = len;
    _head = (binlog_head_t*)_buf;
    return;
}

int BinLogReader::read(uint64_t binlog_id) {
    if (!_inited) {
        log_fatal("BinLogReader is not inited");
        return BINLOG_INNER_NOT_INITED;
    }

    uint32_t read_len;

    // 读取head
    reserve_buf(BINLOG_HEAD_LEN);
    int ret = _big_file_reader.read(binlog_id, (char*)_head, BINLOG_HEAD_LEN, &read_len);
    if (ret != 0) {
        log_warning("read head failed. ret:%d, binlog_id:0x%lX, pos:0x%lX",
            ret, binlog_id, binlog_id);
        if (ret == BINLOG_FILE_EOF || ret == BINLOG_FILE_RECYCLED) {
            return BINLOG_NOT_FOUND;
        } else {
            return ret;
        }
    }
    if (read_len < BINLOG_HEAD_LEN) {
        log_warning("read head less than expected. "
            "ret:%d, binlog_id:0x%lX, pos:0x%lX, data_len:%u, read_len:%u",
            ret, binlog_id, binlog_id, BINLOG_HEAD_LEN, read_len);
        return BINLOG_DATA_READ_LESS;
    }

    // check
    if (_head->magic_head != BINLOG_MAGIC_HEAD) {
        log_warning("magic_head in head is not match. binlog_id:0x%lX, expected:0x%X, actual:%u",
            binlog_id, BINLOG_MAGIC_HEAD, _head->magic_head);
        return BINLOG_DATA_BAD_MAGIC_HEAD;
    }
    if (_head->binlog_id != binlog_id) {
        log_warning("binlog_id in head is not match. expected:0x%lX, actual:0x%lX",
            binlog_id, _head->binlog_id);
        return BINLOG_DATA_CORRUPTED;
    }
    if (_head->total_len > _binlog.max_total_len()) {
        log_warning("total_len is beyond the max_total_len. binlog_id:0x%lX, total_len:%u, limit:%u",
            binlog_id, _head->total_len, _binlog.max_total_len());
        return BINLOG_DATA_LEN_LIMIT;
    }

    // 读取数据
    reserve_buf(_head->total_len);
    const uint32_t body_len = _head->total_len - BINLOG_HEAD_LEN;
    ret = _big_file_reader.read(binlog_id + BINLOG_HEAD_LEN, _head->data, body_len, &read_len);
    if (ret != 0) {
        log_warning("read body failed. ret:%d, binlog_id:0x%lX, pos:0x%lX",
            ret, binlog_id, binlog_id + BINLOG_HEAD_LEN);
        if (ret == BINLOG_FILE_EOF || ret == BINLOG_FILE_RECYCLED) {
            return BINLOG_NOT_FOUND;
        } else {
            return ret;
        }
    }
    if (read_len < body_len) {
        log_warning("read body less than expected. "
            "ret:%d, binlog_id:0x%lX, pos:0x%lX, body_len:%u, data_len:%u, read_len:%u",
            ret, binlog_id, binlog_id + BINLOG_HEAD_LEN, body_len, _head->data_len, read_len);
        return BINLOG_DATA_READ_LESS;
    }

    // tail检查，这里只检查一个字符
    const char tail = _head->data[_head->data_len];
    if (tail != BINLOG_MAGIC_TAIL) {
        log_warning("tail is not match. binlog_id:0x%lX, expected:%u, actual:%u",
            binlog_id, BINLOG_MAGIC_TAIL, tail);
        return BINLOG_DATA_BAD_MAGIC_TAIL;
    }
    // checksum检查
    if (_use_checksum) {
        uint32_t checksum = BinLog::calc_checksum(_head, _head->data, _head->data_len);
        if (checksum != _head->checksum) {
            log_warning("checksum is not match. binlog_id:0x%lX, in_head:%u, actual:%u",
                binlog_id, _head->checksum, checksum);
            return BINLOG_DATA_BAD_CHECKSUM;
        }
    }

    _seq.prev   = _head->prev_binlog_id;
    _seq.cur    = _head->binlog_id;
    _seq.next   = _big_file_reader.big_file().incr_pos(_head->binlog_id, _head->total_len);

    log_trace("BinLogReader read finished. cur:0x%lX, prev:0x%lX, next:0x%lX, "
        "total_len:%u, data_len:%u",
        _seq.cur, _seq.prev, _seq.next, _head->total_len, _head->data_len);
    return 0;
}

} // end of namespace

