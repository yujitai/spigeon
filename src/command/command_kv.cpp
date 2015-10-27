#include "command/command_kv.h"
#include "command/command.h"
#include "db/db_define.h"
#include "util/utils.h"
#include "db/request.h"
#include "engine/engine.h"

namespace store {


#define SLOWLOG_PREPARE(time) \
    long long start = ustime(); \
    long long threshold = time;

#define SLOWLOG_CHECKPOINT1(format, args...)            \
    long long checkpoint1 = ustime();                   \
    if ((checkpoint1 - start) > threshold) {            \
        log_warning("SLOWLOG: %lldus "format,           \
                (checkpoint1 - start), ##args);     \
    }

#define SLOWLOG_CHECKPOINT2(format, args...)                    \
    long long checkpoint2 = ustime();                           \
    if ((checkpoint2 - checkpoint1) > threshold) {              \
        log_warning("SLOWLOG: %lldus "format,                   \
                (checkpoint2 - checkpoint1), ##args);       \
    }

#define INT64_STRING_MAX_LEN 20

#define RETURN_IF_PARAM_ERROR(param, ret)                                 \
    if ((ret)) {                                                          \
        log_warning("failed to get %s from request, ret:%d",              \
                    (param), (ret));                                      \
        return Status::BadRequest(param, "not exists or type incorrect"); \
    }

#define RETURN_IF_TYPE_ERROR(param, ret)                           \
    if ((!ret)) {                                                  \
        log_warning("%s can't be parsed into a long long", param); \
        return Status::UnsupportedType(param, "type incorrect");        \
    }

#define RETURN_IF_ENGINE_ERROR(op, ret)                                 \
    if ((ret) == ENGINE_ERROR) {                                        \
        log_fatal("engine fatal error");                                \
        return Status::InternalError(op, "engine maybe corrupted");     \
    }

#define RETURN_IF_NOTFOUND(key, ret) \
    if ((ret) == ENGINE_NOTFOUND) {     \
        return Status::NotFound(key);    \
    }

// we don't need to set the value to null if something wrong happend with packing
#define RETURN_IF_PACK_ERROR(ret)                          \
    if ((ret)) {                                           \
        log_warning("failed to pack result, ret:%d", ret);   \
        return Status::EntityTooLarge("response too large");    \
    }

//-- get

CommandGet::CommandGet(BinLog *binlog, Engine *engine) :
    Command(binlog, engine) { }

CommandGet::~CommandGet() { }

Status CommandGet::extract_params(const Request &req) {
    int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
    RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);        

    return Status::OK();
}

Status CommandGet::process(uint64_t binlog_id, Pack *pack) {
    UNUSED(binlog_id);
    UNUSED(_slowlog_threshold);
    int ret;
    std::string value;

    if (pack) {
        ret = pack->put_str(STORE_REQ_KEY, _key);
        RETURN_IF_PACK_ERROR(ret);
    }

    ret = _engine->get(_key, &value);
    RETURN_IF_ENGINE_ERROR("get", ret);
    RETURN_IF_NOTFOUND(_key, ret);
    Slice value_slice(value);

    ValueHeader* value_header = (ValueHeader*)value_slice.data(); 
    if(value_header->type != ValueHeader::RAW_TYPE){
        return Status::UnsupportedType("data_type", "type incorrect");
    }
    value_slice.remove_prefix(sizeof(ValueHeader));
    if (pack) {
        ret = pack->put_raw(STORE_REQ_VALUE, value_slice);
        RETURN_IF_PACK_ERROR(ret);
    }
    return Status::OK();
}

//-- set

#define STORE_MAX_DELTA_TIME (10*365*24*3600)
uint32_t sanitize_time(int32_t seconds) {
    if (seconds == 0) {
        return 0;
    } else if (seconds < 0) {
        return time(NULL);
    } else if (seconds < STORE_MAX_DELTA_TIME) {
        return time(NULL) + seconds;
    } else {
        return seconds;
    }
}

CommandSet::CommandSet(BinLog *binlog, Engine *engine) :
    Command(binlog, engine), _value(""), _seconds(0) { }

CommandSet::~CommandSet() { }

Status CommandSet::extract_params(const Request &req) {
    int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
    RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

    ret = req.pack()->get_raw(STORE_REQ_VALUE, &_value);
    RETURN_IF_PARAM_ERROR(STORE_REQ_VALUE, ret);

    //optional
    ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);

    return Status::OK();
}

Status CommandSet::process(uint64_t binlog_id, Pack *pack) {
    int ret;
   
    SLOWLOG_PREPARE(_slowlog_threshold);
    if (pack) {
        ret = pack->put_str(STORE_REQ_KEY, _key);
        RETURN_IF_PACK_ERROR(ret);
    }

    std::string value_check;
    ret = _engine->get(_key, &value_check);
    RETURN_IF_ENGINE_ERROR("get", ret);
    if(ret != ENGINE_NOTFOUND){
        Slice value_slice(value_check);
        ValueHeader* value_header = (ValueHeader*)value_slice.data();
        if(value_header->type != ValueHeader::RAW_TYPE){
            return Status::UnsupportedType("data_type", "type incorrect");
        }
    }
    SLOWLOG_CHECKPOINT1("get data frome engine. key:%s", _key.data());
    
    size_t buf_len = sizeof(ValueHeader) + _value.size();
    char*  buf = (char*)zmalloc(buf_len);
    
    BufferHolder buf_holder(buf);
    memset(buf, 0, sizeof(ValueHeader));
    ValueHeader* value_header = (ValueHeader*)buf;
    value_header->type = ValueHeader::RAW_TYPE;
    value_header->binlog_id = binlog_id;
    
    memcpy(buf + sizeof(ValueHeader), _value.data(), _value.size());
    
    Slice value(buf, buf_len);
    ret = _engine->set(_key, value, sanitize_time(_seconds));
    RETURN_IF_ENGINE_ERROR("set", ret);
    SLOWLOG_CHECKPOINT2("store data to engine. key:%s", _key.data());
    return Status::OK();
}

//-- setex

CommandSetEx::CommandSetEx(BinLog *binlog, Engine *engine) :
    Command(binlog, engine), _value(""), _seconds(0) { }

CommandSetEx::~CommandSetEx() { }

Status CommandSetEx::extract_params(const Request &req) {
    int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
    RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

    ret = req.pack()->get_raw(STORE_REQ_VALUE, &_value);
    RETURN_IF_PARAM_ERROR(STORE_REQ_VALUE, ret);

    ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
    RETURN_IF_PARAM_ERROR(STORE_REQ_SECONDS, ret);

    return Status::OK();
}

Status CommandSetEx::process(uint64_t binlog_id, Pack *pack) {
    int ret;
    
    SLOWLOG_PREPARE(_slowlog_threshold);
    if (pack) {
        ret = pack->put_str(STORE_REQ_KEY, _key);
        RETURN_IF_PACK_ERROR(ret);
    }

    std::string value_check;
    ret = _engine->get(_key, &value_check);
    RETURN_IF_ENGINE_ERROR("get", ret);
    if(ret != ENGINE_NOTFOUND){
        Slice value_slice(value_check);
        ValueHeader* value_header = (ValueHeader*)value_slice.data();
        if(value_header->type != ValueHeader::RAW_TYPE){
            return Status::UnsupportedType("data_type", "type incorrect");
        }
    }
    SLOWLOG_CHECKPOINT1("get data frome engine. key:%s", _key.data());

    size_t buf_len = sizeof(ValueHeader) + _value.size();
    char*  buf = (char*)zmalloc(buf_len);
    
    BufferHolder buf_holder(buf);
    memset(buf, 0, sizeof(ValueHeader));
    ValueHeader* slicehdr = (ValueHeader*)buf;
    slicehdr->type = ValueHeader::RAW_TYPE;
    slicehdr->binlog_id = binlog_id;
    
    memcpy(buf + sizeof(ValueHeader), _value.data(), _value.size());
    
    Slice value(buf, buf_len);
    ret = _engine->set(_key, value, sanitize_time(_seconds));
    RETURN_IF_ENGINE_ERROR("set", ret);
    SLOWLOG_CHECKPOINT2("store data to engine. key:%s", _key.data());
    return Status::OK();
}

//-- del

CommandDel::CommandDel(BinLog *binlog, Engine *engine) :
    Command(binlog, engine) { }

CommandDel::~CommandDel() { }

Status CommandDel::extract_params(const Request &req) {
    int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
    RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

    return Status::OK();
}

Status CommandDel::process(uint64_t binlog_id, Pack *pack) {
    UNUSED(binlog_id);
    int ret;
    if (pack) {
        ret = pack->put_str(STORE_REQ_KEY, _key);
        RETURN_IF_PACK_ERROR(ret);
    }

    ret = _engine->del(_key);
    RETURN_IF_ENGINE_ERROR("del", ret);
    return Status::OK();
}

//-- incrby

CommandIncrBy::CommandIncrBy(BinLog *binlog, Engine *engine):
        Command(binlog, engine) {}

CommandIncrBy::~CommandIncrBy() {}

Status CommandIncrBy::extract_params(const Request &req) {
    int ret;

    ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
    RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

    Slice value;
    ret = req.pack()->get_raw(STORE_REQ_VALUE, &value);
    RETURN_IF_PARAM_ERROR(STORE_REQ_VALUE, ret);

    ret = string2ll(value.data(), value.size(), (long long*)&increment);
    RETURN_IF_TYPE_ERROR(STORE_REQ_VALUE, ret);
    return Status::OK();
}

Status CommandIncrBy::process( uint64_t binlog_id, Pack *pack) {
    int ret;
    SLOWLOG_PREPARE(_slowlog_threshold);
    if (pack) {
        ret = pack->put_str(STORE_REQ_KEY, _key);
        RETURN_IF_PACK_ERROR(ret);
    }
    // step 1: load value from engine
    std::string value;
    uint32_t seconds = 0;
    ret = _engine->get(_key, &value, &seconds);
    RETURN_IF_ENGINE_ERROR("incrby", ret);
    SLOWLOG_CHECKPOINT1("get data frome engine. key:%s", _key.data());

    int64_t num_value;
    if (ret == ENGINE_NOTFOUND) {
        // default to 0 if not found
        num_value = 0;
    } else {
        Slice value_slice(value);
        ValueHeader* value_header = (ValueHeader*)value_slice.data();
        if(value_header->type != ValueHeader::RAW_TYPE){
            return Status::UnsupportedType("data_type", "type incorrect");
        }
        value_slice.remove_prefix(sizeof(ValueHeader));

        ret = string2ll(value_slice.data(),
                        value_slice.size(),
                        (long long*)&num_value);
        RETURN_IF_TYPE_ERROR("value", ret);
    }
    // step 2: increase the value
    num_value += increment;

    // step 3: save the new value
    size_t buffer_size = sizeof(ValueHeader) + INT64_STRING_MAX_LEN + 1;
    char buffer[buffer_size];
    ValueHeader *hdr = (ValueHeader*)buffer;
    memset(buffer, 0, sizeof(ValueHeader));
    hdr->binlog_id = binlog_id;
    
    int len = ll2string(buffer+sizeof(ValueHeader),
                        INT64_STRING_MAX_LEN + 1,
                        num_value);

    ret = _engine->set(_key, Slice(buffer, sizeof(ValueHeader)+len), seconds);
    RETURN_IF_ENGINE_ERROR("incrby", ret);

    if (pack) {
        ret = pack->put_raw(STORE_REQ_VALUE,
                            Slice(buffer+sizeof(ValueHeader), len));
        RETURN_IF_PACK_ERROR(ret);
    }
    SLOWLOG_CHECKPOINT2("store data to engine. key:%s", _key.data());
    return Status::OK();
}

}  // namespace store
