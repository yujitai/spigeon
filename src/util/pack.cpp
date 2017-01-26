#include "util/pack.h"
#include "util/log.h"
#include "util/slice.h"
#include "util/zmalloc.h"

namespace store {

Pack::Pack() : _pack(NULL), _tmp_buf(NULL) { }

Pack::~Pack() {
    if (_tmp_buf != NULL) {
        zfree((void*)_tmp_buf->data());
        delete _tmp_buf;
        _tmp_buf = NULL;
    }
}

int Pack::open(const Slice& buf, enum OpMode mode) {
    size_t tmp_buf_len = 3 * buf.size() + 10240; //FIXME: how big is enough?
    char* tmp_buf = (char*)zmalloc(tmp_buf_len);
    switch (mode) {
    case READ:
        _pack = mc_pack_open_r(buf.data(), buf.size(), tmp_buf, tmp_buf_len);
        break;
    case WRITE:
        _pack = mc_pack_open_w(2, (char*)buf.data(), buf.size(), tmp_buf, tmp_buf_len);
        break;
    default:
        log_warning("open mode error!");
        zfree(tmp_buf);
        tmp_buf = NULL;
        return PACK_MODE_ERROR;
    }
    long pack_err = MC_PACK_PTR_ERR(_pack);
    if (pack_err) {
        log_warning("open pack failed! error[%s]", mc_pack_perror(pack_err));
        zfree(tmp_buf);
        tmp_buf = NULL;
        _pack = NULL;
        return PACK_OPEN_ERROR;
    }
    _tmp_buf = new Slice(tmp_buf, tmp_buf_len);
    return PACK_OK;
}

int Pack::finish() {
    mc_pack_finish(_pack);
    return PACK_OK;
}

int Pack::close() {
    mc_pack_close(_pack);
    return PACK_OK;
}

int Pack::get_raw(const char *key, Slice* value) {
    unsigned int v_len;
    const char* v = (const char*)mc_pack_get_raw(_pack, key, &v_len);
    long pack_err = MC_PACK_PTR_ERR(v);
    if (pack_err) {
        log_debug("pack get key[%s] failed! error[%s]", key, mc_pack_perror(pack_err));
        v = NULL;
        return PACK_GET_ERROR;
    }
    *value = Slice(v, v_len);
    return PACK_OK;
}

int Pack::get_str(const char *key, Slice* value) {
    const char* v = mc_pack_get_str(_pack, key);
    long pack_err = MC_PACK_PTR_ERR(v);
    if (pack_err) {
        log_debug("pack get key[%s] failed! error[%s]", key, mc_pack_perror(pack_err));
        v = NULL;
        return PACK_GET_ERROR;
    }
    *value = Slice(v);
    return PACK_OK;
}

int Pack::get_int32(const char *key, int32_t *value) {
    mc_int32_t mc_int32;
    int ret = mc_pack_get_int32(_pack, key, &mc_int32);
    if (ret < 0) {
        log_debug("pack get key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_GET_ERROR;
    }
    *value = mc_int32;
    return PACK_OK;
}

int Pack::get_uint32(const char *key, uint32_t *value) {
    mc_uint32_t mc_uint32;
    int ret = mc_pack_get_uint32(_pack, key, &mc_uint32);
    if (ret < 0) {
        log_debug("pack get key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_GET_ERROR;
    }
    *value = mc_uint32;
    return PACK_OK;
}

int Pack::get_int64(const char *key, int64_t *value) {
    mc_int64_t mc_int64;
    int ret = mc_pack_get_int64(_pack, key, &mc_int64);
    if (ret < 0) {
        log_debug("pack get key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_GET_ERROR;
    }
    *value = mc_int64;
    return PACK_OK;
}

int Pack::get_uint64(const char *key, uint64_t* value) {
    mc_uint64_t mc_uint64;
    int ret = mc_pack_get_uint64(_pack, key, &mc_uint64);
    if (ret < 0) {
        log_debug("pack get key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_GET_ERROR;
    }
    *value = mc_uint64;
    return PACK_OK;
}

int Pack::put_raw(const char *key, const Slice& value) {
    int ret = mc_pack_put_raw(_pack, key, value.data(), value.size());
    if (ret != 0) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_PUT_ERROR;
    }
    return PACK_OK;
}

int Pack::put_str(const char *key, const Slice& value) {
    int ret = mc_pack_put_str(_pack, key, value.data());
    if (ret != 0) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_PUT_ERROR;
    }
    return PACK_OK;
}

int Pack::put_int32(const char *key, int32_t value) {
    int ret = mc_pack_put_int32(_pack, key, value);
    if (ret != 0) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_PUT_ERROR;
    }
    return PACK_OK;
}

int Pack::put_uint32(const char *key, uint32_t value) {
    int ret = mc_pack_put_uint32(_pack, key, value);
    if (ret != 0) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_PUT_ERROR;
    }
    return PACK_OK;
}

int Pack::put_int64(const char *key, int64_t value) {
    int ret = mc_pack_put_int64(_pack, key, value);
    if (ret != 0) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_PUT_ERROR;
    }
    return PACK_OK;
}

int Pack::put_uint64(const char *key, uint64_t value) {
    int ret = mc_pack_put_uint64(_pack, key, value);
    if (ret != 0) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_PUT_ERROR;
    }
    return PACK_OK;
}

int Pack::put_array(const char *key, Pack* pack) {
    mc_pack_t* subpack = mc_pack_put_array(_pack, key);
    long pack_err = MC_PACK_PTR_ERR(subpack);
    if (pack_err) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(pack_err));
        return PACK_PUT_ERROR;
    }
    *pack = Pack(subpack);
    return PACK_OK;
}

int Pack::put_object_to_array(Pack* pack) {
    mc_pack_t* subpack = mc_pack_put_object(_pack, NULL);
    long pack_err = MC_PACK_PTR_ERR(subpack);
    if (pack_err) {
        log_warning("pack put object to array failed! error[%s]", mc_pack_perror(pack_err));
        return PACK_PUT_ERROR;
    }
    *pack = Pack(subpack);
    return PACK_OK;
}

int Pack::put_object(const char *key, Pack* pack) {
    mc_pack_t* subpack = mc_pack_put_object(_pack, key);
    long pack_err = MC_PACK_PTR_ERR(subpack);
    if (pack_err) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(pack_err));
        return PACK_PUT_ERROR;
    }
    *pack = Pack(subpack);
    return PACK_OK;
}

int Pack::put_null(const char *key) {
    int ret = mc_pack_put_null(_pack, key);
    if (ret != 0) {
        log_warning("pack put key[%s] failed! error[%s]", key, mc_pack_perror(ret));
        return PACK_PUT_ERROR;
    }
    return PACK_OK;
}

int Pack::get_array(const char *key, Pack* pack, size_t* arr_size) {
    mc_pack_t* subpack = mc_pack_get_array(_pack, key);
    long pack_err = MC_PACK_PTR_ERR(subpack);
    if (pack_err) {
        log_debug("pack get key[%s] failed! error[%s]", key, mc_pack_perror(pack_err));
        return PACK_PUT_ERROR;
    }
    *pack = Pack(subpack);
    *arr_size = mc_pack_get_item_count(subpack);
    return PACK_OK;
}

int Pack::get_object_arr(int index, Pack* pack) {
    mc_pack_t* subpack = mc_pack_get_object_arr(_pack, index);
    long pack_err = MC_PACK_PTR_ERR(subpack);
    if (pack_err) {
        log_debug("pack get index[%d] failed! error[%s]", index, mc_pack_perror(pack_err));
        return PACK_PUT_ERROR;
    }
    *pack = Pack(subpack);
    return PACK_OK;
}

}  // namespace store
