#include "command/command_list.h"
#include "command/command.h"
#include "db/db_define.h"
#include "util/utils.h"
#include "db/request.h"
#include "engine/engine.h"
#include "command/list.pb.h"
#include "itemlist.h"


namespace store {

    #define STORE_MAX_DELTA_TIME (10*365*24*3600)

    #define RETURN_IF_PARAM_ERROR(param, ret)                                 \
        if((ret)) {                                                           \
            log_warning("failed to get %s from request, ret:%d",              \
                    (param), (ret));                                          \
            return Status::BadRequest(param, "not exists or type incorrect"); \
        }

    #define RETURN_IF_TYPE_ERROR(param, ret)                                  \
        if ((!ret)) {                                                         \
            log_warning("%s can't be parsed into a long long", param);        \
            return Status::UnsupportedType(param, "type incorrect");          \
        }

    #define RETURN_IF_ENGINE_ERROR(op, ret)                                   \
        if ((ret) == ENGINE_ERROR) {                                          \
            log_fatal("engine fatal error");                                  \
            return Status::InternalError(op, "engine maybe corrupted");       \
        }
    #define RETURN_IF_NOTFOUND(key, ret)                                      \
        if ((ret) == ENGINE_NOTFOUND) {                                       \
            return Status::NotFound(key);                                     \
        }

    // we don't need to set the value to null if something wrong happend with packing
    #define RETURN_IF_PACK_ERROR(ret)                                         \
        if ((ret)) {                                                          \
            log_warning("failed to pack result, ret:%d", ret);                \
            return Status::EntityTooLarge("response too large");              \
        }

    #define SLOWLOG_PREPARE()                                                 \
        long long start_time = ustime();                                      \
        long long threshold = _slowlog_threshold;

    #define SLOWLOG_CHECKPOINT1(format, args...)                              \
        long long checkpoint1 = ustime();                                     \
        if ((checkpoint1 - start_time) > threshold) {                         \
            log_warning("SLOWLOG: %lldus "format,                             \
                        (checkpoint1 - start_time), ##args);                  \
        }

    #define SLOWLOG_CHECKPOINT2(format, args...)                              \
        long long checkpoint2 = ustime();                                     \
        if ((checkpoint2 - checkpoint1) > threshold) {                        \
            log_warning("SLOWLOG: %lldus "format,                             \
                    (checkpoint2 - checkpoint1), ##args);                     \
        }

    #define SLOWLOG_CHECKPOINT3(format, args...)                              \
        long long checkpoint3 = ustime();                                     \
        if ((checkpoint3 - checkpoint2) > threshold) {                        \
            log_warning("SLOWLOG: %lldus "format,                             \
                    (checkpoint3 - checkpoint2), ##args);                     \
        }
    
    //---Lpush
    
    CommandLpush::CommandLpush(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _member(""), _extra(""), _seconds(0){ }

    CommandLpush::~CommandLpush(){ }
             
    Status CommandLpush::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

        ret = req.pack()->get_raw(STORE_REQ_MEMBER, &_member);
        RETURN_IF_PARAM_ERROR(STORE_REQ_MEMBER, ret);
         
        ret = req.pack()->get_raw(STORE_REQ_EXTRA, &_extra);
     
        ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
        if(ret)_seconds = 0;
        return Status::OK();
    }

    Status CommandLpush::process(uint64_t binlog_id, Pack *pack) {
        int32_t ret;
        if (pack) {
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }

        //从磁盘读数据并将protobuf转换为队列

        Item lpush_node;
        SLOWLOG_PREPARE();
        lpush_node.member = _member.to_string();
        lpush_node.extra = _extra.to_string();
    
        ItemList value_list; 
        Status store_status = value_list.get_store_data(_engine, _key);
        if(!store_status.ok()){
            return store_status;
        }
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        //进行lpush操作

        int32_t list_leng = 0;
        list_leng = value_list.lpush(lpush_node);
        SLOWLOG_CHECKPOINT2("method call is timeout.  key:%s", _key.data()); 
        if(pack){
            ret = pack->put_int32(STORE_RESP_LENGTH, list_leng);
            RETURN_IF_PACK_ERROR(ret);
        }

        store_status = value_list.store_data(_engine,
                binlog_id,
                _key,
                _seconds);
        SLOWLOG_CHECKPOINT3("store data to disk or memory. key:%s", _key.data());
        if(store_status.ok()){
            return Status::OK();
        }else{
            return store_status;
        }
    }

    //---Rpush
    
    CommandRpush::CommandRpush(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _member(""), _extra(""), _seconds(0){ }
     
    CommandRpush::~CommandRpush(){ }

    Status CommandRpush::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

        ret = req.pack()->get_raw(STORE_REQ_MEMBER, &_member);
        RETURN_IF_PARAM_ERROR(STORE_REQ_MEMBER, ret);

        ret = req.pack()->get_raw(STORE_REQ_EXTRA, &_extra);

        ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
        if(ret)_seconds = 0;
        return Status::OK();
    }

    Status CommandRpush::process(uint64_t binlog_id, Pack *pack){
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }
        //从磁盘读取并转换

        SLOWLOG_PREPARE();
        Item rpush_node;
        rpush_node.member = _member.to_string();
        rpush_node.extra = _extra.to_string();
        ItemList value_list;
        Status store_status = value_list.get_store_data(_engine, _key);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        if(!store_status.ok()){
            return store_status;
        }
        //进行rpush操作
        
        int32_t list_leng = 0;
        list_leng = value_list.rpush(rpush_node);
        if(pack){
            ret = pack->put_int32(STORE_RESP_LENGTH, list_leng);
            RETURN_IF_PACK_ERROR(ret);
        }
        SLOWLOG_CHECKPOINT2("method call is timeout.  key:%s", _key.data());

        //转成protobuf并存储到磁盘

        store_status = value_list.store_data(_engine,
                binlog_id,
                _key,
                _seconds);
        SLOWLOG_CHECKPOINT3("store data to disk or memory. key:%s", _key.data());
        if(store_status.ok()){
            return Status::OK();
        }else{
            return store_status;
        }
    }
    
    //--Lsetbymember
    
    CommandLsetbymember::CommandLsetbymember(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _member(""), _extra(""), _seconds(0){ }

    CommandLsetbymember::~CommandLsetbymember(){ }

    Status CommandLsetbymember::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);
        
        ret = req.pack()->get_raw(STORE_REQ_MEMBER, &_member);
        RETURN_IF_PARAM_ERROR(STORE_REQ_MEMBER, ret);
        
        ret = req.pack()->get_raw(STORE_REQ_EXTRA, &_extra);
        RETURN_IF_PARAM_ERROR(STORE_REQ_EXTRA, ret);
 
        ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
        if(ret)_seconds = 0;
        return Status::OK();
    }

    Status CommandLsetbymember::process( uint64_t binlog_id, Pack *pack){
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }
        SLOWLOG_PREPARE();
        Item value_node;
        value_node.member = _member.to_string();
        value_node.extra = _extra.to_string();
          
        ItemList value_list;
        bool is_success = false;
        Status store_status = value_list.get_store_data(_engine, _key);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        if(!store_status.ok())return store_status;

        is_success = value_list.lsetbymember(value_node);
        SLOWLOG_CHECKPOINT2("method call is timeout.  key:%s", _key.data());

        if(is_success == false){
            return Status::NotFound("this member not fine!");
        }

        store_status = value_list.store_data(_engine,
                binlog_id,
                _key,
                _seconds);
        SLOWLOG_CHECKPOINT3("store data to disk or memory. key:%s", _key.data());
        if(store_status.ok()){
            return Status::OK();
        }else {
            return store_status;
        }
    }

    //---Lset

    CommandLset::CommandLset(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _member(""), _extra(""), _seconds(0), _index(0){ }

    CommandLset::~CommandLset(){ }

    Status CommandLset::extract_params(const Request &req){

        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

        ret = req.pack()->get_raw(STORE_REQ_MEMBER, &_member);
        RETURN_IF_PARAM_ERROR(STORE_REQ_MEMBER, ret);
                    
        ret = req.pack()->get_raw(STORE_REQ_EXTRA, &_extra);

        ret = req.pack()->get_int32(STORE_REQ_INDEX, &_index); 
        RETURN_IF_PARAM_ERROR(STORE_REQ_INDEX, ret);

        ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
        if(ret)_seconds = 0;
        return Status::OK();
    }

    Status CommandLset::process( uint64_t binlog_id, Pack *pack){
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }
        SLOWLOG_PREPARE();
        Item index_node;
        index_node.member = _member.to_string();
        index_node.extra = _extra.to_string();
           
        ItemList value_list;
        bool is_success;
        Status store_status = value_list.get_store_data(_engine, _key);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        if(!store_status.ok()){
            return store_status;
        }
        //进行操作 
        is_success = value_list.lset(index_node, _index);
        SLOWLOG_CHECKPOINT2("method call is timeout.  key:%s", _key.data());
        if(is_success == false ){
            return Status::NotFound("this index not found!");
        }
        store_status = value_list.store_data(_engine,
                binlog_id,
                _key,
                _seconds);
        SLOWLOG_CHECKPOINT3("store data to disk or memory. key:%s", _key.data());
        if(store_status.ok()){
            return Status::OK();
        }else{
            return store_status;
        }
    }
    
    //--Lpop
    
    CommandLpop::CommandLpop(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _member(""), _extra(""), _seconds(0){ }

    CommandLpop::~CommandLpop(){ }

    Status CommandLpop::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);
        
        ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
        if(ret)_seconds = 0;
        return Status::OK();
    }

    Status CommandLpop::process(uint64_t binlog_id, Pack *pack){
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }
        SLOWLOG_PREPARE();
        ItemList value_list;
        Status store_status = value_list.get_store_data(_engine, _key);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        if(!store_status.ok())return store_status;
        Item lpop_node;
        lpop_node.member = "";
        lpop_node.extra = "";
        if(value_list.lpop(lpop_node) == false){
            log_debug("key is exist!");
        }
        SLOWLOG_CHECKPOINT2("method call is timeout.  key:%s", _key.data());
        if(pack){

            Slice reps_member(lpop_node.member);
            Slice reps_extra(lpop_node.extra);
            ret = pack->put_raw(STORE_RESP_MEMBER, reps_member);
            RETURN_IF_PACK_ERROR(ret);
            ret = pack->put_raw(STORE_RESP_EXTRA, reps_extra);
            RETURN_IF_PACK_ERROR(ret);
        }
        if(value_list.size() == 0){
            ret = _engine->del(_key);
            RETURN_IF_ENGINE_ERROR("del", ret);
            return Status::OK();
        }
        store_status = value_list.store_data(_engine,
                binlog_id,
                _key,
                _seconds);
        SLOWLOG_CHECKPOINT3("store data to disk or memory. key:%s", _key.data());
        if(store_status.ok()){
            return Status::OK();
        }else {
            return store_status;
        }
    }
    //--Rpop
    
    CommandRpop::CommandRpop(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _member(""), _extra(""), _seconds(0){ }

    CommandRpop::~CommandRpop(){ }

    Status CommandRpop::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);
        ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
        if(ret)_seconds = 0;
        
        return Status::OK();
    }

    Status CommandRpop::process(uint64_t binlog_id, Pack *pack){
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }

        SLOWLOG_PREPARE();
        Item rpop_node;
        rpop_node.member = "";
        rpop_node.extra = "";
        ItemList value_list;
        Status store_status = value_list.get_store_data(_engine, _key);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        if(!store_status.ok())return store_status;
        if(value_list.rpop(rpop_node) == false){
            log_debug("key is not exist");
        }
        SLOWLOG_CHECKPOINT2("method call is timeout.  key:%s", _key.data());
        if(pack){
            Slice resp_member(rpop_node.member);
            Slice resp_extra(rpop_node.extra);
            ret = pack->put_raw(STORE_RESP_MEMBER, resp_member);
            RETURN_IF_PACK_ERROR(ret);
            ret = pack->put_raw(STORE_RESP_EXTRA, resp_extra);
            RETURN_IF_PACK_ERROR(ret);
        }

        if(value_list.size() == 0){
            ret = _engine->del(_key);
            RETURN_IF_ENGINE_ERROR("del", ret);
            return Status::OK();
        }
        store_status = value_list.store_data(_engine,
                binlog_id,
                _key,
                _seconds);
        SLOWLOG_CHECKPOINT3("store data to disk or memory. key:%s", _key.data());
        if(store_status.ok()){
            return Status::OK();
        }else{
            return store_status;
        }
    }
    //---Lrem
    
    CommandLrem::CommandLrem(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _member(""), _extra(""), _seconds(0), _count(0) { }

    CommandLrem::~CommandLrem(){ }

    Status CommandLrem::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

        ret = req.pack()->get_raw(STORE_REQ_MEMBER, &_member);
        RETURN_IF_PARAM_ERROR(STORE_REQ_MEMBER, ret);
            
        ret = req.pack()->get_raw(STORE_REQ_EXTRA, &_extra);

        ret = req.pack()->get_int32(STORE_REQ_COUNT, &_count);
        RETURN_IF_PARAM_ERROR(STORE_REQ_COUNT, ret);
            
        ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
        if(ret)_seconds = 0;
        return Status::OK();
    }

    Status CommandLrem::process( uint64_t binlog_id, Pack *pack){
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }

        SLOWLOG_PREPARE();
        ItemList value_list;
        Status store_status = value_list.get_store_data(_engine, _key);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        if(!store_status.ok())return store_status;

        Item lrem_node;
        lrem_node.member = _member.to_string();
        lrem_node.extra = _extra.to_string();

        int32_t remove_num;
        remove_num = value_list.lrem(lrem_node, _count);
        SLOWLOG_CHECKPOINT2("method call is timeout.  key:%s", _key.data());

        if(pack){
            ret  =  pack->put_int32("remove_num", remove_num);
            RETURN_IF_PACK_ERROR(ret);
        }

        if(value_list.size() == 0){
            ret = _engine->del(_key);
            RETURN_IF_ENGINE_ERROR("del", ret);
            return Status::OK();
        }
        store_status = value_list.store_data(_engine,
                binlog_id,
                _key,
                _seconds);
        SLOWLOG_CHECKPOINT3("store data to disk or memory. key:%s", _key.data());
        if(store_status.ok()){
            return Status::OK();
        }else{
            return store_status;
        }
    }
    //--Lrim
    
    CommandLrim::CommandLrim(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _start(0), _stop(0), _seconds(0){ }

    CommandLrim::~CommandLrim(){ }

    Status CommandLrim::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

        ret = req.pack()->get_int32(STORE_REQ_START, &_start);
        RETURN_IF_PARAM_ERROR(STORE_REQ_START, ret);

        ret = req.pack()->get_int32(STORE_REQ_STOP, &_stop);
        RETURN_IF_PARAM_ERROR(STORE_REQ_STOP, ret);

        ret = req.pack()->get_int32(STORE_REQ_SECONDS, &_seconds);
        if(ret)_seconds = 0;
        return Status::OK();
    }
    Status CommandLrim::process(uint64_t binlog_id, Pack *pack){
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }
        
        SLOWLOG_PREPARE();
        ItemList value_list;
        bool is_success = false;
        Status store_status = value_list.get_store_data(_engine, _key);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        if(!store_status.ok())return store_status;
        is_success = value_list.lrim(_start, _stop);
        SLOWLOG_CHECKPOINT2("method call is timeout.  key:%s", _key.data());
        if(is_success == false){
            ret = _engine->del(_key);
            RETURN_IF_ENGINE_ERROR("del", ret);
            return Status::OK();
        }
        store_status = value_list.store_data(_engine,
                binlog_id,
                _key,
                _seconds);
        SLOWLOG_CHECKPOINT3("store data to disk or memory. key:%s", _key.data());
        if(store_status.ok()){
            return Status::OK();
        }else{
            return store_status;
        }
    }
    //-Llen

    CommandLlen::CommandLlen(BinLog *binlog, Engine *engine) :
        Command(binlog, engine){ }

    CommandLlen::~CommandLlen(){ }

    Status CommandLlen::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

        return Status::OK();
    }

    Status CommandLlen::process(uint64_t binlog_id, Pack *pack){
        UNUSED(binlog_id);
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }
        SLOWLOG_PREPARE();
        int32_t list_leng = 0;
        std::string content_str;
        ret = _engine->get(_key, &content_str);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        RETURN_IF_ENGINE_ERROR("get", ret);
        if(ret == ENGINE_NOTFOUND){
            if(pack){
                ret = pack->put_int32(STORE_RESP_LENGTH, list_leng);
                RETURN_IF_PACK_ERROR(ret);
            }
            return Status::OK();
        }
        Slice value_slice(content_str);
        ValueHeader* value_header = (ValueHeader*)value_slice.data();
        if(value_header->type != ValueHeader::LIST_PB_TYPE){
            log_warning("data type incorrect");
            return Status::UnsupportedType("data_type", "type incorrect");
        }
        value_slice.remove_prefix(sizeof(ValueHeader));
        content_str = value_slice.to_string();

        store_list::list store_to_list;
        store_to_list.ParseFromString(content_str);
        list_leng = store_to_list.itemlist_size();

        if(pack){
            ret = pack->put_int32(STORE_RESP_LENGTH, list_leng);
            RETURN_IF_PACK_ERROR(ret);
        }
        return Status::OK();
    }
    //--Lgetbymember

    CommandLgetbymember::CommandLgetbymember(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _member(""){ }

    CommandLgetbymember::~CommandLgetbymember(){ }

    Status CommandLgetbymember::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);
        
        ret = req.pack()->get_raw(STORE_REQ_MEMBER, &_member);
        RETURN_IF_PARAM_ERROR(STORE_REQ_MEMBER, ret);

        return Status::OK();
    }

    Status CommandLgetbymember::process(uint64_t binlog_id, Pack *pack){
        UNUSED(binlog_id);
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }

        SLOWLOG_PREPARE();
        std::string content_str;
        ret = _engine->get(_key, &content_str);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        RETURN_IF_ENGINE_ERROR("get", ret);
        RETURN_IF_NOTFOUND(_key, ret);
        Slice value_slice(content_str);
        ValueHeader* value_header = (ValueHeader*)value_slice.data();
        if(value_header->type != ValueHeader::LIST_PB_TYPE){
            log_warning("data type incorrect");
            return Status::UnsupportedType("data_type", "type incorrect");
        }
        value_slice.remove_prefix(sizeof(ValueHeader));
        content_str = value_slice.to_string();
                                          
        store_list::list store_to_list;
        store_to_list.ParseFromString(content_str);

        std::string _value;
        for(int32_t idx = 0; idx < store_to_list.itemlist_size(); idx++){
            store_list::list::item item = store_to_list.itemlist(idx);
            _value = item.member();
            if(_member == _value){
                if(pack){
                    Slice resp_member(_value);
                    int ret = pack->put_raw(STORE_RESP_MEMBER, resp_member);
                    RETURN_IF_PACK_ERROR(ret);
                    _value = item.extra();
                    Slice resp_extra(_value);
                    ret = pack->put_raw(STORE_RESP_EXTRA, resp_extra);
                    RETURN_IF_PACK_ERROR(ret);
                    ret = pack->put_int32(STORE_RESP_INDEX,idx);
                    RETURN_IF_PACK_ERROR(ret);
                }
                return Status::OK();
                }
        }
        return Status::NotFound("not found something by member!");
    }
    //--Lindex

    CommandLindex::CommandLindex(BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _index(0){ }

    CommandLindex::~CommandLindex(){ }
     
    Status CommandLindex::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

        ret = req.pack()->get_int32(STORE_REQ_INDEX, &_index);
        RETURN_IF_PARAM_ERROR(STORE_REQ_INDEX, ret);

        return Status::OK();
    }

    Status CommandLindex::process( uint64_t binlog_id, Pack *pack){
        UNUSED(binlog_id);
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }

        SLOWLOG_PREPARE();
        std::string content_str;
        ret = _engine->get(_key, &content_str);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        RETURN_IF_ENGINE_ERROR("get", ret);
        RETURN_IF_NOTFOUND(_key, ret);
        Slice value_slice(content_str);
        ValueHeader* value_header = (ValueHeader*)value_slice.data();
        if(value_header->type != ValueHeader::LIST_PB_TYPE){
            log_warning("data type incorrect");
            return Status::UnsupportedType("data_type", "type incorrect");
        }
        value_slice.remove_prefix(sizeof(ValueHeader));
        content_str = value_slice.to_string();
        store_list::list store_to_list;
        store_to_list.ParseFromString(content_str);
        if(_index < 0){
            _index = (int32_t)store_to_list.itemlist_size() + _index;
            if(_index < 0) _index = (int32_t)store_to_list.itemlist_size();
        }
        if(_index >= store_to_list.itemlist_size()){
            return Status::NotFound("not found something by index!");
        }else{
            store_list::list::item item = store_to_list.itemlist(_index);
            if(pack){
                Slice resp_member(item.member());
                Slice resp_extra(item.extra());
                ret = pack->put_raw(STORE_RESP_MEMBER, resp_member);
                RETURN_IF_PACK_ERROR(ret);
                ret = pack->put_raw(STORE_RESP_EXTRA, resp_extra);
                RETURN_IF_PACK_ERROR(ret);
            }
        }
        return Status::OK();
    }

    //Lrange

    CommandLrange::CommandLrange (BinLog *binlog, Engine *engine) :
        Command(binlog, engine), _pos(0), _count(0){ }

    CommandLrange::~CommandLrange(){}

    Status CommandLrange::extract_params(const Request &req){
        int ret = req.pack()->get_str(STORE_REQ_KEY, &_key);
        RETURN_IF_PARAM_ERROR(STORE_REQ_KEY, ret);

        ret = req.pack()->get_int32(STORE_REQ_POS, &_pos);
        RETURN_IF_PARAM_ERROR(STORE_REQ_POS, ret);

        ret = req.pack()->get_int32(STORE_REQ_COUNT, &_count);
        RETURN_IF_PARAM_ERROR(STORE_REQ_COUNT, ret);
        return Status::OK();
    }

    Status CommandLrange::process(uint64_t binlog_id, Pack *pack){
        UNUSED(binlog_id);
        int ret;
        if(pack){
            ret = pack->put_str(STORE_REQ_KEY, _key);
            RETURN_IF_PACK_ERROR(ret);
        }
        SLOWLOG_PREPARE();
        std::string content_str;
        ret = _engine->get(_key, &content_str);
        SLOWLOG_CHECKPOINT1("get data frome disk or memory. key:%s", _key.data());
        RETURN_IF_ENGINE_ERROR("get", ret);
        RETURN_IF_NOTFOUND(_key, ret);
        Slice value_slice(content_str);
        ValueHeader* value_header = (ValueHeader*)value_slice.data();
        if(value_header->type != ValueHeader::LIST_PB_TYPE){
            log_warning("data type incorrect");
            return Status::UnsupportedType("data_type", "type incorrect");
        }
        value_slice.remove_prefix(sizeof(ValueHeader));
        content_str = value_slice.to_string();
        store_list::list store_to_list;
        store_to_list.ParseFromString(content_str);

        
        if(_count == 0){
            if(pack){
                Pack resp_pack;
                ret = pack->put_array(STORE_RESP_DATA, &resp_pack);
                RETURN_IF_PACK_ERROR(ret);
                for(int idx = 0; idx < store_to_list.itemlist_size(); idx++){
                    store_list::list::item item = store_to_list.itemlist(idx);
                    Pack sub_resp_pack;
                    ret = resp_pack.put_object_to_array(&sub_resp_pack);
                    RETURN_IF_PACK_ERROR(ret);
                    ret = sub_resp_pack.put_raw(STORE_RESP_MEMBER, item.member());
                    RETURN_IF_PACK_ERROR(ret);
                    ret = sub_resp_pack.put_raw(STORE_RESP_EXTRA, item.extra());
                    RETURN_IF_PACK_ERROR(ret);
                    sub_resp_pack.finish();
                }
            }
        }else{
            if(pack){
                Pack resp_pack;
                ret = pack->put_array(STORE_RESP_DATA, &resp_pack);
                RETURN_IF_PACK_ERROR(ret);
                if(_pos < 0){
                    if(_pos * -1 > store_to_list.itemlist_size())_pos = 0;
                    else _pos = store_to_list.itemlist_size() + _pos;
                }
                if(_pos < store_to_list.itemlist_size()){
                    for(int idx = 0; idx < (int32_t)_count && _pos + idx < (int32_t)store_to_list.itemlist_size(); idx++ ){
                        store_list::list::item item = store_to_list.itemlist(idx + _pos);
                        Pack sub_resp_pack;
                        ret = resp_pack.put_object_to_array(&sub_resp_pack);
                        RETURN_IF_PACK_ERROR(ret);
                        ret = sub_resp_pack.put_raw(STORE_RESP_MEMBER, item.member());
                        RETURN_IF_PACK_ERROR(ret);
                        ret = sub_resp_pack.put_raw(STORE_RESP_EXTRA, item.extra());
                        RETURN_IF_PACK_ERROR(ret);
                        sub_resp_pack.finish();
                    }
                }
            }
        }

        return Status::OK();
    }

}
