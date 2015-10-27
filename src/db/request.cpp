#include "db/request.h"
#include "db/db_define.h"

namespace store {
Request::Request(Pack *pack, unsigned long log_id, bool from_raw)
        : _from_raw(from_raw), _pack(pack), _log_id(log_id)
{
    
}

Request::~Request() {
    // release the pack if the request was created from raw data
    if (_from_raw)
        delete _pack;
}

Request* Request::from_raw(const Slice &raw, unsigned long log_id) {
    Pack *pack = new Pack;
    int ret = pack->open(raw, Pack::READ);
    if (ret != 0) {
        delete pack;
        return NULL;
    }
    return new Request(pack, log_id, true);
}

Request* Request::from_pack(Pack *pack, unsigned long log_id) {
    return new Request(pack, log_id, false);    
}


Slice Request::method() const{
    Slice method;
    int ret = _pack->get_str(STORE_REQ_METHOD, &method);
    if (ret != 0) {
        return Slice();
    } 
    return method;
}

Slice Request::key() const{
    Slice key;
    int ret = _pack->get_str("key", &key);
    if(ret != 0){
        return Slice();
    }
    return key;
}

}
