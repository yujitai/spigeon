#include "db/response.h"

#include "db/db_define.h"
#include "util/log.h"
#include "util/slice.h"
#include "util/zmalloc.h"

namespace store {

Response::Response(int buffer_size)
        : _pack(NULL), _buffer(NULL), _buffer_size(buffer_size), _finished(false),
          _body_size(0)
{
    assert(buffer_size >= (int)sizeof(nshead_t));
    if ((_buffer_size - sizeof(nshead_t)) > 0) 
        _pack = new Pack();
    _buffer = (char*)zmalloc(_buffer_size);        
}

Response::~Response() {
    delete _pack;
    // don't release the buffer if finished
    if (!_finished)
        zfree(_buffer);
}

int Response::init() {
    Slice resp_buffer(_buffer, _buffer_size);
    resp_buffer.remove_prefix(sizeof(nshead_t));
    if (_pack != NULL) {
        int ret = _pack->open(resp_buffer, Pack::WRITE);
        if (ret != 0) {
            log_warning("failed to open response pack, ret: %d", ret);
            return RESPONSE_PACK_ERROR;
        }
    }
    return RESPONSE_OK;
}

void Response::fill_nshead(nshead_t* nshead, unsigned int body_len) {
    memset(nshead, 0, sizeof(nshead_t));
    const char* provider = "";
    nshead->version = 2;
    //nshead->log_id = 1;
    snprintf(nshead->provider, 16, provider);
    nshead->magic_num = NSHEAD_MAGICNUM;
    //nshead->reserved = 0;
    nshead->body_len = body_len;
}

int Response::finish() {
    //uint64_t body_len = 0;
    if (_pack != NULL) {
        int ret = _pack->close();
        if (ret != 0) {
            log_warning("failed to close response pack, ret: %d", ret);
            return RESPONSE_PACK_ERROR;;
        }
        _body_size = _pack->size();
    }
    
    fill_nshead((nshead_t*)_buffer, _body_size);
    _finished = true;
    return RESPONSE_OK;
}

}  // namespace store
