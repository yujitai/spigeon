
#include "mcpack_writer.h"
#include <stdio.h>

namespace store {

McPackWriter::McPackWriter() : _buf(NULL), _buf_len(0), _version(MCPACK_VERSION_DEF), _data(NULL) { }

McPackWriter::~McPackWriter() {
    close();
}

int McPackWriter::open(char* buf, const int buf_len, int version) {
    _version = version;
    _data = mc_pack_open_w(version, buf, buf_len, _tmpbuf, sizeof(_tmpbuf));
    if (MC_PACK_PTR_ERR(_data)) {
        printf("open mcpack failed! error[%s]", mc_pack_perror((long)_data));
        _data = NULL;
        return MCPACK_WRITER_OPEN_ERROR;
    }
    _buf = buf;
    _buf_len = buf_len;
    return 0;
}

void McPackWriter::close() {
    //if (_data != NULL) {
    //    mc_pack_close(_data);
    //    _data = NULL;
    //}
}

int McPackWriter::reset_mc_pack() {
    if (_data != NULL) {
        mc_pack_close(_data);
        _data = NULL;
    }
    if (NULL == _buf) {
        printf("mcpack not open!");
        return MCPACK_WRITER_NOT_OPEN;
    }
    _data = mc_pack_open_w(_version, _buf, _buf_len, _tmpbuf, sizeof(_tmpbuf));
    if (MC_PACK_PTR_ERR(_data)) {
        printf("open mcpack failed! error[%s]", mc_pack_perror((long)_data));
        _data = NULL;
        return MCPACK_WRITER_OPEN_ERROR;
    }
    return 0;
}

int McPackWriter::put_raw(const char* key, const char* value, const int value_len) {
    int ret;
    if ((ret = mc_pack_put_raw(_data, key, value, value_len)) != 0) {
        printf("mcpack put raw failed! ret[%d]", ret);
        return MCPACK_WRITER_PUT_DATA_ERROR;
    }
    return 0;
}

int McPackWriter::put_str(const char* key, const char* value) {
    int ret;
    if ((ret = mc_pack_put_str(_data, key, value)) != 0) {
        printf("mcpack put str failed! ret[%d]", ret);
        return MCPACK_WRITER_PUT_DATA_ERROR;
    }
    return 0;
}

int McPackWriter::put_int32(const char* key, int value) {
    int ret;
    if ((ret = mc_pack_put_int32(_data, key, value)) != 0) {
        printf("mcpack put failed! ret[%d]", ret);
        return MCPACK_WRITER_PUT_DATA_ERROR;
    }
    return 0;
}

int McPackWriter::put_uint32(const char* key, uint32_t value) {
    int ret;
    if ((ret = mc_pack_put_uint32(_data, key, value)) != 0) {
        printf("mcpack put failed! ret[%d]", ret);
        return MCPACK_WRITER_PUT_DATA_ERROR;
    }
    return 0;
}

int McPackWriter::put_uint64(const char* key, uint64_t value) {
    int ret;
    if ((ret = mc_pack_put_uint64(_data, key, value)) != 0) {
        printf("mcpack put failed! ret[%d]", ret);
        return MCPACK_WRITER_PUT_DATA_ERROR;
    }
    return 0;
}

}  // namespace store

