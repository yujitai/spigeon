/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file big_file.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/11/20 14:52:45
 * @brief 
 **/

#ifndef __STORE_BINLOG_BIGFILE_H__
#define __STORE_BINLOG_BIGFILE_H__

#include <string>
#include <vector>
#include <sys/uio.h> // for iovec
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include "util/lock.h"

namespace store {

// [-3000, -3999] binlog use for big_file and binlog
enum big_file_error_t {
    // [-3000, -3099] logic error
    BINLOG_EXCLUSIVE                = -3000, // other process is writing the same binlog
    BINLOG_INNER_INITED             = -3001,
    BINLOG_INNER_NOT_INITED         = -3002,
    BINLOG_CHECKPOINT_OFF           = -3003,
    BINLOG_READ_ONLY                = -3004,
    BINLOG_CANNOT_TRUNCATE          = -3005,
    BINLOG_CANNOT_MAP               = -3006,

    // [-3100, -3199] io/system error
    BINLOG_IO_ERROR                 = -3100,
    BINLOG_IO_READ_LESS             = -3101,
    BINLOG_FILE_EOF                 = -3102,
    BINLOG_FILE_RECYCLED            = -3103,
    BINLOG_FILE_LOST                = -3104,
    BINLOG_MMAP_ERROR               = -3105, // mmap/munmap failed

};

#define RLOCK MutexHolder holder(_d_mutex)
#define WLOCK MutexHolder holder(_d_mutex)
//#define RLOCK ASSERT(_inited); RLockHolder holder(_rwlock);
//#define WLOCK ASSERT(_inited); WLockHolder holder(_rwlock);

class BigFileWriter;
class BigFileReader;
class MmapSeqWritableFile;
class MmapReadableFile;

class BigFile {
private:
    static const uint32_t VERSION1                  = 0x1;
    static const uint32_t MAGIC_NUMBER              = 0x4d8e622f;
    static const uint64_t DEFAULT_SINGLE_FILE_LIMIT = (1 << 30); // 1G
    static const uint64_t MIN_SINGLE_FILE_LIMIT     = (1 << 20); // 1M
    static const uint64_t MAX_SINGLE_FILE_LIMIT     = (1 << 31); // 2G
#pragma pack(4)
    struct meta_t {
        uint32_t    magic_num;
        uint32_t    version:8;
        uint32_t    reserved:24;
        uint64_t    single_file_limit;
        uint32_t    custom1;
        uint32_t    custom2;
        uint32_t    custom3;
    };
#pragma pack()

public:
    BigFile(bool read_only = false, uint64_t single_file_limit = DEFAULT_SINGLE_FILE_LIMIT,
            uint32_t custom1 = 0, uint32_t custom2 = 0, uint32_t custom3 = 0,
            bool use_checkpoint = true);
    ~BigFile();

    uint32_t parse_idx(uint64_t pos)    { return (pos >> 32); }
    uint64_t parse_offset(uint64_t pos) { return ((pos << 32) >> 32); }
    void assert_pos(uint64_t pos) { 
        ASSERT(parse_offset(pos) < _meta.single_file_limit); 
    }
    uint64_t make_pos(int idx, uint64_t off) {
        ASSERT(idx >= 0 && off < (1UL<<32));
        if (off >= _meta.single_file_limit) {
            idx++;
            off = 0;
        }
        return ((uint64_t)idx<<32) + off;
    }
    uint64_t make_offset(uint64_t off) {
        ASSERT(off < (1UL<<32));
        return (off >= _meta.single_file_limit ? 0 : off);
    }
    uint64_t incr_pos(uint64_t pos, uint64_t incr) {
        if (parse_offset(pos) + incr >= _meta.single_file_limit) {
            return make_pos(parse_idx(pos)+1, 0);
        } else {
            return pos + incr;
        }
    }

    std::string make_data_file_path(int idx);
    uint64_t single_file_limit()        { return _meta.single_file_limit; }
    uint32_t meta_custom1()             { return _meta.custom1; }
    uint32_t meta_custom2()             { return _meta.custom2; }
    uint32_t meta_custom3()             { return _meta.custom3; }

    int first_idx()                     { RLOCK; return parse_idx(_first_pos); }
    int cur_idx()                       { RLOCK; return parse_idx(_cur_pos); }
    uint64_t first_pos()                { RLOCK; return _first_pos; }
    uint64_t cur_pos()                  { RLOCK; return _cur_pos; }
    uint64_t checkpoint_binlog_id()     { RLOCK; return _checkpoint_binlog_id; }
    int reader_counter()                { RLOCK; return _reader_counter; }

    void incr_cur_pos(uint64_t i)       { WLOCK; _cur_pos = incr_pos(_cur_pos, i); }
    void set_cur_pos(uint64_t p) {
        WLOCK; 
        _cur_pos = make_pos(parse_idx(p), parse_offset(p));
    }
    void incr_reader_counter(int offset) {
        WLOCK;
        _reader_counter += offset; 
        ASSERT(_reader_counter >= 0);
    }

    int open(const std::string &file_dir, const std::string &file_name);
    int open(const std::string &file_path);
    int close();

    int load_checkpoint();
    int dump_checkpoint(uint64_t binlog_id);
    int sync_checkpoint();

    int sync();

    BigFileWriter *writer();

    BigFileReader *fetch_reader();
    int put_back_reader(BigFileReader *reader);

private:
    int create_meta();
    int read_meta();
    int scan_data(int *out_first_idx, int *out_cur_idx);
    int check_end(int *out_first_idx, int *out_cur_idx);

    int open_checkpoint_file();
    void close_checkpoint_file();

    bool _inited;
    Mutex _mutex;
    Mutex _d_mutex;
    //RWLock _rwlock;

    bool _read_only;

    std::string _file_dir;
    std::string _file_name;
    std::string _file_path;
    std::string _meta_path;

    meta_t _meta;

    uint64_t _first_pos;
    uint64_t _cur_pos;

    BigFileWriter *_writer;
    int _reader_counter;

    bool _use_checkpoint;
    uint64_t _checkpoint_binlog_id;
    FILE *_checkpoint_file;

    int _meta_fd;
}; // end of BigFile

class BigFileWriter {
public:
    int open();
    int close();

    BigFile &big_file()             { return _big_file; }

    int append(const char *data, uint32_t data_len,
        uint64_t *out_pos, uint32_t *out_writen_len);

    int append(const char *head, uint32_t head_len,
        const char *data, uint32_t data_len,
        uint64_t *out_pos, uint32_t *out_writen_len);

    int appendv(const struct iovec *vector, int count,
        uint64_t *out_pos, uint32_t *out_writen_len);

    int sync();

    int truncate(uint64_t target_pos);

private:
    friend class BigFile;
    BigFileWriter(BigFile &big_file);
    ~BigFileWriter();

    int close_file();
    int open_file();

    int do_appendv(const struct iovec *vector, int count,
        uint64_t old_single_offset, uint32_t *out_writen_len);

    bool _inited;

    BigFile &_big_file;
    MmapSeqWritableFile *_single_file;

    int _meta_fd;

    bool _truncate_next;
}; // end of BigFileWriter

class BigFileReader {
public:
    int open();
    int close();

    BigFile &big_file()             { return _big_file; }

    int read(uint64_t pos, char *buf, uint32_t buf_len, uint32_t *out_read_len);

private:
    friend class BigFile;
    BigFileReader(BigFile &big_file);
    ~BigFileReader();

    int close_file();
    int open_file(int idx);

    bool _inited;

    BigFile &_big_file;
    MmapReadableFile *_single_file;

    int _idx;
}; // end of BigFileReader

class MmapFile {
public:

    static void debug_info(uint32_t &page_size, uint32_t &map_size);

protected:
    MmapFile(bool read_only); // only for sub-class
    virtual ~MmapFile();

    static const uint32_t MIN_MAP_SIZE              = (50 * 1024 * 1024);
    static uint32_t calc_map_size()     { return roundup(MIN_MAP_SIZE, getpagesize()); }

    //roundup x to a 2^ of y
    static uint64_t roundup(uint64_t x, uint64_t y) {
        uint64_t z = y;
        while (z < x) {
            z *= 2;
        }
        return z;
    }

    static uint64_t truncate_to_boundary(uint64_t s, uint64_t n) {
        s -= (s & (n - 1));
        assert((s % n) == 0);
        return s;
    }

    const uint32_t _page_size;
    const uint32_t _map_size;       // How much extra memory to map at a time

    std::string _file_path;
    int _fd;

    bool _read_only;

    char* _base;            // The mapped region
    char* _limit;           // Limit of the mapped region
    uint64_t _base_offset;  // Offset of _base in file

}; // end of MmapFile

// modified from leveldb::PosixMmapFile
// We preallocate up to an extra megabyte and use memcpy to append new
// data to the file.  This is safe since we either properly close the
// file before reading from it, or for log files, the reading code
// knows enough to skip zero suffixes.
class MmapSeqWritableFile : public MmapFile {
public:
    MmapSeqWritableFile();
    virtual ~MmapSeqWritableFile();

    uint64_t cur_pos()                  { return _base_offset + (_dst - _base); }

    int open(const std::string &file_path, bool truncate_whole_file);
    int close();

    int append(const char *data, uint32_t len);
    int sync();

    int rollback(uint64_t pos);

private:
    int do_map(uint64_t base_offset, uint64_t inner_offset);
    int map(uint64_t pos);
    int unmap();
    int switch_file();

    int init_map();

    // [----------------------------------]
    //          ^        ^            ^
    //          |        |            |
    //          _base    _dst         _limit = _base+_map_size
    //          _base_offset
    char* _dst;             // Where to write next  (in range [_base,_limit])
    char* _last_sync;       // Where have we synced up to
    bool _pending_sync;     // Have we done an munmap of unsynced data?
}; // end of MmapSeqWritableFile

class MmapReadableFile : public MmapFile {
public:
    MmapReadableFile();
    virtual ~MmapReadableFile();

    int open(const std::string &file_path);
    int close();

    int read(uint64_t pos, char *buf, uint32_t len, uint32_t *out_read_len);
private:
    int map(uint64_t pos);
    int do_map(uint64_t base_offset);
    int unmap();

    uint32_t _cur_map_size;
}; // end of MmapReadableFile

#undef RLOCK
#undef WLOCK

} // end of namespace

#endif //__STORE_BINLOG_BIGFILE_H__

