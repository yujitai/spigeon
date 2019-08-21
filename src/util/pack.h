#ifndef __STORE_PACK_H_
#define __STORE_PACK_H_

#include <stdint.h>

#include "mc_pack.h"

namespace zf {

class Slice;

// [-2700 -2799] pack
enum pack_error_t {
    PACK_OK         = 0,
    PACK_MODE_ERROR = 2700,
    PACK_OPEN_ERROR = 2701,
    PACK_PUT_ERROR  = 2702,
    PACK_GET_ERROR  = 2703,
};

class Pack {
public:
    Pack();
    ~Pack();

    enum OpMode {
        READ  = 1,
        WRITE = 2,
    };

    int open(const Slice& buf, enum OpMode mode);
    int finish();
    int close();

    int get_raw(const char *key, Slice* value);
    int get_str(const char *key, Slice* value);
    int get_int32(const char *key, int32_t* value);
    int get_uint32(const char *key, uint32_t* value);
    int get_int64(const char *key, int64_t* value);
    int get_uint64(const char *key, uint64_t* value);
    int get_array(const char *key, Pack* pack, size_t* arr_size);
    int get_object_arr(int index, Pack* pack);
    int get_str_arr(int index,Slice* value);

    int put_raw(const char *key, const Slice& value);
    int put_str(const char *key, const Slice& value);
    int put_int32(const char *key, int32_t value);
    int put_uint32(const char *key, uint32_t value);
    int put_int64(const char *key, int64_t value);
    int put_uint64(const char *key, uint64_t value);
    int put_null(const char *key);
    int put_array(const char *key, Pack* pack);
    int put_object(const char *key, Pack* pack);
    int put_object_to_array(Pack* pack);

    char* data() { return (char*)mc_pack_get_buffer(_pack); }
    size_t size() { return mc_pack_get_size(_pack); }

private:
    Pack(mc_pack_t* pack) : _pack(pack), _tmp_buf(NULL) { }

    mc_pack_t* _pack;
    Slice* _tmp_buf;
};

}  // namespace zf

#endif  // __STORE_PACK_H_
