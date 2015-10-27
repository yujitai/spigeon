#include "replication/replication.h"

#include "binlog/binlog.h"
#include "db/db.h"
#include "db/db_define.h"
#include "inc/env.h"
#include "db/response.h"
#include "server/event.h"
#include "db/request.h"
#include "util/pack.h"
#include "util/nshead.h"
#include "util/stat.h"
#include "util/store_define.h"
#include "util/zmalloc.h"
#include "util/config_file.h"
#include "util/scoped_ptr.h"
#include "util/utils.h"

namespace store {

#define MAX_MESSAGE_SIZE 512

struct SyncMessage {
    uint64_t next_binlog_id;
    uint64_t last_binlog_id;
    uint32_t checksum;
};

struct BinLogMessage {
    uint64_t binlog_id;
    uint64_t timestamp;
    uint32_t checksum;
    Slice data;
};

static command_t repl_cmd_table[] = {
    { "slowlog_threshold",
      conf_set_usec_slot,
      offsetof(ReplOptions, slowlog_threshold)
    },
    null_command
};

Replication::Replication(DB* db, BinLog *binlog)
        : _db(db), _binlog(binlog) { }

Replication::~Replication() { }

int Replication::init_conf() {
    _options = (ReplOptions){ 1000 };
    return REPLICATION_OK;
}

int Replication::load_conf(const char *filename) {
    if (load_conf_file(filename, repl_cmd_table, &_options) != CONFIG_OK) {
        log_fatal("failed to load config file for replication module");
        return REPLICATION_ERROR;
    } else {
        fprintf(stderr,
                "Replication Options:\n"
                "slowlog_threshold: %lldus\n"
                "\n",
                _options.slowlog_threshold
                );
        return REPLICATION_OK;
    }
}

int Replication::validate_conf() {
    return REPLICATION_OK;
}

#define RETURN_IF_PACK_ERROR(ret)               \
    if ((ret)) {                                \
        log_fatal("mcpack error");              \
        return REPLICATION_MCPACK_ERROR;        \
    }

#define RETURN_IF_BINLOG_ERROR(ret)             \
    if ((ret)) {                                \
        log_fatal("binlog error");            \
        return REPLICATION_BINLOG_ERROR;        \
    }

#define RETURN_IF_DB_ERROR(s)                   \
    if (!s.ok()) {                              \
        log_fatal("binlog db error");           \
        return REPLICATION_DB_PROCESS_ERROR;    \
    }

#define RETURN_IF_ERROR(ret)                    \
    if ((ret) != REPLICATION_OK) {              \
        return ret;                             \
    }

#define RETURN_IF_RESPONSE_ERROR(ret)           \
    if ((ret) != RESPONSE_OK) {                 \
        log_fatal("response error");            \
        return REPLICATION_ERROR;               \
    }

#define RETURN_IF_REQUEST_PARSE_ERROR(request)  \
    if (!request.get()) {                       \
        log_fatal("fail to parse request");     \
        return REPLICATION_ERROR;               \
    }

int Replication::get_binlog(RepStatus *status, Slice *log) {
    if (status->reader == NULL) {
        return REPLICATION_BINLOG_ERROR;
    }
    
    BinLogReader *reader = status->reader;
    if (!reader->next_sendable()) {
        return REPLICATION_BINLOG_NOT_AVAIL;
    }
    int ret = reader->read_next();
    RETURN_IF_ERROR(ret);

    BinLogMessage msg;
    msg.binlog_id = reader->head().binlog_id;
    msg.timestamp = reader->head().timestamp_us;
    msg.checksum = reader->head().checksum;
    msg.data = Slice(reader->data(), reader->data_len());
    
    ret = build_binlog_message(msg, reader->total_len(), log);
    RETURN_IF_ERROR(ret);
    stat_set_max("last_rep_time", EventLoop::current_time());        
    return REPLICATION_OK;
}

int Replication::build_binlog_message(const BinLogMessage& msg,
                                      size_t len,
                                      Slice *log) {
    Response resp(len + 1024);          // FIX me..
    int ret = resp.init();
    RETURN_IF_RESPONSE_ERROR(ret);

    Pack *pack = resp.pack();
    ret = pack->put_str(STORE_REQ_METHOD, Slice(STORE_REPL_DATA)) ||
            pack->put_uint64(STORE_REQ_BINLOG_ID, msg.binlog_id) ||
            pack->put_uint64(STORE_REQ_TIMESTAMP, msg.timestamp) ||
            pack->put_uint32(STORE_REQ_CHECKSUM, msg.checksum) ||
            pack->put_raw(STORE_REQ_DATA, msg.data);
    RETURN_IF_PACK_ERROR(ret);

    ret = resp.finish();
    RETURN_IF_RESPONSE_ERROR(ret);

    *log = Slice(resp.data(), resp.size());
    return REPLICATION_OK;
}

int Replication::build_sync(Slice *sync) {
    int ret;
    SyncMessage msg;
    msg.next_binlog_id = _binlog->next_binlog_id();
    if (msg.next_binlog_id) {
        ret = get_last_binlog(&(msg.last_binlog_id), &(msg.checksum));
        RETURN_IF_ERROR(ret);
    }
    ret = build_sync_message(msg, sync);
    return ret;
}

int Replication::get_last_binlog(uint64_t *log_id, uint32_t *checksum) {
    ASSERT(sync != NULL);
    int ret;
    uint64_t last_binlog_id = _binlog->last_binlog_id();
    BinLogReaderHolder reader(_binlog, _binlog->fetch_reader());
    if (!reader.get()) {
        log_warning("binlog fetch reader failed");
        return REPLICATION_BINLOG_ERROR;
    }

    ret = reader->read(last_binlog_id);
    RETURN_IF_BINLOG_ERROR(ret);

    *log_id = last_binlog_id;
    *checksum = reader->head().checksum;
    return REPLICATION_OK;
}

int Replication::build_sync_message(const SyncMessage& msg, Slice *sync) {
    int ret;
    Response resp(MAX_MESSAGE_SIZE);                 // FIX me please...
    ret = resp.init();
    RETURN_IF_RESPONSE_ERROR(ret);
    
    Pack *pack = resp.pack();

    ret = pack->put_str(STORE_REQ_METHOD, Slice(STORE_COMMAND_SYNC)) ||
            pack->put_uint64("next_binlog_id", msg.next_binlog_id) ||
            pack->put_uint64("last_binlog_id", msg.last_binlog_id) ||
            pack->put_uint32(STORE_REQ_CHECKSUM, msg.checksum);
    RETURN_IF_PACK_ERROR(ret);
    
    ret = resp.finish();
    RETURN_IF_RESPONSE_ERROR(ret);
    
    *sync = Slice(resp.data(), resp.size());
    return REPLICATION_OK;
}

int Replication::parse_sync_message(const Request& req, SyncMessage *msg) {
    Pack *pack = req.pack();
    int ret = pack->get_uint64("next_binlog_id", &(msg->next_binlog_id)) ||
            pack->get_uint64("last_binlog_id", &(msg->last_binlog_id)) ||
            pack->get_uint32(STORE_REQ_CHECKSUM, &(msg->checksum));
    RETURN_IF_PACK_ERROR(ret);
    return REPLICATION_OK;
}

int Replication::process_sync(const Request& req, RepStatus *status) {
    stat_incr("total_sync_request");
    SyncMessage msg;
    int ret = parse_sync_message(req, &msg);
    RETURN_IF_ERROR(ret);
    
    if (msg.next_binlog_id < _binlog->first_binlog_id()) {
        log_warning("slave data too old, need to manually resync");
        return REPLICATION_SLAVE_DATA_TOO_OLD;
    }

    status->binlog = _binlog;
    status->reader = _binlog->fetch_reader();
    if (status->reader == NULL) {
        return REPLICATION_BINLOG_ERROR;
    }
    
    if (msg.next_binlog_id > 0) {
        // we need to validate the last binlog record from the slave
        ret = status->reader->read(msg.last_binlog_id);
        RETURN_IF_BINLOG_ERROR(ret);

        if (status->reader->head().checksum != msg.checksum) {
            log_warning("fail to validate checksum");
            return REPLICATION_BINLOG_ERROR;
        }
    }
    return REPLICATION_OK;
}

int Replication::build_ping(Slice *ping) {
    int ret;
    Response resp(MAX_MESSAGE_SIZE);
    ret = resp.init();
    RETURN_IF_RESPONSE_ERROR(ret);

    Pack *pack = resp.pack();
    ret = pack->put_str(STORE_REQ_METHOD, Slice(STORE_REPL_PING));
    RETURN_IF_PACK_ERROR(ret);

    ret = resp.finish();
    RETURN_IF_RESPONSE_ERROR(ret);

    *ping = Slice(resp.data(), resp.size());
    return REPLICATION_OK;
}

int Replication::build_handshake_msg(Slice *msg) {
    int ret;
    Response resp(MAX_MESSAGE_SIZE);
    ret = resp.init();
    RETURN_IF_RESPONSE_ERROR(ret);

    Pack *pack = resp.pack();
    ret = pack->put_str(STORE_REQ_METHOD, Slice(STORE_REPL_HANDSHAKE));
    RETURN_IF_PACK_ERROR(ret);

    ret = resp.finish();
    RETURN_IF_RESPONSE_ERROR(ret);

    *msg = Slice(resp.data(), resp.size());
    return REPLICATION_OK;
}

#define RETURN_IF_WRITER_NOT_EXISTS(writer)     \
    if (!(writer)) {                            \
        log_fatal("can not get binlog writer"); \
        return REPLICATION_BINLOG_ERROR;        \
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

int Replication::process_binlog(const Request& request) {
    BinLogMessage msg;
    int ret = parse_binlog_message(request, &msg);
    RETURN_IF_ERROR(ret);
    uint64_t delay = 0;
    if (_binlog->current_time() > msg.timestamp)
        delay = _binlog->current_time() - msg.timestamp;
    
    stat_set("current_binlog_delay", delay);
    stat_set_max("max_binlog_delay", delay);
        
    BinLogWriter *writer = _binlog->writer();
    RETURN_IF_WRITER_NOT_EXISTS(writer);

    SLOWLOG_PREPARE(_options.slowlog_threshold);
    binlog_id_seq_t seq;
    ret = writer->write(msg.binlog_id, msg.timestamp,
                        msg.data.data(), msg.data.size(),
                        &seq);
    RETURN_IF_BINLOG_ERROR(ret);
    SLOWLOG_CHECKPOINT1("write binlog");
        
    Status s = _db->process_binlog(msg.data, msg.binlog_id);
    if(s.isInternalError()){
        RETURN_IF_DB_ERROR(s);
        return REPLICATION_INTERNAL_ERROR;
    }
    writer->active_binlog_mark_done(seq);
    stat_set_max("max_binlog_id", msg.binlog_id);

    return REPLICATION_OK;
}

int Replication::parse_binlog_message(const Request& request, BinLogMessage *msg) {
    Pack *pack = request.pack();
    int ret = pack->get_uint64(STORE_REQ_BINLOG_ID, &(msg->binlog_id)) ||
            pack->get_uint64(STORE_REQ_TIMESTAMP, &(msg->timestamp)) ||
            pack->get_uint32(STORE_REQ_CHECKSUM, &(msg->checksum))  ||
            pack->get_raw(STORE_REQ_DATA, &(msg->data));
    RETURN_IF_PACK_ERROR(ret);
    return REPLICATION_OK;
}

}  // namespace store
