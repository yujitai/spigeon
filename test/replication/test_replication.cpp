#include <stdio.h>
#include <unistd.h>
#include <string>
#include "util/slice.h"
#include "util/log.h"
#include "gtest/gtest.h"
#include "db/black_hole_engine.h"
#include "db/db.h"
#include "db/db_define.h"
#include "inc/env.h"
#include "db/response.h"
#include "util/nshead.h"
#include "replication/replication.h"
#include "mcpack_reader.h"
#include "mcpack_writer.h"
#include "event/request.h"
#include "event/event.h"
#include "util/config.h"
#include "binlog/binlog.h"
#include "global.h"

using namespace store;

extern Env* env;

int main(int argc, char **argv) {
    ::testing::AddGlobalTestEnvironment(new StoreEnv());
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class test_Replication_suite : public ::testing::Test {
    static const int MCPACK_BUF_MAX_LEN = 16 * 1024 * 1024;
protected:
    test_Replication_suite() {
        _rep = env->replication();
        _binlog = env->binlog();
        _rep_stat = new RepStatus();
        _pack_r = new McPackReader();
        _pack_w = new McPackWriter();
        _req_buf_len = MCPACK_BUF_MAX_LEN;
        _req_buf = (char*)zmalloc(MCPACK_BUF_MAX_LEN);
        _slice = new Slice();
        pack = NULL;
    }

    virtual ~test_Replication_suite() {
        //delete _rep;
        //_rep = NULL;
        //delete _req;
        //_req = NULL;
        //delete _slice;
        //_slice = NULL;
        //delete _pack_r;
        //_pack_r = NULL;
        //delete _pack_w;
        //_pack_w = NULL;
    }

    virtual void SetUp() { }

    virtual void TearDown() { }

    BinLog* _binlog;
    Replication* _rep;
    RepStatus* _rep_stat;
    Request* _req;
    Pack _req_pack;
    Slice*   _slice;
    McPackReader* _pack_r;
    McPackWriter* _pack_w;
    char* _req_buf;
    int   _req_buf_len;
    char* _resp_buf;
    int   _resp_buf_len;
    mc_pack_t* pack;
};

TEST_F(test_Replication_suite, build_sync) {
    ASSERT_EQ(0, _rep->build_sync(_slice));
    OPEN_RESPONSE_MCPACK_FOR_READ(_pack_r, _slice);
    const char*  method;
    unsigned int method_len;
    const char* expected_method = "SYNC";
    ASSERT_EQ(0, _pack_r->get_str("method", method));
    ASSERT_TRUE(std::string(expected_method) == method);

    log_debug("first binlog_id [0x%lX] second binlog_id[0x%lX] last_binlog_id[0x%lX]", _binlog->first_binlog_id(), _binlog->next_binlog_id(), _binlog->last_binlog_id());
    log_debug("value[0x%lX]", (uint64_t)-1);
}

TEST_F(test_Replication_suite, build_ping) {
    ASSERT_EQ(0, _rep->build_ping(_slice));
    OPEN_RESPONSE_MCPACK_FOR_READ(_pack_r, _slice);
    const char*  method;
    unsigned int method_len;
    const char* expected_method = "PING";
    ASSERT_EQ(0, _pack_r->get_str("method", method));
    ASSERT_TRUE(std::string(expected_method) == method);
}

TEST_F(test_Replication_suite, process_sync_get_binlog) {
    ASSERT_EQ(0, _rep->build_sync(_slice));

    // test process_sync
    _req_buf_len = _slice->size();
    OPEN_RESPONSE_MCPACK_FOR_READ(_pack_r, _slice);
    Pack pack;
    ASSERT_EQ(0, pack.open(*_slice, Pack::READ));
    _req = new Request(&pack, *_slice, Slice("SYNC"), 0);
    ASSERT_EQ(0, _rep->process_sync(*_req, _rep_stat));
    log_debug("binlog_id seq prev[0x%lX] cur[0x%lX] next[0x%lX]", _rep_stat->seq.prev, _rep_stat->seq.cur, _rep_stat->seq.next);

    // test get_binlog
    ASSERT_EQ(REPLICATION_BINLOG_NOT_AVAIL, _rep->get_binlog(_rep_stat, _slice));

    _rep_stat->seq.cur = 0;
    ASSERT_EQ(0, _rep->get_binlog(_rep_stat, _slice));
    OPEN_RESPONSE_MCPACK_FOR_READ(_pack_r, _slice);
    uint64_t binlog_id;
    ASSERT_EQ(0, _pack_r->get_uint64("binlog_id", binlog_id));
    uint64_t prev_binlog_id;
    ASSERT_EQ(0, _pack_r->get_uint64("prev_binlog_id", prev_binlog_id));
    uint64_t timestamp;
    ASSERT_EQ(0, _pack_r->get_uint64("timestamp", timestamp));
    uint32_t checksum;
    ASSERT_EQ(0, _pack_r->get_uint32("checksum", checksum));
    const char* data;
    unsigned int data_len;
    ASSERT_EQ(0, _pack_r->get_raw("data", data, data_len));
    const char* method;
    ASSERT_EQ(0, _pack_r->get_str("method", method));
    log_debug("binlog_id[0x%lX] prev_binlog_id[0x%lX] timestamp[0x%lX] checksum[0x%X] data[%s] data_len[%u] method[%s]",
        binlog_id, prev_binlog_id, timestamp, checksum, data, data_len, method);
}

int construct_set_request(char* & data, unsigned int& data_len) {
    McPackWriter* data_pack = new McPackWriter();
    int buf_max_len = 2 * 1024;
    data = (char*)zmalloc(buf_max_len);
    //ASSERT_EQ(0, data_pack->open(data, buf_max_len));
    data_pack->open(data, buf_max_len);
    const char* method = "SET";
    const char* key = "test_key";
    const char* value = "test_value";
    //ASSERT_EQ(0, data_pack->put_str("method", method, strlen(method)));
    //ASSERT_EQ(0, data_pack->put_str("key", key, strlen(key)));
    //ASSERT_EQ(0, data_pack->put_raw("value", value, strlen(value)));
    data_pack->put_str("method", method);
    data_pack->put_str("key", key);
    data_pack->put_raw("value", value, strlen(value));
    data_pack->close();
    data_len = data_pack->get_raw_mc_pack_len();
    return 0;
}

TEST_F(test_Replication_suite, process_binlog) {
    // get_binlog
    ASSERT_EQ(0, _pack_w->open(_req_buf, _req_buf_len));
    uint64_t binlog_id = 0x8C;
    ASSERT_EQ(0, _pack_w->put_uint64("binlog_id", binlog_id));
    uint64_t prev_binlog_id = 0;
    ASSERT_EQ(0, _pack_w->put_uint64("prev_binlog_id", prev_binlog_id));
    uint64_t timestamp = 0;
    ASSERT_EQ(0, _pack_w->put_uint64("timestamp", timestamp));
    uint32_t checksum = 0;
    ASSERT_EQ(0, _pack_w->put_uint32("checksum", checksum));
    const char* method = "REPLICATE";
    ASSERT_EQ(0, _pack_w->put_str("method", method));
    char* data;
    unsigned int data_len;
    ASSERT_EQ(0, construct_set_request(data, data_len));
    ASSERT_EQ(0, _pack_w->put_raw("data", data, data_len));
    log_debug("binlog_id[0x%lX] prev_binlog_id[0x%lX] timestamp[0x%lX] checksum[0x%X] data[%s] data_len[%u]", binlog_id, prev_binlog_id, timestamp, checksum, data, data_len);

    // test process_binlog
    Slice log(_req_buf, _pack_w->get_raw_mc_pack_len());
    ASSERT_EQ(0, _rep->process_binlog(log));
}

