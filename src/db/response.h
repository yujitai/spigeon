#ifndef __STORE_RESPONSE_H_
#define __STORE_RESPONSE_H_

#include <stdint.h>
#include "mc_pack.h"
#include "util/pack.h"
#include "util/nshead.h"

namespace store {

class Slice;

// [-1000, -1999] event
enum response_error_t {
    // [-1600, -1700] event
    RESPONSE_OK  = 0,
    RESPONSE_PACK_ERROR   = -1600,
    RESPONSE_MALLOC_ERROR = -1601,
    RESPONSE_ERROR = -1999
};

class Response {
public:
    Response(int buffer_size);
    ~Response();

    int init();

    Pack* pack() { return _pack; }

    const char *data() { return _buffer; }
    // FIX this method may fail
    int size() { return (sizeof(nshead_t) + _body_size); }
    
    int finish();
private:
    void fill_nshead(nshead_t* nshead, unsigned int body_len);

    Pack* _pack;
    char *_buffer;
    int _buffer_size;
    bool _finished;
    int _body_size;
};

}  // namespace store

#endif  // __STORE_RESPONSE_H_
