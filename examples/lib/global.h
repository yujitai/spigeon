#ifndef __STORE_TEST_GLOBAL_H_
#define __STORE_TEST_GLOBAL_H_
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
#include "mcpack_reader.h"
#include "mcpack_writer.h"
#include "event/request.h"
#include "event/event.h"
#include "util/config.h"
#include "binlog/binlog.h"

using namespace store;

#define OPEN_REQUEST_MCPACK_FOR_READ do {          \
        ASSERT_EQ(0, _req_pack.open(Slice(_req_buf, _req_buf_len), Pack::READ)); \
        _req->raw = Slice(_req_buf, _req_buf_len); \
        _req->pack = &_req_pack;                   \
    } while (0)

#define OPEN_RESPONSE_MCPACK_FOR_READ(_pack_r, _slice) do {          \
        _slice->remove_prefix(sizeof(nshead_t));                     \
        ASSERT_EQ(0, _pack_r->open(_slice->data(), _slice->size())); \
    } while (0)

Env* env = NULL;

class StoreEnv : public ::testing::Environment {
    virtual void SetUp() {
        Engine* engine = new BlackHoleEngine();
        env = new Env(engine);
        if (env->init(".", "conf/server.conf") == ENV_ERROR) {
            fprintf(stderr, "env init failed\n");
            exit(-1);
        }
    }
    virtual void TearDown() {
    }
};

#endif  // __STORE_TEST_GLOBAL_H_
