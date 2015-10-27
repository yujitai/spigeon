#ifndef __STORE_DB_UTIL_H_
#define __STORE_DB_UTIL_H_

#include <stdint.h>
#include "util/zmalloc.h"

namespace store {

#define STORE_REQ_METHOD             "method"
#define STORE_REQ_KEY                "key"
#define STORE_REQ_VALUE              "value"
#define STORE_REQ_SECONDS            "seconds"
#define STORE_REQ_BINLOG_ID          "binlog_id"
#define STORE_REQ_LAST_BINLOG_ID     "last_binlog_id"
#define STORE_REQ_PREV_BINLOG_ID     "prev_binlog_id"
#define STORE_REQ_TIMESTAMP          "timestamp"
#define STORE_REQ_CHECKSUM           "checksum"
#define STORE_REQ_DATA               "data"
#define STORE_REQ_BATCH              "reqs"
#define STORE_REQ_BATCH_METHOD_NAME  "batch"
#define STORE_REQ_EXTRA              "extra"
#define STORE_REQ_LENGTH             "length"
#define STORE_REQ_INDEX              "index"
#define STORE_REQ_COUNT              "count"
#define STORE_REQ_START              "start"
#define STORE_REQ_STOP               "stop"
#define STORE_REQ_MEMBER             "member"
#define STORE_REQ_POS                "pos"

#define STORE_RESP_ERRNO             "err_no"
#define STORE_RESP_ERRNO             "err_no"
#define STORE_RESP_ERRMSG            "err_msg"
#define STORE_RESP_DATA              "ret"
#define STORE_RESP_STAT              "stat"
#define STORE_RESP_VALUE             "value"
#define STORE_RESP_OK                "OK"
#define STORE_NSHEAD_PROVIDER        "store"
#define STORE_RESP_SETMSG            "set_msg"
#define STORE_RESP_POPMSG            "pop_msg"
#define STORE_RESP_INDMSG            "ind_msg"
#define STORE_RESP_NOLIST            "nolist"
#define STORE_RESP_LENGTH            "length"
#define STORE_RESP_MEMBER            "member"
#define STORE_RESP_EXTRA             "extra"
#define STORE_RESP_INDEX             "index"

struct ValueHeader {
    enum {
        RAW_TYPE = 0,
        INT64_TYPE = 1,
        LIST_PB_TYPE = 2
    };    
    uint64_t    binlog_id;
    char        type;
    char        reserved[3];
};

class BufferHolder {
public:
    BufferHolder(const char* buf) : _buf(buf) { }
    ~BufferHolder() { zfree((void*)_buf); _buf = NULL; }

private:
    const char* _buf;
};

}  // namespace store

#endif  // __STORE_DB_UTIL_H_
