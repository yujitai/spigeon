#include <stdio.h>
#include <unistd.h>
#include <string>
#include "util/slice.h"
#include "util/log.h"
#include "gtest/gtest.h"
#include "db/db.h"
#include "db/db_define.h"
#include "db/black_hole_engine.h"
#include "util/nshead.h"
#include "replication/replication.h"
#include "replication/recovery.h"
#include "mcpack_reader.h"
#include "mcpack_writer.h"
#include "event/request.h"
#include "event/event.h"
#include "util/config.h"
#include "binlog/binlog.h"
#include "inc/env.h"
#include "global.h"

using namespace store;

extern Env* env;

int main(int argc, char **argv) {
    ::testing::AddGlobalTestEnvironment(new StoreEnv());
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class test_Recovery_suite : public ::testing::Test {
    static const int MCPACK_BUF_MAX_LEN = 16 * 1024 * 1024;
protected:
    test_Recovery_suite() {
        _rep = env->replication();
        _recovery = env->recovery();
        _binlog = env->binlog();
        _rep_stat = new RepStatus();
        _pack_r = new McPackReader();
        _pack_w = new McPackWriter();
        _req_buf_len = MCPACK_BUF_MAX_LEN;
        _req_buf = (char*)zmalloc(MCPACK_BUF_MAX_LEN);
        _slice = new Slice();
        pack = NULL;
    }

    virtual ~test_Recovery_suite() {
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
    Recovery* _recovery;
    RepStatus* _rep_stat;
    Request* _req;
    Slice*   _slice;
    McPackReader* _pack_r;
    McPackWriter* _pack_w;
    char* _req_buf;
    int   _req_buf_len;
    char* _resp_buf;
    int   _resp_buf_len;
    mc_pack_t* pack;
};

TEST_F(test_Recovery_suite, build_sync) {
    ASSERT_EQ(0, _recovery->recovery());
}

