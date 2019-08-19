
#ifndef __STORE_MCPACK_WRITER_H_
#define __STORE_MCPACK_WRITER_H_

#include <stdint.h>
#include "mc_pack.h"

namespace store {

enum mcpack_writer_error_t {
    MCPACK_WRITER_OPEN_ERROR     = -2000,
    MCPACK_WRITER_PUT_DATA_ERROR = -2001,
    MCPACK_WRITER_NOT_OPEN       = -2002,
};

class McPackWriter {
    static const int TMP_BUF_MAX_LEN = 16 * 1024 * 1024;
    static const int MCPACK_VERSION_DEF = 2;
    char  _tmpbuf[TMP_BUF_MAX_LEN];  // 临时缓冲区，用于建立句柄、索引
    char* _buf;
    int   _buf_len;
    int   _version;
    mc_pack_t*  _data;

public:
    McPackWriter();

    ~McPackWriter();

    int open(char* buf, const int buf_len, int version = 2);

    void close();

    int reset_mc_pack();

    int put_raw(const char* key, const char* value, int value_len);

    int put_str(const char* key, const char* value);

    int put_int32(const char* key, int value);

    int put_uint32(const char* key, uint32_t value);

    int put_uint64(const char* key, uint64_t value);

    mc_pack_t* get_raw_mc_pack() { return _data; }

    int get_raw_mc_pack_len() { return mc_pack_get_size(_data); }
};

}  // namespace store

#endif  // __STORE_MCPACK_WRITER_H_

