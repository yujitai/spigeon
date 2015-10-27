#include <stdio.h>
#include <unistd.h>
#include <string>
#include "util/slice.h"
#include "util/log.h"
#include "gtest/gtest.h"
#include "db/db.h"
#include "db/db_define.h"
#include "util/nshead.h"
#include "util/config.h"
#include "mcpack_reader.h"
#include "mcpack_writer.h"
#include "event/request.h"
#include "event/event.h"
#include "engine/leveldb.h"
#include "db/black_hole_engine.h"
#include "inc/env.h"
#include "global.h"

using namespace store;

extern Env* env;

int main(int argc, char **argv) {
    ::testing::AddGlobalTestEnvironment(new StoreEnv());
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

#define OPEN_REQUEST_MCPACK_FOR_READ do {                    \
        ASSERT_EQ(0, _req_pack.open(Slice(_req_buf, _req_buf_len), Pack::READ)); \
        _req->raw = Slice(_req_buf, _req_buf_len);           \
        _req->pack = &_req_pack;                 \
    } while (0)

#define OPEN_RESPONSE_MCPACK_FOR_READ do {       \
        _slice->remove_prefix(sizeof(nshead_t)); \
        ASSERT_EQ(0, _pack_r->open(_slice->data(), _slice->size())); \
    } while (0)

class test_DB_suite : public ::testing::Test {
    static const int MCPACK_BUF_MAX_LEN = 16 * 1024 * 1024;
protected:
    test_DB_suite() {
        //LeveldbConfig *conf = new LeveldbConfig;
        //Engine* engine = new store::Leveldb(conf);
        _db = env->db();
        _pack_r = new McPackReader();
        _pack_w = new McPackWriter();
        _req_buf_len = MCPACK_BUF_MAX_LEN;
        _req_buf = (char*)zmalloc(MCPACK_BUF_MAX_LEN);
        _req = new Request();
        _slice = new Slice();
        pack = NULL;
    }

    virtual ~test_DB_suite() {
        //delete _db;
        //_db = NULL;
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

    DB* _db;
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

TEST_F(test_DB_suite, method_not_exit) {
    ASSERT_EQ(0, _pack_w->open(_req_buf, _req_buf_len));
    std::string key("key_get");
    std::string method("UNKNOWN_METHOD");
    _req->method = method;
    //_pack_r->close();
    ASSERT_EQ(0, _pack_w->put_str("key", key.c_str()));

    OPEN_REQUEST_MCPACK_FOR_READ;
    ASSERT_EQ(0, _db->process_request(*_req, _slice));

    OPEN_RESPONSE_MCPACK_FOR_READ;
    int err_no;
    ASSERT_EQ(0, _pack_r->get_int32("err_no", err_no));
    ASSERT_EQ(DB_METHOD_NOT_EXIST, err_no);
}

TEST_F(test_DB_suite, get) {
    ASSERT_EQ(0, _pack_w->open(_req_buf, _req_buf_len));
    std::string key("key_get__________________");
    std::string method("GET");
    _req->method = method;
    //_pack_r->close();
    ASSERT_EQ(0, _pack_w->put_str("key", key.c_str()));

    OPEN_REQUEST_MCPACK_FOR_READ;
    EXPECT_EQ(0, _db->process_request(*_req, _slice));

    OPEN_RESPONSE_MCPACK_FOR_READ;
    int err_no;
    ASSERT_EQ(0, _pack_r->get_int32("err_no", err_no));
    ASSERT_EQ(0, err_no);
}

TEST_F(test_DB_suite, set_param_error) {
    std::string key("key_set");
    std::string value("value_set");
    std::string method("SET");
    _req->method = method;

    ASSERT_EQ(0, _pack_w->open(_req_buf, _req_buf_len));
    ASSERT_EQ(0, _pack_w->put_str("key", key.c_str()));

    OPEN_REQUEST_MCPACK_FOR_READ;
    EXPECT_EQ(0, _db->process_request(*_req, _slice));

    OPEN_RESPONSE_MCPACK_FOR_READ;
    int err_no;
    ASSERT_EQ(0, _pack_r->get_int32("err_no", err_no));
    ASSERT_EQ(DB_REQ_PARAM_ERROR, err_no);
}

TEST_F(test_DB_suite, set) {
    std::string key("key_set");
    std::string value("value_set");
    std::string method("SET");
    _req->method = method;

    ASSERT_EQ(0, _pack_w->open(_req_buf, _req_buf_len));
    ASSERT_EQ(0, _pack_w->put_str("key", key.c_str()));
    ASSERT_EQ(0, _pack_w->put_raw("value", value.c_str(), value.size()));

    OPEN_REQUEST_MCPACK_FOR_READ;
    EXPECT_EQ(0, _db->process_request(*_req, _slice));

    OPEN_RESPONSE_MCPACK_FOR_READ;
    int err_no;
    ASSERT_EQ(0, _pack_r->get_int32("err_no", err_no));
    ASSERT_EQ(0, err_no);
}

TEST_F(test_DB_suite, del) {
    std::string key("key_del");
    std::string method("DEL");
    _req->method = method;

    ASSERT_EQ(0, _pack_w->open(_req_buf, _req_buf_len));
    ASSERT_EQ(0, _pack_w->put_str("key", key.c_str()));

    OPEN_REQUEST_MCPACK_FOR_READ;
    EXPECT_EQ(0, _db->process_request(*_req, _slice));

    OPEN_RESPONSE_MCPACK_FOR_READ;
    int err_no;
    ASSERT_EQ(0, _pack_r->get_int32("err_no", err_no));
    ASSERT_EQ(0, err_no);
}


