#include "db/recovery.h"

#include <string>
#include "binlog/binlog.h"
#include "db/db.h"
#include "db/db_define.h"
#include "engine/engine.h"
#include "inc/env.h"
#include "db/request.h"
#include "util/stat.h"
#include "util/config_file.h"
#include "util/scoped_ptr.h"

namespace store {
Recovery::Recovery(DB* db, BinLog *binlog)
        : _db(db), _binlog(binlog){ }

Recovery::~Recovery() { }

int Recovery::init_conf() {
    return RECOVERY_OK;
}

int Recovery::load_conf(const char *) {
    return RECOVERY_OK;
}

int Recovery::validate_conf() {
    return RECOVERY_OK;
}

#define RETURN_IF_BINLOG_ERROR(ret)             \
    if ((ret)) {                                \
        log_fatal("fail to read binlog");       \
        return RECOVERY_BINLOG_ERROR;           \
    }

int Recovery::recover() {
    int ret;
    if (_binlog->next_binlog_id() == 0) {
        log_notice("no binlog need to recover from");
        return RECOVERY_OK;
    }
    BinLogReaderHolder reader(_binlog, _binlog->fetch_reader());    
    if (!reader.get()) {
        log_fatal("fail to fetch binlog reader");
        return RECOVERY_BINLOG_ERROR;
    }
    BinLogWriter *writer = _binlog->writer();
    if (!writer) {
        log_fatal("fail to get binlog writer");
        return RECOVERY_BINLOG_ERROR;
    }

    uint64_t cur_binlog_id = _binlog->checkpoint_binlog_id();
    uint64_t last_binlog_id = _binlog->last_binlog_id();
    // put cursor to the checkpoint
    ret = reader->read(cur_binlog_id);
    RETURN_IF_BINLOG_ERROR(ret);    
    while (reader->next_binlog_id() <= last_binlog_id) {
        ret = reader->read_next();
        RETURN_IF_BINLOG_ERROR(ret);
        
        cur_binlog_id = reader->cur_binlog_id();
        scoped_ptr<Request> request(Request::from_raw(Slice(reader->data(),
                                                            reader->data_len()),
                                                      /* no log_id */
                                                      0)); 
        Status s = _db->process_idempotent_request(*request, cur_binlog_id);
        if (s.isInternalError()) Env::suicide(); // ignore other errors
    }
    writer->dump_checkpoint(last_binlog_id);
    return RECOVERY_OK;    
}

}  // namespace store
