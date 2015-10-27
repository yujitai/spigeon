#include <stdio.h>
#include <unistd.h>
#include "util/log.h"
#include "gtest/gtest.h"
#include "db/db_wrapper.h"

using namespace store;

/**
 * @brief 测试Log库
 */
class test_DBWrapper_suite : public ::testing::Test {
protected:
    test_DBWrapper_suite() {
        _db = new DBWrapper();
    }

    virtual ~test_DBWrapper_suite() {
        delete _db;
        _db = NULL;
    }

    virtual void SetUp() { }

    virtual void TearDown() { }

    DBWrapper* _db;
};

TEST_F(test_DBWrapper_suite, test_all_methods) {
    int key_num = 10;
    const int key_max_len = 1024;
    const int value_max_len = 1024;
    char tmp_key[key_max_len];
    char* tmp_value = (char*)malloc(value_max_len);
    for (int i = 0; i < key_num; ++i) {
        snprintf(tmp_key, key_max_len, "key_%d", i);
        std::string key(tmp_key);
        snprintf(tmp_value, value_max_len, "value_%d", i);
        std::string value(tmp_value);

        // set key 
        ASSERT_EQ(0, _db->set(key.c_str(), key.size(), value.c_str(), value.size()));

        // get key
        int tmp_value_len = 0;
        tmp_value[0] = 0;
        ASSERT_EQ(0, _db->get(key.c_str(), key.size(), tmp_value, tmp_value_len));
        ASSERT_STREQ(value.c_str(), tmp_value);
        ASSERT_EQ((int)value.size(), tmp_value_len);

        // del key
        ASSERT_EQ(0, _db->del(key.c_str(), key.size()));
        ASSERT_EQ(0, _db->del(key.c_str(), key.size()));
        
        // get key
        tmp_value_len = 0;
        tmp_value[0] = 0;
        ASSERT_EQ(0, _db->get(key.c_str(), key.size(), tmp_value, tmp_value_len));
        ASSERT_EQ(0, tmp_value_len);
        ASSERT_TRUE('\0' == tmp_value[0]);
    }
    free(tmp_value);
    tmp_value = NULL;
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

