
#include "mcpack_reader.h"
#include <stdio.h>

namespace store {

McPackReader::McPackReader() : _data(NULL) { }

McPackReader::~McPackReader() {
    close();
}

int McPackReader::open(const char* buf, const int buf_len) {
    _data = mc_pack_open_r(buf, buf_len, _tmpbuf, sizeof(_tmpbuf));
    if (MC_PACK_PTR_ERR(_data)) {
        printf("open mcpack failed!error[%s]", mc_pack_perror((long)_data));
        _data = NULL;
        return MCPACK_READER_OPEN_ERROR;
    }
    return 0;
}

void McPackReader::close() {
    if (NULL != _data) {
        mc_pack_close(_data);
        _data = NULL;
    }
}

int McPackReader::get_raw(const char* key, const char* & value, unsigned int& value_len) {
    value = (const char*)mc_pack_get_raw(_data, key, &value_len);
    if (MC_PACK_PTR_ERR(value)) {
        printf("mcpack get raw failed! error[%s]", mc_pack_perror((long)_data));
        value = NULL;
        value_len = 0;
        return MCPACK_READER_GET_RAW_ERROR;
    }
    return 0;
}

int McPackReader::get_str(const char* key, const char* & value) {
    value = mc_pack_get_str(_data, key);
    if (MC_PACK_PTR_ERR(value)) {
        printf("mcpack get raw failed! error[%s]", mc_pack_perror((long)_data));
        value = NULL;
        return MCPACK_READER_GET_STR_ERROR;
    }
    return 0;
}

int McPackReader::get_int32(const char* key, int& value) {
    int ret;
    if ((ret = mc_pack_get_int32(_data, key, &value)) < 0) {
        printf("mcpack get failed! ret[%d]", ret);
        return MCPACK_READER_GET_STR_ERROR;
    }
    return 0;
}

int McPackReader::get_uint32(const char* key, uint32_t& value) {
    int ret;
    mc_uint32_t tmp_mc_uint32;
    if ((ret = mc_pack_get_uint32(_data, key, &tmp_mc_uint32)) < 0) {
        printf("mcpack get failed! ret[%d]", ret);
        return MCPACK_READER_GET_STR_ERROR;
    }
    value = tmp_mc_uint32;
    return 0;
}

int McPackReader::get_uint64(const char* key, uint64_t& value) {
    int ret;
    mc_uint64_t tmp_mc_uint64;
    if ((ret = mc_pack_get_uint64(_data, key, &tmp_mc_uint64)) < 0) {
        printf("mcpack get failed! ret[%d]", ret);
        return MCPACK_READER_GET_STR_ERROR;
    }
    value = tmp_mc_uint64;
    return 0;
}

}  // namespace store

