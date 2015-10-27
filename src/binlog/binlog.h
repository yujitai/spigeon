/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file binlog.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/23 03:07:23
 * @brief binlog组件
 *        BinLog维护公共状态信息，提供状态查询接口
 *        BinLogWriter负责所有的写入，包含binlog写入和完成标记
 *        BinLogReader负责读取binlog，内部有buffer
 **/

#ifndef __STORE_BINLOG_BINLOG_H__
#define __STORE_BINLOG_BINLOG_H__

#include <string>
#include <vector>
#include <deque>
#include <list>

#include "big_file.h"
#include "util/lock.h"
#include "inc/module.h"

namespace store {

// [-3000, -3999] binlog use for big_file and binlog
enum binlog_error_t {
    // [-3200, -3299] logic error
    BINLOG_FETCH_ERROR              = -3200, // get reader/writer failed
    BINLOG_NOT_FOUND                = -3201, // binlog id not available on disk,
                                             // too large or too small

    // [-3300, -3399] data error
    BINLOG_DATA_CORRUPTED           = -3300,
    BINLOG_DATA_LEN_LIMIT           = -3301,
    BINLOG_DATA_BAD_MAGIC_HEAD      = -3302,
    BINLOG_DATA_BAD_MAGIC_TAIL      = -3303,
    BINLOG_DATA_BAD_CHECKSUM        = -3304,
    BINLOG_DATA_BAD_CHECKPOINT      = -3305,
    BINLOG_DATA_LAST_CORRUPTED      = -3306,
    BINLOG_DATA_READ_LESS           = -3307,

};

// only used for short
#define RLOCK MutexHolder holder(_d_mutex)
#define WLOCK MutexHolder holder(_d_mutex)
//#define RLOCK ASSERT(_inited); RLockHolder holder(_rwlock);
//#define WLOCK ASSERT(_inited); WLockHolder holder(_rwlock);

class BinLogWriter;
class BinLogReader;
// binlog的数据格式为：head+data+tail
// tail保证数据按4字节对齐
#pragma pack(4)
struct binlog_head_t {
    uint32_t    magic_head;
    uint32_t    checksum;           /// 校验码，包含从binlog_id开始至data的结尾(不含tail)
    uint64_t    binlog_id;          /// 当前记录的binlog_id
    uint64_t    prev_binlog_id;     /// 上一条记录的binlog_id
    uint64_t    timestamp_us;       /// 主库写入时的时间，主从的时间戳需要保持一致
    uint32_t    total_len;          /// 总长度，head+data+padding对齐的部分
    uint32_t    data_len;           /// 数据区长度，不含head部分
    uint32_t    reserved[2];
    char        data[];
}; // 48 Bytes
#pragma pack()

const uint32_t BINLOG_MAGIC_HEAD        = 0x4d8e622f; /// 32bit
const char     BINLOG_MAGIC_TAIL        = 0xFF; /// 1字节
const uint32_t BINLOG_HEAD_LEN          = sizeof(binlog_head_t);
const uint32_t BINLOG_ALIGNMENT         = 4; /// 按4字节对齐，取决于magic_head的长度
const uint64_t BINLOG_OVER_HEAD_LEN     = BINLOG_HEAD_LEN + 2*BINLOG_ALIGNMENT;

// binlog_id序列，可以双向遍历，同时也方便了传参
struct binlog_id_seq_t {
    uint64_t    prev;
    uint64_t    cur;
    uint64_t    next;
};

struct BinLogOptions {
    /*
     * @param read_only         是否只读模式，默认false
     * @param dump_interval_ms  dump checkpoint时间的间隔
     * @param use_active_list   是否记录活跃列表，如果使用，必须调用writer的active_binlog_mark_done
     * @param single_file_limit 文件切分的大小，为0表示使用默认值
     * @param max_data_len      数据的最大长度，为0表示使用默认值
     */
    bool read_only;
    uint64_t dump_interval_ms;
    bool use_active_list;
    uint64_t single_file_limit;
    uint32_t max_data_len;

    BinLogOptions(bool in_read_only = false,
        uint64_t in_dump_interval_ms = 10*1000,
        bool in_use_active_list = true,
        uint64_t in_single_file_limit = 0,
        uint64_t in_max_data_len = 0)
    {
        read_only = in_read_only;
        dump_interval_ms = in_dump_interval_ms;
        use_active_list = in_use_active_list;
        single_file_limit = in_single_file_limit;
        max_data_len = in_max_data_len;
    }
};

/**
 * @brief BinLog维护公共状态信息，提供状态查询接口
 */
class BinLog : public Module{
private:
    static const uint32_t MAX_DATA_LEN          = 50 * 1024 *1024;  /// 数据最大长度
    static const uint32_t DEFAULT_MAX_DATA_LEN  = 20 * 1024 * 1024;

public:
    /**
     * @brief 构造函数
     */
    BinLog(const BinLogOptions &options);
    //BinLog(bool read_only = false, uint64_t dump_interval_ms = 10*1000,
        //bool use_active_list = true,
        //uint64_t single_file_limit = 0,
        //uint32_t max_data_len = DEFAULT_MAX_DATA_LEN);

    virtual ~BinLog();

    int init_conf() { return 0; }
    int load_conf(const char *filename) { (void)filename; return 0; }
    int validate_conf() { return 0; }

    uint64_t first_binlog_id()                  { RLOCK; return _first_binlog_id; }
    uint64_t last_binlog_id()                   { RLOCK; return _last_binlog_id; }
    uint64_t next_binlog_id()                   { RLOCK; return _next_binlog_id; }

    uint64_t max_mark_done_binlog_id();

    uint64_t checkpoint_binlog_id()             { return _big_file.checkpoint_binlog_id(); }

    /**
     * @brief 初始化
     * @param file_dir          数据的目录，不要和其他数据混用一个目录
     * @param file_name         数据的文件名称
     * @return 0为成功，小于0为错误
     */
    int open(const std::string &file_dir, const std::string &file_name = "binlog");

    /**
     * @brief 关闭
     * @note  如果不显示调用，析构是也会调用
     * @return 0为成功，小于0为发生错误
     */
    int close();

    /**
     * @brief 获取writer
     * @note  writer是Singleton，BinLogWriter是线程安全的
     * @return NULL为发生错误
     */
    BinLogWriter *writer();

    /**
     * @brief 获取reader
     * @param use_checksum      是否校验checksum
     * @note  BinLogReader非线程安全，每个线程需要使用自己的
     *        使用完毕后适时用put_back_reader
     * @return NULL为发生错误
     */
    BinLogReader *fetch_reader(bool use_checksum = true);

    /**
     * @brief 释放reader
     * @return 0为成功，小于0为发生错误
     */
    int put_back_reader(BinLogReader *reader);

    /**
     * @brief 设置强制truncate结尾
     * @note  正常情况下不使用，fix工具使用的接口
     */
    void set_force_truncate_dirty(bool v)       { _force_truncate_dirty = v; }

    int active_binlog_list_size();

    int active_binlog_list_max_size();

    static uint64_t current_time();

    void debug_info(uint64_t &single_file_limit, int &first_idx, int &cur_idx,
        uint64_t &first_pos, uint64_t &cur_pos);
private:
    class ActiveBinlogList;
    friend class BinLogWriter;
    friend class BinLogReader;

    uint32_t max_total_len()                    { return _max_total_len; }

    static uint32_t calc_checksum(const binlog_head_t *head, const char *data, int data_len);
    static uint32_t calc_align_padding(uint64_t l, uint32_t a=BINLOG_ALIGNMENT) { return (l%a==0 ? 0 : a-(l%a)); }
    static uint32_t calc_align_offset(uint64_t len, uint32_t align = BINLOG_ALIGNMENT) { return len % align; }

    void set_last_binlog_id(uint64_t v)         { WLOCK; _last_binlog_id = v; }
    void set_next_binlog_id(uint64_t v)         { WLOCK; _next_binlog_id = v; }

    int scan_data();
    int check_end();
    bool is_safe_truncate();
    int truncate_end();

    void active_binlog_add(binlog_id_seq_t seq);
    void active_binlog_mark_done(uint64_t id);
    bool _inited;
    Mutex _mutex;
    Mutex _d_mutex;
    RWLock _rwlock;

    bool _read_only;

    BigFile _big_file;
    BinLogWriter *_binlog_writer;

    uint32_t _max_total_len;

    uint64_t _first_binlog_id;
    uint64_t _last_binlog_id;
    uint64_t _next_binlog_id;

    bool _use_active_list;

    ActiveBinlogList *_active_list;

    uint64_t _dump_interval_ms;

    bool _force_truncate_dirty;
}; // end of BinLog

class BinLog::ActiveBinlogList {
public:
    ActiveBinlogList() : _list_size(0), _max_list_size(0)
    { memset(&_zero, 0, sizeof(_zero)); }
    ~ActiveBinlogList()                 {}

    //binlog_id_seq_t min()               { return (_list.empty() ? _zero : _list[0]); }
    binlog_id_seq_t min()               { return (_list.empty() ? _zero : *_list.begin()); }

    // BinLog可以保证按递增顺序add，因此_list自然就是排好序的
    void add(binlog_id_seq_t seq)       {
        _list.push_back(seq);
        _list_size++;
        if (_list_size > _max_list_size) _max_list_size = _list_size;
    }

    void remove(uint64_t binlog_id);

    void remove(binlog_id_seq_t seq)    { remove(seq.cur); }

    int size()                     { return _list_size; }
    int max_size() { return _max_list_size; }
private:
    binlog_id_seq_t _zero;
    std::list<binlog_id_seq_t> _list;
    int _list_size;
    int _max_list_size;
}; // end of ActiveBinlogList

/**
 * @brief BinLogWriter负责所有的写入，包含binlog写入和完成标记
 *        通过BinLog::writer()构造
 */
class BinLogWriter {
public:
    /**
     * @brief 追加写入，使用于主库
     * @param data              数据地址
     * @param data_len          数据长度
     * @param out_seq           返回binlog_id序号
     * @return 0为成功，小于0为发生错误
     */
    int append(const char *data, uint32_t data_len, binlog_id_seq_t *out_seq,
        binlog_head_t *out_head = NULL);

    /**
     * @brief 定位写入，指定写入到某个位置，适用于从库
     * @param binlog_id         binlog_id
     * @param timestamp_us      时间戳
     * @param data              数据地址
     * @param data_len          数据长度
     * @param out_seq           返回binlog_id序号
     * @return 0为成功，小于0为发生错误
     */
    int write(uint64_t binlog_id, uint64_t timestamp_us,
        const char *data, uint32_t data_len, binlog_id_seq_t *out_seq,
        binlog_head_t *out_head = NULL);

    /**
     * @brief 标记binlog为执行成功
     */
    void active_binlog_mark_done(uint64_t id)   { _binlog.active_binlog_mark_done(id); }

    /**
     * @brief 标记binlog为执行成功
     */
    void active_binlog_mark_done(binlog_id_seq_t seq) { _binlog.active_binlog_mark_done(seq.cur); }

    void dump_checkpoint(uint64_t id);

private:
    friend class BinLog;
    BinLogWriter(BinLog &binlog, BigFileWriter &big_file_writer,
        uint64_t dump_interval_ms);
    ~BinLogWriter();

    int open();

    int close();

    int do_write(uint64_t timestamp_us, const char *data, uint32_t data_len,
        binlog_id_seq_t *out_seq, binlog_head_t *out_head);

    int truncate(uint64_t pos);

    void do_dump_checkpoint(uint64_t id, uint64_t timestamp_us);
    void try_dump_checkpoint();

    bool _inited;
    Mutex _mutex;

    BinLog &_binlog;
    BigFileWriter &_big_file_writer;

    char _tail[BINLOG_ALIGNMENT*2];

    class Task;
    std::deque<Task*> _task_list;

    uint64_t _last_dump_timestamp_us;
    uint64_t _dump_interval_ms;
}; // end of BinLogWriter

class BinLogWriter::Task {
public:
    Task(Mutex &m) : cond_var(m) {}
    ~Task() {}

    uint64_t binlog_id;
    CondVar cond_var;
}; // end of BinLogWriter::Task

/**
 * @brief BinLogReader负责读取binlog，内部有buffer
 *        通过BinLog::fetch_reader()构造
 * @note  非线程安全
 */
class BinLogReader {
public:
    /*
     * @brief 以下是一组获取地址和长度的接口，一些接口有冗余，按需使用
     */
    const char *buf()           { return _buf; }
    uint32_t total_len()        { return head().total_len; }
    const binlog_head_t &head() { return *(binlog_head_t*)_buf; }
    const binlog_id_seq_t &seq(){ return _seq; }
    const char *data()          { return head().data; }
    uint32_t data_len()         { return head().data_len; }
    uint64_t prev_binlog_id()   { return _seq.prev; }
    uint64_t cur_binlog_id()    { return _seq.cur; }
    uint64_t next_binlog_id()   { return _seq.next; }

    /**
     * @brief 判断是否到binlog结尾
     */
    bool eof()                  { return (_binlog.next_binlog_id() == 0 || next_binlog_id() > _binlog.last_binlog_id()); }
    bool eof(uint64_t id)       { return (_binlog.next_binlog_id() == 0 || id > _binlog.last_binlog_id()); }

    /**
     * @brief 判断是否已被标记完成
     */
    bool next_sendable()        { 
        return (_binlog.next_binlog_id() > 0 
            && _binlog.max_mark_done_binlog_id() >= next_binlog_id());
    }
    bool sendable(uint64_t id)  { 
        return (_binlog.next_binlog_id() > 0 
            && _binlog.max_mark_done_binlog_id() >= id);
    }

    /**
     * @brief 读取接口
     * @return 0为正常，
     *         BINLOG_NOT_FOUND为binlog不存在
     *         以及其他错误
     */
    int read_next()             { return read(next_binlog_id()); }
    int read(uint64_t id);

private:
    friend class BinLog;
    BinLogReader(BinLog &binlog, BigFileReader &big_file_reader, bool use_checksum = true);
    ~BinLogReader();

    BigFileReader *big_file_reader() { return &_big_file_reader; }

    int open();
    int close();

    void reserve_buf(uint32_t len);

    bool _inited;
    Mutex _mutex;

    BinLog &_binlog;
    BigFileReader &_big_file_reader;

    bool _use_checksum;
    binlog_id_seq_t _seq;

    char *_buf;
    uint32_t _buf_len;
    binlog_head_t *_head;
}; // end of BinLogReader

class BinLogReaderHolder {
public:
    BinLogReaderHolder(BinLog* binlog, BinLogReader* reader)
            : _binlog(binlog), _reader(reader) { }
    ~BinLogReaderHolder() {
        // FIX is it exception free?
        if (_binlog != NULL) {
            _binlog->put_back_reader(_reader);
        }
    }
    BinLogReader &operator*() const { return *_reader; }
    BinLogReader *operator->() const { return _reader; }
    BinLogReader *get() const { return _reader; }

private:
    BinLog*       _binlog;
    BinLogReader* _reader;
};

#undef RLOCK
#undef WLOCK

} // end of namespace

#endif //__STORE_BINLOG_BINLOG_H__

