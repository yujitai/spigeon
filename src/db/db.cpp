#include "db/db.h"

#include "binlog/binlog.h"
#include "command/command.h"
#include "db/db_define.h"
#include "inc/env.h"
#include "db/response.h"
#include "db/request.h"
#include "util/log.h"
#include "util/nshead.h"
#include "util/slice.h"
#include "util/stat.h"
#include "util/store_define.h"
#include "util/config_file.h"
#include "util/scoped_ptr.h"
#include "util/status.h"
#include "engine/engine.h"
#include "util/utils.h"

namespace store {
static int set_readonly(int argc, char **argv, command_t *cmd, void *conf) {
    UNUSED(argv);
    UNUSED(cmd);
    if (argc < 2) return CONFIG_ERROR;

    DBOptions *options = (DBOptions*)conf;
    options->read_only = true;
    return CONFIG_OK;
}

static command_t db_cmd_table[] = {
    { "max_response_buffer_size",
      conf_set_mem_slot,
      offsetof(DBOptions, response_buf_size)
    },

    { "slaveof",
      set_readonly,
      0
    },

    { "slowlog_threshold",
      conf_set_usec_slot,
      offsetof(DBOptions, slowlog_threshold)
    },

    { "cmd_slowlog_threshold",
      conf_set_usec_slot,
      offsetof(DBOptions, cmd_slowlog_threshold)
    },

    null_command
};


DB::DB(BinLog *binlog, Engine *engine)
        : _binlog(binlog), _engine(engine), _cmd_factory(NULL){
    _cmd_factory = new CommandFactory(_binlog, _engine);
}

DB::~DB() {
    delete _cmd_factory;
}

int DB::init_conf() {
    _options = (DBOptions){ 10 * 1024 * 1024,
                            false,
                            1000
    };
    return DB_OK;
}

int DB::load_conf(const char *filename) {
    if (load_conf_file(filename, db_cmd_table, &_options) != CONFIG_OK) {
        log_fatal("failed to load config file for db module");
        return DB_ERROR;
    } else {
        fprintf(stderr,
                "DB Options:\n"
                "response_buffer_size: %lld\n"
                "read_only: %d\n"
                "slowlog_threshold: %lldus\n"
                "cmd_slowlog_threshold: %lldus\n"
                "\n",
                _options.response_buf_size,
                _options.read_only,
                _options.slowlog_threshold,
                _options.cmd_slowlog_threshold
                );
        return DB_OK;
    }            
}

int DB::validate_conf() {
    return DB_OK;
}

#define RETURN_IF_PARAM_ERROR(param, ret)                               \
    if ((ret)) {                                                        \
        log_warning("failed to get %s from request, ret:%d",            \
                    (param), (ret));                                    \
        return Status::BadRequest(param,                                \
                                  "not exists or type incorrect");      \
    }

#define RETURN_IF_NO_SUCH_METHOD(cmd, name)                             \
        if (!(cmd).get()) {                                             \
            log_warning("method %s not exist", name.data());            \
            return Status::UnknownMethod(name.data(), "unknown method"); \
        }

#define RETURN_IF_NO_PERMISSION(cmd)                       \
    if (_options.read_only && (cmd)->is_write()) {         \
        log_warning("try to modify a read-only database"); \
        return Status::Forbidden("read only database");    \
    }

#define RETURN_IF_PACK_ERROR(ret)                          \
    if ((ret)) {                                           \
        log_warning("failed to pack, ret:%d", ret);        \
        return Status::EntityTooLarge("packing failed");    \
    }

#define RETURN_IF_RESPONSE_ERROR(ret)                   \
    if ((ret) != RESPONSE_OK) {                         \
        log_fatal("fail to finish response");           \
        return Status::InternalError("response error"); \
    } 

#define RETURN_IF_REQUEST_PARSE_ERROR(request)                  \
    if (!request.get()) {                                   \
        log_warning("fail request parsing");                \
        return Status::BadRequest("request parsing error");  \
    }


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

#define SLOWLOG_CHECKPOINT3(format, args...)                    \
    long long checkpoint3 = ustime();                           \
    if ((checkpoint3 - checkpoint2) > threshold) {              \
        log_warning("SLOWLOG: %lldus "format,                   \
                    (checkpoint3 - checkpoint2), ##args);       \
    }

Status DB::process_sub_request(const Request& request, Pack* pack) {
    ASSERT(pack != NULL);
    Slice method = request.method();
   
    scoped_ptr<Command> cmd(_cmd_factory->create(method));
    cmd->_slowlog_threshold = _options.slowlog_threshold;
    RETURN_IF_NO_SUCH_METHOD(cmd, method);
    RETURN_IF_NO_PERMISSION(cmd);

    Status s = cmd->extract_params(request);
    if (!s.ok()) return s;

    stat_incr("cmds_" + method.to_string());
    stat_incr("cmds_all");

    binlog_id_seq_t seq;
    if (cmd->is_write()) {
        {
            SLOWLOG_PREPARE(_options.slowlog_threshold);
            KeyLockHolder lock_holder(&_key_lock, cmd->get_key());
            SLOWLOG_CHECKPOINT1("get lock");
            
            s = write_binlog(request, &seq);
            if (s.isInternalError()) return s;
            SLOWLOG_CHECKPOINT2("write binlog");
                        
            s = cmd->process(seq.cur, pack);
            if (s.isInternalError()) return s;
            SLOWLOG_CHECKPOINT3("command: %s", method.data());            
        }
        // if other error, will mark_request_done and return error
        mark_request_done(seq);
        stat_set_max("max_binlog_id", seq.cur);
    } else {
        SLOWLOG_PREPARE(_options.slowlog_threshold);
        s = cmd->process(seq.cur, pack);
        SLOWLOG_CHECKPOINT1("command: %s", method.data());
    }
    
    return s;
}

Status DB::write_binlog(const Request& req, binlog_id_seq_t* seq) {
    BinLogWriter* binlog_w = _binlog->writer();
    if (binlog_w == NULL) {
        log_fatal("get writer failed!");
        return Status::InternalError("fail to get binlog writer");
    }
    int ret = binlog_w->append(req.raw().data(), req.raw().size(), seq);
    if (ret != 0) {
        log_fatal("append binlog failed! ret[%d]", ret);
        return Status::InternalError("fail to write binlog");
    }
    log_debug("request write binlog prev[0x%lX] cur[0x%lX] next[0x%lX]", seq->prev, seq->cur, seq->next);
    return Status::OK();
}

Status DB::mark_request_done(const binlog_id_seq_t& seq) {
    BinLogWriter* binlog_w = _binlog->writer();
    if (binlog_w == NULL) {
        log_fatal("get writer failed!");
        return Status::InternalError("fail to get binlog writer");
    }
    binlog_w->active_binlog_mark_done(seq);
    return Status::OK();
}

Status DB::dispatch_request(const Request& request, Response* resp) {
    ASSERT(resp != NULL);
    int ret;
    // init response
    Pack resp_pack;
    ret = resp->pack()->put_array(STORE_RESP_DATA, &resp_pack);
    RETURN_IF_PACK_ERROR(ret);

    // get requests
    Pack req_pack;
    size_t arr_size;
    ret = request.pack()->get_array(STORE_REQ_BATCH, &req_pack, &arr_size);
    RETURN_IF_PARAM_ERROR(STORE_REQ_BATCH, ret);

    log_data_t* ld = log_get_thread_log_data();
    log_data_push_uint(ld, "batch", arr_size);

    for (size_t i = 0; i < arr_size; ++i) {
        // init sub request
        Pack sub_req_pack;
        ret = req_pack.get_object_arr(i, &sub_req_pack);

        RETURN_IF_PACK_ERROR(ret);
            
        scoped_ptr<Request> sub_req(Request::from_pack(&sub_req_pack,
                                                       request.log_id()));
        RETURN_IF_REQUEST_PARSE_ERROR(sub_req);
        // init sub response
        Pack sub_resp_pack;
        ret = resp_pack.put_object_to_array(&sub_resp_pack);
        RETURN_IF_PACK_ERROR(ret);

        // process sub request
        Status s = process_sub_request(*sub_req, &sub_resp_pack);
        if (i == 0) { // only print the first request
            log_data_push_str(ld, "method0", sub_req->method().data());
            log_data_push_str(ld, "key0", sub_req->key().data());
        }

        if (s.isInternalError()) return s;

        // put err_no/err_msg into sub response
        ret = set_error_msg(&sub_resp_pack, s);
        RETURN_IF_PACK_ERROR(ret);
        sub_resp_pack.finish();
    }
    resp_pack.finish();
    return Status::OK();
}


int DB::set_error_msg(Pack *pack, const Status& status) {
    int ret = pack->put_int32(STORE_RESP_ERRNO, status.code());
    if (ret != 0) {
        log_warning("mcpack put err_no failed!");
        return DB_PACK_ERROR;
    }
    ret = pack->put_str(STORE_RESP_ERRMSG, status.toString());
    if (ret != 0) {
        log_warning("mcpack put err_msg failed!");
        return DB_PACK_ERROR;
    }
    return DB_OK;
    
}

Status DB::process_request(const Request& request, Slice* resp) {
    int ret;
    stat_incr("total_cmd_request");
    log_data_t* ld = log_get_thread_log_data();
    log_data_push_uint(ld, "log_id", request.log_id());
    log_data_push_uint(ld, "req_size", request.pack()->size());
    
    Response result(_options.response_buf_size);
    ret = result.init();
    RETURN_IF_RESPONSE_ERROR(ret);

    Status s = dispatch_request(request, &result);

    if (s.isInternalError()) return s;

    ret = set_error_msg(result.pack(), s);
    RETURN_IF_PACK_ERROR(ret);
    
    ret = result.finish();
    RETURN_IF_RESPONSE_ERROR(ret);

    *resp = Slice(result.data(), result.size());
    return Status::OK();
}

#define MAX_ERROR_RESPONSE_SIZE 512
Status DB::build_error_response(const Status& status, Slice *resp) {
    Response result(MAX_ERROR_RESPONSE_SIZE);
    int ret = result.init();
    RETURN_IF_RESPONSE_ERROR(ret);
    ret = set_error_msg(result.pack(), status);
    RETURN_IF_PACK_ERROR(ret);
    ret = result.finish();
    RETURN_IF_RESPONSE_ERROR(ret);

    *resp = Slice(result.data(), result.size());
    return Status::OK();
}

Status DB::process_monitor_request(Slice *resp) {
    // just return a empty response
    Response result(sizeof(nshead_t));
    int ret = result.init();
    RETURN_IF_RESPONSE_ERROR(ret);
    ret = result.finish();
    RETURN_IF_RESPONSE_ERROR(ret);
    *resp = Slice(result.data(), result.size());
    return Status::OK();
}

Status DB::process_binlog(const Slice& data, uint64_t binlog_id) {
    scoped_ptr<Request> request(Request::from_raw(data, 0));
    RETURN_IF_REQUEST_PARSE_ERROR(request);

    scoped_ptr<Command> cmd(_cmd_factory->create(request->method()));
    cmd->_slowlog_threshold = _options.slowlog_threshold;//放入析构函数比较复杂，直接进行赋值处理
    RETURN_IF_NO_SUCH_METHOD(cmd, request->method());


    Status s = cmd->extract_params(*request);
    if (!s.ok()){
        log_warning("cmd extract_params fail command: %s key:%s! binlog_id=%lu", request->method().data(),request->key().data(),binlog_id);
        return s;
    }
    SLOWLOG_PREPARE(_options.slowlog_threshold);
    s = cmd->process(binlog_id, NULL);
    if(!s.ok()){
        log_warning("cmd process fail! command: %s key:%s! binlog_id=%lu",request->method().data(),request->key().data(),binlog_id);
    }
    SLOWLOG_CHECKPOINT1("command: %s key:%s", request->method().data(),request->key().data());
    return s;
}

Status DB::process_idempotent_request(const Request &request, uint64_t binlog_id) {        
    Slice method = request.method();
    scoped_ptr<Command> cmd(_cmd_factory->create(method));
    cmd->_slowlog_threshold = _options.slowlog_threshold;
    RETURN_IF_NO_SUCH_METHOD(cmd, method);

    Status s = cmd->extract_params(request);
    if (s.isInternalError()) return s;

    std::string value;
    int ret = _engine->get(cmd->get_key(), &value);
    if (ret == ENGINE_ERROR) {
        return Status::InternalError("");
    } else if (ret == ENGINE_NOTFOUND) {
        s = cmd->process(binlog_id, NULL);        
    } else {
        ValueHeader *hdr = (ValueHeader*)value.data();
        if (hdr->binlog_id < binlog_id) {
            s = cmd->process(binlog_id, NULL);
        }
    }
    if (s.isInternalError()) return s;
    stat_set_max("max_binlog_id", binlog_id);
    return s;
}

}  // namespace store
