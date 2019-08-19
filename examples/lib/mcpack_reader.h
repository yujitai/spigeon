
#ifndef __STORE_MCPACK_READER_H_
#define __STORE_MCPACK_READER_H_

#include <stdint.h>
#include "mc_pack.h"

namespace store {

enum mcpack_reader_error_t {
    MCPACK_READER_OPEN_ERROR    = -1001,
    MCPACK_READER_GET_STR_ERROR = -1002,
    MCPACK_READER_GET_RAW_ERROR = -1003,
};

class McPackReader {
    static int const TMP_BUF_MAX_LEN = 10 * 1024 * 1024;
    char _tmpbuf[TMP_BUF_MAX_LEN];  // 临时缓冲区，用于建立句柄、索引

    mc_pack_t*  _data;

public:
    McPackReader();

    ~McPackReader();

    int open(const char* buf, const int buf_len);

    void close();

    mc_pack_t* get_mc_pack() { return _data; }

    int get_raw(const char* key, const char* & value, unsigned int& value_len);

    int get_str(const char* key, const char* & value);

    int get_int32(const char* key, int& value);

    int get_uint32(const char* key, uint32_t& value);

    int get_uint64(const char* key, uint64_t& value);

    int get_version() { return mc_pack_get_version(_data); }
};

}  // namespace store

#endif  // __STORE_MCPACK_READER_H_

