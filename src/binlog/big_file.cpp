/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file big_file.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/20 15:05:18
 * @brief 
 **/

#include "big_file.h"

#include <sys/mman.h>
#include <sys/file.h>
#include <algorithm>

#include "util/log.h"
#include "util/file.h"

namespace store {

#define TRUNCATE_ZERO_ENDING true // 默认模式，暂不暴露为参数

/***************************************************************************
 * BigFile
 **************************************************************************/

BigFile::BigFile(bool read_only, uint64_t single_file_limit,
    uint32_t custom1, uint32_t custom2, uint32_t custom3,
    bool use_checkpoint)
    : _inited(false),
      _read_only(read_only),
      _first_pos(0),
      _cur_pos(0),
      _writer(NULL),
      _reader_counter(0),
      _use_checkpoint(use_checkpoint),
      _checkpoint_binlog_id(0),
      _checkpoint_file(NULL),
      _meta_fd(-1)
{
    if (single_file_limit == 0) {
        single_file_limit = DEFAULT_SINGLE_FILE_LIMIT;
    }
    if (single_file_limit > MAX_SINGLE_FILE_LIMIT
        || single_file_limit < MIN_SINGLE_FILE_LIMIT)
    {
        log_fatal("invalid single_file_limit(0x%lX). min:0x%lX, max:0x%lX, "
            "will use default:0x%lX",
            single_file_limit, MIN_SINGLE_FILE_LIMIT,
            MAX_SINGLE_FILE_LIMIT, DEFAULT_SINGLE_FILE_LIMIT);
        single_file_limit = DEFAULT_SINGLE_FILE_LIMIT;
    }

    memset(&_meta, 0, sizeof(_meta));
    _meta.version           = BigFile::VERSION1;
    _meta.magic_num         = BigFile::MAGIC_NUMBER;
    _meta.single_file_limit = single_file_limit;
    _meta.custom1           = custom1;
    _meta.custom2           = custom2;
    _meta.custom3           = custom3;
}

BigFile::~BigFile() {
    if (_inited) {
        close();
    }
}

std::string BigFile::make_data_file_path(int idx) {
    ASSERT(idx >= 0);
    char buf[100];
    snprintf(buf, sizeof(buf), ".%d", idx);
    return _file_path + buf;
}

int BigFile::open(const std::string &file_path) {
    const std::string file_dir = FileUtil::dirname(file_path);
    const std::string file_name = FileUtil::basename(file_path);
    return open(file_dir, file_name);
}

int BigFile::open(const std::string &file_dir, const std::string &file_name) {
    int ret = 0;
    if (_inited) {
        log_fatal("BigFile is already inited");
        return BINLOG_INNER_INITED;
    }

    _file_dir = file_dir;
    _file_name = file_name;
    _file_path = (file_dir[file_dir.size() - 1] == '/' 
                 ? file_dir + file_name
                 : file_dir + "/" + file_name);
    _meta_path = _file_path + ".meta";

    log_debug("open file:%s. mode:%s", _file_path.c_str(),
        (_read_only ? "read_only" : "read_write"));

    // 扫描数据文件
    int first_idx = -1;
    int cur_idx = -1;
    ret = scan_data(&first_idx, &cur_idx);
    if (ret != 0) {
        log_fatal("scan_data failed. path:%s, ret:%d", _file_dir.c_str(), ret);
        return ret;
    }
    log_debug("BigFile scan_data: first_idx:%d, cur_idx:%d", first_idx, cur_idx);
    ASSERT(cur_idx >= first_idx);
    
    // 检查meta文件是否存在
    const bool meta_exists = (FileUtil::exists(_meta_path) == 0);
    if (!meta_exists) {
        if (cur_idx != -1) { // data file exists but meta file missing
            log_fatal("data file exists but meta file(%s) lost", _meta_path.c_str());
            return BINLOG_FILE_LOST;
        }
        if (_read_only) {
            log_fatal("is open for read_only, but meta file(%s) is lost",
                _meta_path.c_str());
            return BINLOG_FILE_LOST;
        }
    }

    // 扫描元信息
    if (!meta_exists) {
        ret = create_meta();
    } else {
        ret = read_meta();
    }
    if (ret != 0) {
        log_fatal("create_meta or read_meta failed. path:%s, ret:%d", _meta_path.c_str(), ret);
        return ret;
    }

    // 确定最后一个文件的大小
    uint64_t cur_file_size = 0;
    if (cur_idx > -1) {
        ret = FileUtil::file_size(make_data_file_path(cur_idx), &cur_file_size);
        if (ret != 0) {
            log_fatal("file_size for %s failed, ret:%d",
                make_data_file_path(cur_idx).c_str(), ret);
            return BINLOG_IO_ERROR;
        }
    }
    log_debug("BigFile cur_file_size:0x%lX", cur_file_size);

    if (cur_file_size >= _meta.single_file_limit) { // 如果已经到文件末尾，则递增到下一个文件
        log_debug("reach the end of file. move to next. idx:%d, size:0x%lX",
            cur_idx, cur_file_size);
        cur_idx += 1;
        cur_file_size = 0;
    }

    // 最后更新成员变量
    _first_pos = 0;
    _cur_pos = 0;
    if (cur_idx > -1) {
        _first_pos = make_pos(first_idx, 0);
        _cur_pos = make_pos(cur_idx, cur_file_size);
    }

    if (_use_checkpoint) {
        // 加载checkpoint
        ret = load_checkpoint();
        if (ret != 0) {
            log_fatal("load_checkpoint failed. ret:%d", ret);
            return ret;
        }
    }

    if (!_read_only) {
        // lock meta file，防止多个进程同时使用BigFileWriter
        ASSERT(_meta_fd >= 0);
        if (flock(_meta_fd, LOCK_EX | LOCK_NB) < 0) {
            log_fatal("flockdataa(%s) failed. maybe other is using the same file. err:%d(%s)",
                _meta_path.c_str(), errno, strerror(errno));
            return BINLOG_EXCLUSIVE;
        }
    }

    log_notice("BigFile open finished. first_id:%d, cur_idx:%d, first_pos:0x%lX, cur_pos:0x%lX "
        "single_file_limit:0x%lX",
        first_idx, cur_idx, _first_pos, _cur_pos, _meta.single_file_limit);
    _inited = true;
    return 0;
}

int BigFile::close() {
    if (!_inited) {
        return BINLOG_INNER_NOT_INITED;
    }

    if (_meta_fd >= 0) {
        flock(_meta_fd, LOCK_UN);
        ::close(_meta_fd);
        _meta_fd = -1;
    }
    if (_writer) {
        _writer->close();
        delete _writer;
        _writer = NULL;
    }
    close_checkpoint_file();

    _inited = false;
    return 0;
}

int BigFile::create_meta() {
    int ret = 0;
    ASSERT(!_read_only);

    // 默认meta已在构造函数中准备好
    ret = FileUtil::create_parent_dir(_meta_path);
    if (ret != 0) {
        log_warning("create_parent_dir for %s failed. ret:%d", _meta_path.c_str(), ret);
        return BINLOG_IO_ERROR;
    }

    _meta_fd = ::open(_meta_path.c_str(), O_CREAT | O_RDWR, 
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (_meta_fd < 0) {
        log_warning("create meta file(%s) failed. err:%d(%s)", _meta_path.c_str(), errno, strerror(errno));
        return BINLOG_IO_ERROR;
    }
    ret = pwrite(_meta_fd, &_meta, sizeof(_meta), 0);
    if (ret != sizeof(_meta)) {
        log_warning("meta file pwrite failed. ret:%d, err:%d(%s)", ret, errno, strerror(errno));
        return BINLOG_IO_ERROR;
    }

    return 0;
}

int BigFile::read_meta() {
    int ret = 0;
    _meta_fd = ::open(_meta_path.c_str(), O_RDONLY);
    if (_meta_fd < 0) {
        log_warning("open meta file(%s) failed. err:%d(%s)", _meta_path.c_str(), errno, strerror(errno));
        return BINLOG_IO_ERROR;
    }
    ret = pread(_meta_fd, &_meta, sizeof(_meta), 0);
    if (ret != sizeof(_meta)) {
        log_warning("meta file pread failed. ret:%d, err:%d(%s)", ret, errno, strerror(errno));
        return BINLOG_IO_ERROR;
    }
    return 0;
}

int BigFile::scan_data(int *out_first_idx, int *out_cur_idx) {
    int ret = 0;

    // 检查目录是否存在
    if (FileUtil::exists(_file_dir) != 0) {
        //log_warning("data dir(%s) not exists, continue with warning", _file_dir.c_str());
        *out_first_idx = -1;
        *out_cur_idx = -1;
        return 0;
    }

    // 获取文件列表
    std::vector<std::string> file_list;
    ret = FileUtil::scan_dir(_file_dir, _file_name, &file_list);
    if (ret != 0) {
        log_warning("scan_dir failed. ret:%d", ret);
        return BINLOG_IO_ERROR;
    }

    // 查找数据文件
    const int file_list_size = file_list.size();
    std::vector<int> idx_list;
    idx_list.reserve(file_list_size);
    for (int i = 0; i < file_list_size; ++i) {
        int tmp = 0;
        char *ctmp = NULL;
        tmp = strtol(file_list[i].c_str() + _file_name.length() + 1/*skip .*/, &ctmp, 10);
        if (ctmp != NULL && *ctmp == '\0') {
            idx_list.push_back(tmp);
        }
    }

    const int idx_list_size = idx_list.size();
    if (idx_list_size <= 0) { // 空目录
        log_warning("data dir(%s) is empty, continue with warning", _file_dir.c_str());
        *out_first_idx = -1;
        *out_cur_idx = -1;
        return 0;
    }

    // 获取最后一段连续的idx
    std::sort(idx_list.begin(), idx_list.end());
    *out_cur_idx = idx_list[idx_list_size - 1];
    *out_first_idx = *out_cur_idx;
    if (idx_list_size > 1) {
        for (int i = idx_list_size - 2; i >= 0; --i) {
            if (idx_list[i] == idx_list[i+1] - 1) {
                *out_first_idx = idx_list[i];
            } else {
                log_warning("data file missing, will ignore files before idx:%d", idx_list[i+1]);
                break; // 发现不连续
            }
        }
    }

    if (!_read_only) {
        // 检查数据文件结尾是否为空
        ret = check_end(out_first_idx, out_cur_idx);
        if (ret != 0) {
            log_fatal("check idx end failed. ret:%d", ret);
            return ret;
        }
    }

    return 0;
}

int BigFile::check_end(int *out_first_idx, int *out_cur_idx) {
    if (!TRUNCATE_ZERO_ENDING) {
        return 0;
    }
    int err = 0;
    MmapSeqWritableFile *single_file = NULL;

    while (true) {
        single_file = new (std::nothrow) MmapSeqWritableFile();
        ASSERT(single_file);

        const std::string path = make_data_file_path(*out_cur_idx);
        int ret = single_file->open(path, false/*truncate_whole_file*/);
        if (ret != 0) {
            log_fatal("open file(%s) failed. ret:%d", path.c_str(), ret);
            err = ret;
            goto out;
        }

        if (single_file->cur_pos() != 0 || *out_cur_idx == *out_first_idx) {
            break;
        }
        // 当前文件为全0，并且不是第一个文件，则：删除文件，倒退到上一个文件
        if (unlink(path.c_str()) != 0) {
            log_fatal("unlink file(%s) failed. err:%d(%s)", path.c_str(), errno, strerror(errno));
            err = BINLOG_IO_ERROR;
            goto out;
        }
        (*out_cur_idx)--;
    }

out:
    if (single_file) {
        single_file->close();
        delete single_file;
    }
    return err;
}

int BigFile::open_checkpoint_file() {
    if (_checkpoint_file) {
        return 0;
    }

    const std::string checkpoint_path = _file_path + ".checkpoint";
    bool exists = (FileUtil::exists(checkpoint_path.c_str()) == 0);

    // 如果文件已存在，使用w+将把已有文件截断，使用a+将只能在末尾追加，因此需要智能判断
    const char *mode = "r+";
    if (_read_only && !exists) {
        log_warning("checkpoint file(%s) lost", checkpoint_path.c_str());
        return BINLOG_FILE_LOST;
    } else if (_read_only && exists) {
        mode = "r";
    } else if (!_read_only && !exists) {
        mode = "w+";
    } else {
        mode = "r+";
    }
    _checkpoint_file = fopen(checkpoint_path.c_str(), mode); 
    if (!_checkpoint_file) {
        log_warning("fopen failed. file:%s, err:%d(%s)", checkpoint_path.c_str(), errno, strerror(errno));
        return BINLOG_IO_ERROR;
    }

    return 0;
}

void BigFile::close_checkpoint_file() {
    if (_checkpoint_file) {
        fclose(_checkpoint_file);
        _checkpoint_file = NULL;
    }
}

int BigFile::load_checkpoint() {
    if (!_use_checkpoint) {
        log_warning("invalid logic, use_checkpoint is off");
        return BINLOG_CHECKPOINT_OFF;
    }

    MutexHolder holder(_d_mutex);
    //WLockHolder holder(_rwlock);
    uint64_t binlog_id = 0;

    int ret = open_checkpoint_file();
    if (ret != 0) {
        log_warning("open_checkpoint_file failed. ret:%d", ret);
        return ret;
    }

    ret = fseek(_checkpoint_file, 0, SEEK_SET);
    if (ret != 0) {
        log_warning("fseek checkpoint file failed, err:%d(%s)", errno, strerror(errno));
        goto failed;
    }

    ret = fscanf(_checkpoint_file, "0x%lX", &binlog_id);
    if (ret != 1) {
        if (feof(_checkpoint_file)) {
            if (_cur_pos != 0) {
                // 数据非空，但是checkpoin为空
                log_fatal("empty or invalid checkpoint file when data exist.");
            }
            return 0;
        }
        log_warning("fscanf checkpoint file failed, ret:%d, err:%d(%s)", ret, errno, strerror(errno));
        goto failed;
    }
    _checkpoint_binlog_id = binlog_id;
    log_debug("load_checkpoint: 0x%lX", _checkpoint_binlog_id);
    return 0;
failed:
    close_checkpoint_file();
    return BINLOG_IO_ERROR;
}

int BigFile::dump_checkpoint(uint64_t binlog_id) {
    if (!_use_checkpoint) {
        log_warning("invalid logic, use_checkpoint is off");
        return BINLOG_CHECKPOINT_OFF;
    }
    if (_read_only) {
        log_warning("read_only");
        return BINLOG_READ_ONLY;
    }

    MutexHolder holder(_d_mutex);
    //WLockHolder holder(_rwlock);

    int ret = open_checkpoint_file();
    if (ret != 0) {
        log_warning("open_checkpoint_file failed. ret:%d", ret);
        return ret;
    }

    ret = fseek(_checkpoint_file, 0, SEEK_SET);
    if (ret != 0) {
        log_warning("fseek checkpoint file failed, err:%d(%s)", errno, strerror(errno));
        goto failed;
    }
    ret = fprintf(_checkpoint_file, "0x%lX\n", binlog_id);
    if (ret < 0) {
        log_warning("fprintf checkpoint file failed, ret:%d, err:%d(%s)", ret, errno, strerror(errno));
        goto failed;
    }
    _checkpoint_binlog_id = binlog_id;
    ret = fflush(_checkpoint_file);
    if (ret != 0) {
        log_warning("fflush checkpoint file failed, err:%d(%s)", errno, strerror(errno));
        goto failed;
    }

    log_debug("BigFile dump_checkpoint: 0x%lX", _checkpoint_binlog_id);
    return 0;
failed:
    close_checkpoint_file();
    return BINLOG_IO_ERROR;
}

int BigFile::sync_checkpoint() {
    if (_read_only) {
        log_warning("read_only");
        return BINLOG_READ_ONLY;
    }

    if (_use_checkpoint && _checkpoint_file) {
        int ret = fdatasync(fileno(_checkpoint_file));
        if (ret != 0) {
            log_warning("fdatasync checkpoint file failed, err:%d(%s)", errno, strerror(errno));
            return BINLOG_IO_ERROR;
        }
    }
    return 0;
}

int BigFile::sync() {
    if (_read_only) {
        log_warning("read_only");
        return BINLOG_READ_ONLY;
    }

    int err = 0;
    if (_writer) {
        if (_writer->sync() != 0) {
            err = -1;
        }
    }
    if (sync_checkpoint() != 0) {
        err = -1;
    }
    return err;
}

BigFileWriter *BigFile::writer() {
    if (_read_only) {
        log_warning("read_only");
        return NULL;
    }

    // Singleton，全局只能有一个writer
    if (_writer) {
        return _writer;
    }
    {
        MutexHolder holder(_mutex);
        if (_writer) {
            return _writer;
        }

        BigFileWriter *writer = new (std::nothrow) BigFileWriter(*this);
        ASSERT(writer);
        int ret = writer->open();
        if (ret != 0) {
            log_fatal("BigFileWriter open failed. ret:%d", ret);
            delete writer;
            return NULL;
        }

        _writer = writer;

        return _writer;
    }
}

BigFileReader *BigFile::fetch_reader() {
    BigFileReader *reader = new (std::nothrow) BigFileReader(*this);
    ASSERT(reader);
    int ret = reader->open();
    if (ret != 0) {
        log_warning("BigFileReader open failed. ret:%d", ret);
        delete reader;
        return NULL;
    }
    incr_reader_counter(1);
    return reader;
}

int BigFile::put_back_reader(BigFileReader *reader) {
    ASSERT(reader);
    reader->close();
    delete reader;
    incr_reader_counter(-1);
    return 0;
}

/***************************************************************************
 * BigFileWriter
 **************************************************************************/

BigFileWriter::BigFileWriter(BigFile &big_file)
    : _inited(false),
      _big_file(big_file),
      _single_file(NULL),
      _truncate_next(false)
{
}

BigFileWriter::~BigFileWriter() {
    if (_inited) {
        close();
    }
}

int BigFileWriter::open() {
    if (_inited) {
        log_fatal("BigFileWriter is already inited");
        return BINLOG_INNER_INITED;
    }

    int ret = open_file();
    if (ret != 0) {
        log_fatal("BigFileWriter open_file failed. ret:%d", ret);
        return ret;
    }

    _inited = true;
    return 0;
}

int BigFileWriter::close() {
    if (!_inited) {
        return BINLOG_INNER_NOT_INITED;
    }

    close_file();

    _inited = false;
    return 0;
}

int BigFileWriter::open_file() {
    if (_single_file) {
        close_file();
    }

    _single_file = new (std::nothrow) MmapSeqWritableFile();
    ASSERT(_single_file);

    const std::string path = _big_file.make_data_file_path(_big_file.cur_idx());
    int ret = _single_file->open(path, _truncate_next);
    _truncate_next = false;
    if (ret != 0) {
        log_fatal("open file(%s) failed. ret:%d", path.c_str(), ret);
        return ret;
    }

    return 0;
}

int BigFileWriter::close_file() {
    if (_single_file) {
        //sync before close? 这是目前sync的一个盲区，暂不处理
        _single_file->close();
        delete _single_file;
        _single_file = NULL;
    }
    return 0;
}

int BigFileWriter::do_appendv(const struct iovec *vector, int count,
    uint64_t old_single_offset, uint32_t *out_writen_len)
{
    int ret = 0;
    *out_writen_len = 0;

    for (int i = 0; i < count; ++i) {
        const char *data = (char*)vector[i].iov_base;
        const uint32_t data_len = vector[i].iov_len;
        ASSERT(data);

        ret = _single_file->append(data, data_len);
        if (ret != 0) {
            log_fatal("append file failed. ret:%d, data_len:%u", ret, data_len);
            return ret;
        }
        ASSERT(old_single_offset + data_len == _single_file->cur_pos());
        *out_writen_len += data_len;
        old_single_offset += data_len;
    }

    return 0;
}

int BigFileWriter::appendv(const struct iovec *vector, int count,
    uint64_t *out_pos, uint32_t *out_writen_len)
{
    // 暂不加锁，由上层来保证安全
    // 如果写入失败，会执行rollback回滚到原先位置
    // 如果跨越single_file_limit，也会全部写到本文件后再切换到下一个文件

    ASSERT(vector);
    ASSERT(count > 0);
    int ret = 0;

    if (!_inited) {
        log_fatal("BigFileWriter is not inited");
        return BINLOG_INNER_NOT_INITED;
    }

    if (!_single_file) {
        ret = open_file();
        if (ret != 0) {
            log_fatal("BigFileWriter open_file failed. ret:%d", ret);
            return ret;
        }
    }

    const uint64_t old_pos = _big_file.cur_pos();
    const uint64_t old_offset = _big_file.parse_offset(old_pos);
    ASSERT(old_offset < _big_file.single_file_limit());
    ASSERT(old_offset == _single_file->cur_pos());

    ret = do_appendv(vector, count, old_offset, out_writen_len);
    if (ret != 0) {
        log_fatal("need rollback to pos:0x%lX, offset:0x%lX", old_pos, old_offset);
        int ret2 = _single_file->rollback(old_offset);
        if (ret2 != 0) {
            log_fatal("rollback failed. ret:%d, offset:0x%lX", ret2, old_offset);
            return ret2;
        }
        return ret;
    }
    log_trace("appendv finished. pos:0x%lX, data_len:%u", old_pos, *out_writen_len);

    // 先更新single_file再更改全局状态
    // close_file时会将未写区域truncate，truncate后才对reader可见
    if (_single_file->cur_pos() >= _big_file.single_file_limit()) {
        close_file();
        _truncate_next = true;
    }
    // 更新全局状态
    _big_file.incr_cur_pos(*out_writen_len);
    *out_pos = old_pos;

    return 0;
}

int BigFileWriter::append(const char *data, uint32_t data_len,
    uint64_t *out_pos, uint32_t *out_writen_len)
{
    ASSERT(data);

    struct iovec vec[1];
    vec[0].iov_base = (void*)data;
    vec[0].iov_len  = data_len;

    return appendv(vec, 1, out_pos, out_writen_len);
}

int BigFileWriter::append(const char *head, uint32_t head_len,
    const char *data, uint32_t data_len, uint64_t *out_pos, uint32_t *out_writen_len)
{
    ASSERT(data);

    struct iovec vec[2];
    vec[0].iov_base = (void*)head;
    vec[0].iov_len  = head_len;
    vec[1].iov_base = (void*)data;
    vec[1].iov_len  = data_len;

    return appendv(vec, 2, out_pos, out_writen_len);
}

int BigFileWriter::truncate(uint64_t target_pos) {
    if (!_inited) {
        log_fatal("BigFileWriter is not inited");
        return BINLOG_INNER_NOT_INITED;
    }
    if (_big_file.reader_counter() > 0) {
        log_fatal("cannot truncate file when other's reading");
        return BINLOG_CANNOT_TRUNCATE;
    }

    ASSERT(target_pos < _big_file.cur_pos());
    _big_file.assert_pos(target_pos);

    int ret = 0;
    const int target_idx = _big_file.parse_idx(target_pos);
    const uint64_t target_offset = _big_file.parse_offset(target_pos);
    std::string path;

    path = _big_file.make_data_file_path(target_idx);
    uint64_t file_size = 0;
    ret = FileUtil::file_size(path, &file_size);
    if (ret < 0) {
        log_fatal("get file_size failed. path:%s, ret:%d", path.c_str(), ret);
        return false;
    }
    if (target_offset > file_size) {
        log_fatal("target_offset(0x%lX) is larger than file_size(0x%lX)",
            target_offset, file_size);
        return BINLOG_CANNOT_TRUNCATE;
    }

    // 关闭当前_single_file
    close_file();

    // 逆序判断，删除多余文件
    for (int cur_idx = _big_file.cur_idx(); cur_idx > target_idx; --cur_idx) {
        path = _big_file.make_data_file_path(cur_idx);
        if (FileUtil::exists(path) != 0) {
            continue;
        }
        if (::truncate(path.c_str(), 0) != 0) { // 先truncate文件避免被其他人误读
            log_fatal("truncate file(%s) failed. err:%d(%s)", path.c_str(), errno, strerror(errno));
            goto fatal_failed;
        }
        if (unlink(path.c_str()) != 0) {
            log_fatal("unlink file(%s) failed. err:%d(%s)", path.c_str(), errno, strerror(errno));
            goto fatal_failed;
        }
    }

    // 截断到target_offset
    path = _big_file.make_data_file_path(target_idx);
    if (::truncate(path.c_str(), target_offset) != 0) {
        log_fatal("truncate file(%s) failed. err:%d(%s)", path.c_str(), errno, strerror(errno));
        goto fatal_failed;
    }

    // 最后再更新
    _big_file.set_cur_pos(target_pos);

    // 再重新在target_offset位置打开
    _truncate_next = false;
    ret = open_file();
    if (ret != 0) {
        log_fatal("BigFileWriter open_file failed. ret:%d", ret);
        goto fatal_failed;
    }

    log_trace("BigFile truncate finished. target_pos:0x%lX, actual_pos:0x%lX",
        target_pos, _big_file.cur_pos());
    return 0;
fatal_failed:
    _inited = false; // 严重错误，不再提供服务
    return BINLOG_IO_ERROR;
}

int BigFileWriter::sync() { 
    if (_single_file) { 
        return _single_file->sync(); 
    } 
    return 0; 
}

/***************************************************************************
 * BigFileReader
 **************************************************************************/

BigFileReader::BigFileReader(BigFile &big_file)
    : _inited(false),
      _big_file(big_file),
      _single_file(NULL),
      _idx(-1)
{
}

BigFileReader::~BigFileReader() {
    if (_inited) {
        close();
    }
}

int BigFileReader::open() {
    if (_inited) {
        log_fatal("BigFileReader is already inited");
        return BINLOG_INNER_INITED;
    }

    _inited = true;

    return 0;
}

int BigFileReader::close() {
    if (!_inited) {
        return BINLOG_INNER_NOT_INITED;
    }

    _idx = -1;
    close_file();

    _inited = false;
    return 0;
}

int BigFileReader::open_file(int idx) {
    ASSERT(idx >= 0);

    if (_single_file) {
        close_file();
    }

    _single_file = new (std::nothrow) MmapReadableFile();
    ASSERT(_single_file);

    const std::string path = _big_file.make_data_file_path(idx);
    int ret = _single_file->open(path);
    if (ret != 0) {
        log_fatal("open file(%s) failed. ret:%d", path.c_str(), ret);
        return ret;
    }
    _idx = idx;

    return 0;
}

int BigFileReader::close_file() {
    if (_single_file) {
        _single_file->close();
        delete _single_file;
        _single_file = NULL;
    }
    return 0;
}

int BigFileReader::read(uint64_t pos, char *buf, uint32_t expect_len,
    uint32_t *out_read_len)
{
    // strict: default mode, make sure read_len is equal to expect_len
    const bool strict = true;
    ASSERT(buf);

    int ret = 0;
    if (!_inited) {
        log_fatal("BigFileReader is not inited");
        return BINLOG_INNER_NOT_INITED;
    }

    // 检查
    if (pos >= _big_file.cur_pos()) {
        log_warning("read pos is beyond the cur_pos. pos:0x%lX, cur_pos:0x%lX",
            pos, _big_file.cur_pos());
        return BINLOG_FILE_EOF;
    }
    if (strict && (pos + expect_len > _big_file.cur_pos())) {
        log_warning("read pos+len is beyond the cur_pos. pos:0x%lX, expect_len:%u, cur_pos:0x%lX",
            pos, expect_len, _big_file.cur_pos());
        return BINLOG_FILE_EOF;
    }
    if (pos < _big_file.first_pos()) {
        log_warning("read pos is beyond the first_pos. pos:0x%lX, first_pos:0x%lX",
            pos, _big_file.first_pos());
        return BINLOG_FILE_RECYCLED;
    }

    // 文件idx定位
    const int idx = _big_file.parse_idx(pos);
    if (idx != _idx) {
        ret = open_file(idx);
        if (ret != 0) {
            log_warning("BigFileReader open_file failed. ret:%d", ret);
            return ret;
        }
    }
    ASSERT(_single_file);

    // 读取数据
    const uint64_t offset = _big_file.parse_offset(pos);  
    ret = _single_file->read(offset, buf, expect_len, out_read_len);
    if (ret != 0) {
        log_warning("read failed. ret:%d, pos:0x%lX, offset:0x%lX, expect_len:%u, "
            "out_read_len:%u",
            ret, pos, offset, expect_len, *out_read_len);
        return ret;
    }

    if (*out_read_len < expect_len) {
        if (strict) {
            log_warning("read len(%d) is not expected(%u)", *out_read_len, expect_len);
            return BINLOG_IO_READ_LESS;
        }
    }
    log_debug("BigFileReader read finished. pos:0x%lX, expect_len:%u, len:%u",
        pos, expect_len, *out_read_len);

    return 0;
}

/***************************************************************************
 * MmapFile
 **************************************************************************/

MmapFile::MmapFile(bool read_only)
    : _page_size(getpagesize()),
      _map_size(calc_map_size()),
      _fd(-1),
      _read_only(read_only),
      _base(NULL),
      _limit(NULL),
      _base_offset(0)
{
    ASSERT((_page_size & (_page_size - 1)) == 0);
    ASSERT((_map_size & (_map_size - 1)) == 0);
    ASSERT((_map_size % _page_size) == 0);
}

MmapFile::~MmapFile() {
}

void MmapFile::debug_info(uint32_t &page_size, uint32_t &map_size) {
    page_size = getpagesize();
    map_size = calc_map_size();
}

/***************************************************************************
 * MmapSeqWritableFile
 **************************************************************************/

MmapSeqWritableFile::MmapSeqWritableFile()
    : MmapFile(false/*read_only*/),
      _dst(NULL),
      _last_sync(NULL),
      _pending_sync(false)
{
}

MmapSeqWritableFile::~MmapSeqWritableFile() {
    if (_fd >= 0) {
        close();
    }
}

int MmapSeqWritableFile::do_map(uint64_t base_offset, uint64_t inner_offset) {
    ASSERT(_base == NULL);
    _base_offset = base_offset;

    // 确保有足够空间
    if (ftruncate(_fd, _base_offset + _map_size) < 0) {
        log_fatal("ftruncate for new region failed. offset:0x%lX",
            _base_offset + _map_size);
        return BINLOG_IO_ERROR;
    }

    void* ptr = mmap(NULL, _map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
        _fd, _base_offset);
    if (ptr == MAP_FAILED) {
        log_fatal("mmap failed. err:%d(%s)", errno, strerror(errno));
        return BINLOG_MMAP_ERROR;
    }
    _base = (char*)ptr;
    _limit = _base + _map_size;
    _dst = _base + inner_offset;
    _last_sync = _dst;
    return 0;
}

int MmapSeqWritableFile::map(uint64_t pos) {
    // 计算位置
    uint64_t base_offset = truncate_to_boundary(pos, _map_size);
    return do_map(base_offset, pos - base_offset);
}

int MmapSeqWritableFile::unmap() {
    if (!_base) {
        return 0;
    }

    if (_base && _last_sync && _limit && _last_sync < _limit) {
        // Defer syncing this data until next sync() call, if any
        _pending_sync = true;
    }

    int err = 0;
    if (munmap(_base, _map_size) != 0) {
        log_fatal("munmap failed. err:%d(%s)", errno, strerror(errno));
        err = BINLOG_MMAP_ERROR;
    }
    _base = NULL;
    _limit = NULL;

    _dst = NULL;
    _last_sync = NULL;

    return err;
}

int MmapSeqWritableFile::switch_file() {
    int ret = unmap();
    if (ret != 0) {
        log_fatal("unmap failed. ret:%d", ret);
        return ret;
    }
    ret = map(_base_offset + _map_size);
    if (ret != 0) {
        log_fatal("map failed. ret:%d", ret);
        return ret;
    }
    return 0;
}

int MmapSeqWritableFile::open(const std::string &file_path, bool truncate_whole_file) {
    _file_path = file_path;

    _fd = ::open(file_path.c_str(),
        O_CREAT | O_RDWR | (truncate_whole_file ? O_TRUNC : 0),
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (_fd < 0) {
        log_fatal("open failed. path:%s, truncate_whole_file:%s, err:%d(%s)",
            file_path.c_str(), (truncate_whole_file?"true":"false"), errno, strerror(errno));
        return BINLOG_IO_ERROR;
    }

    int ret = init_map();
    if (ret != 0) {
        log_fatal("init_map failed. ret:%d", ret);
        return ret;
    }

    return 0;
}

int MmapSeqWritableFile::init_map() {
    // 获取文件大小
    uint64_t file_size = 0;
    int ret = FileUtil::file_size(_fd, &file_size);
    if (ret != 0) {
        log_fatal("get file_size failed. path:%s, ret:%d", _file_path.c_str(), ret);
        return BINLOG_IO_ERROR;
    }

    ret = map(file_size);
    if (ret != 0) {
        log_fatal("map failed. ret:%d", ret);
        return ret;
    }

    if (!TRUNCATE_ZERO_ENDING || cur_pos() == 0) {
        return 0;
    }

    bool truncated = false;
    while (true) {
        // 寻找non-zero
        while (_dst > _base && *(_dst-1) == '\0') {
            truncated = true;
            _dst--;
            _last_sync = _dst;
        }
        if (_dst != _base || _base_offset == 0) {
            if (truncated) {
                log_trace("file truncate zero to 0x%lX. path:%s",
                    cur_pos(), _file_path.c_str());
            }
            break;
        }

        log_debug("found all zero at 0x%lX, move to previous map. path:%s",
            cur_pos(), _file_path.c_str());
        // 如果全为zero，且不是文件起始，则向前map
        ret = unmap();
        if (ret != 0) {
            log_fatal("unmap failed. ret:%d", ret);
            return ret;
        }
        // 这里特殊一些，需要map到上一个区域的末尾，调用do_map
        ret = do_map(_base_offset - _map_size, _map_size);
        if (ret != 0) {
            log_fatal("do_map failed. ret:%d", ret);
            return ret;
        }
    }

    return 0;
}

int MmapSeqWritableFile::close() {
    if (_fd < 0) {
        return 0;
    }

    bool need_truncate = false;
    const uint64_t offset = _base_offset + (_dst - _base);
    if (_base && _dst && offset < _base_offset + _map_size) {
        // Trim the extra space at the end of the file
        need_truncate = true;
    }

    int err = 0;
    int ret = unmap();
    if (ret != 0) {
        log_fatal("unmap failed. ret:%d", ret);
        err = ret;
    }
    if (need_truncate) {
        int ret = ftruncate(_fd, offset);
        if (ret != 0) {
            log_fatal("ftruncate extra space failed. offset:0x%lX", offset);
            err = BINLOG_IO_ERROR;
        }
    }
    if (::close(_fd) < 0) {
        log_fatal("close file failed. err:%d(%s)", errno, strerror(errno));
        if (err != 0) {
            err = BINLOG_IO_ERROR;
        }
    }

    _fd = -1;
    _base = NULL;
    _limit = NULL;
    _base_offset = 0;

    _dst = NULL;
    _last_sync = NULL;
    _pending_sync = false;

    return err;
}

int MmapSeqWritableFile::append(const char *data, uint32_t len) {
    // 如果data为NULL，就将数据区写0
    const char* src = data;
    uint64_t left = len;
    while (left > 0) {
        ASSERT(_base <= _dst);
        ASSERT(_dst <= _limit);
        const uint64_t available = _limit - _dst;
        if (available == 0) {
            switch_file();
        }

        const uint64_t n = (left <= available) ? left : available;
        if (src) {
            memcpy(_dst, src, n);
        } else {
            memset(_dst, 0, n); // 安全起见，memset一下
        }
        _dst += n;
        if (src) {
            src += n;
        }
        left -= n;
    }
    return 0;
}

int MmapSeqWritableFile::rollback(uint64_t pos) {
    ASSERT(pos < cur_pos());
    int ret = unmap();
    if (ret != 0) {
        log_fatal("unmap failed. ret:%d", ret);
    }
    ret = map(pos); // map的同时会truncate掉多余的数据
    if (ret != 0) {
        log_fatal("map failed. ret:%d", ret);
        return ret;
    }

    // 再把pos后的数据清0
    memset(_dst, 0, _limit - _dst);
    return 0;
}

int MmapSeqWritableFile::sync() {
    int err;

    if (_pending_sync) {
        // Some unmapped data was not synced
        _pending_sync = false;
        if (fdatasync(_fd) < 0) {
            log_fatal("fdatasync failed. err:%d(%s)", errno, strerror(errno));
            err = BINLOG_IO_ERROR;
        }
    }

    if (_base && _last_sync && _dst && _dst > _last_sync) {
        // Find the beginnings of the pages that contain the first and last
        // bytes to be synced.
        uint64_t p1 = truncate_to_boundary(_last_sync - _base, _page_size);
        uint64_t p2 = truncate_to_boundary(_dst - _base - 1, _page_size);
        if (msync(_base + p1, p2 - p1 + _page_size, MS_SYNC) < 0) {
            log_fatal("msync failed. err:%d(%s)", errno, strerror(errno));
            err = BINLOG_MMAP_ERROR;
        }
        _last_sync = _dst;
    }

    return err;
}

/***************************************************************************
 * MmapReadableFile
 **************************************************************************/

MmapReadableFile::MmapReadableFile()
    : MmapFile(true/*read_only*/),
      _cur_map_size(0)
{
}

MmapReadableFile::~MmapReadableFile() {
    if (_fd >= 0) {
        close();
    }
}

int MmapReadableFile::map(uint64_t pos) {
    // 计算位置
    uint64_t base_offset = truncate_to_boundary(pos, _map_size);

    int ret = 0;
    if (_base) {
        // 如果当前pos的位置已经被map进来，则说明可以满足读取
        if (_base_offset == base_offset && pos < _base_offset + _cur_map_size) {
            return 0;
        }
        ret = unmap();
        if (ret != 0) {
            log_warning("unmap failed. ret:%d", ret);
            return ret;
        }
    }
    return do_map(base_offset);
}

int MmapReadableFile::do_map(uint64_t base_offset) {
    ASSERT(_base == NULL);
    _base_offset = base_offset;

    uint64_t file_size = 0;
    int ret = FileUtil::file_size(_fd, &file_size);
    if (ret != 0) {
        log_warning("get file_size failed");
        return BINLOG_IO_ERROR;
    }

    if (base_offset >= file_size) {
        log_warning("cannot map file at 0x%lX, file_size:0x%lX", base_offset, file_size);
        return BINLOG_CANNOT_MAP;
    }

    _cur_map_size = (_map_size < (file_size-base_offset) ? _map_size : (file_size-base_offset));

    void* ptr = mmap(NULL, _cur_map_size, PROT_READ, MAP_SHARED,
        _fd, _base_offset);
    if (ptr == MAP_FAILED) {
        log_warning("mmap failed. err:%d(%s)", errno, strerror(errno));
        return BINLOG_MMAP_ERROR;
    }
    _base = (char*)ptr;
    _limit = _base + _cur_map_size;
    return 0;
}

int MmapReadableFile::unmap() {
    if (!_base) {
        return 0;
    }

    int err = 0;
    if (munmap(_base, _cur_map_size) != 0) {
        log_warning("munmap failed. err:%d(%s)", errno, strerror(errno));
        err = BINLOG_MMAP_ERROR;
    }
    _base = NULL;
    _limit = NULL;

    return err;
}

int MmapReadableFile::open(const std::string &file_path) {
    _file_path = file_path;

    _fd = ::open(file_path.c_str(), O_RDONLY);
    if (_fd < 0) {
        log_warning("open failed. path:%s, err:%d(%s)",
            file_path.c_str(), errno, strerror(errno));
        return BINLOG_IO_ERROR;
    }

    return 0;
}

int MmapReadableFile::close() {
    if (_fd < 0) {
        return 0;
    }

    int err = 0;
    int ret = 0;
    if (_base) {
        ret = unmap();
        if (ret != 0) {
            log_warning("unmap failed. ret:%d", ret);
        }
    }
    if (::close(_fd) < 0) {
        log_warning("close file failed. err:%d(%s)", errno, strerror(errno));
        if (err != 0) {
            err = BINLOG_IO_ERROR;
        }
    }

    _fd = -1;
    _base = NULL;
    _limit = NULL;
    _base_offset = 0;
    
    return err;
}

int MmapReadableFile::read(uint64_t pos, char *buf, uint32_t len,
    uint32_t *out_read_len)
{
    ASSERT(out_read_len);

    int ret = 0;

    uint64_t cur_pos    = pos;
    char *cur_buf       = buf;
    uint64_t left       = len;
    *out_read_len       = 0;
    
    while (left > 0) {
        ret = map(cur_pos);
        if (ret != 0) {
            log_warning("map failed. ret:%d", ret);
            return ret;
        }

        ASSERT(cur_pos >= _base_offset);
        const uint64_t cur_offset = cur_pos - _base_offset;
        const uint64_t available = _cur_map_size - cur_offset;
        const uint64_t reading = (left < available ? left : available);
        ASSERT(reading > 0);
        memcpy(cur_buf, _base + cur_offset, reading);

        cur_pos += reading;
        cur_buf += reading;
        left -= reading;
        *out_read_len += reading;
    }

    return 0;
}

} // namespace store

