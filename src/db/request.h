#ifndef __STORE_REQUEST_H_
#define __STORE_REQUEST_H_

#include "util/pack.h"
#include "util/slice.h"
#include "util/store_define.h"
namespace store {

class Request {
public:
    Request(Pack* pack, unsigned long log_id, bool from_raw);    
    ~Request();

    Pack* pack() const { return _pack; }
    Slice method() const;
    Slice key() const;
    Slice raw() const { return Slice(_pack->data(), _pack->size()); }
    unsigned long log_id() const { return _log_id; }
    static Request* from_raw(const Slice &raw, unsigned long log_id);
    static Request* from_pack(Pack *pack, unsigned long log_id);
private:
    bool _from_raw;
    Pack* _pack;
    unsigned long _log_id;
};


}  // namespace store

#endif  // __STORE_REQUEST_H_
