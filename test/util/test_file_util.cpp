/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/

#include <gtest/gtest.h>

#include "util/file.h"
#include "db/black_hole_engine.h"

using namespace store;

int main(int argc, char **argv) try
{
    int ret = 0;

	// test
    testing::InitGoogleTest(&argc, argv);
    ret = RUN_ALL_TESTS();

	return ret;
} catch (std::exception & ex) {
    printf("std::exception: [what:%s]\n", ex.what());
} catch (...) {
    printf("ooooops. unknown exception.\n");
}

/**
 * @brief 
 **/
class test_FileUtil : public ::testing::Test{
    protected:
        test_FileUtil() {
        };
        virtual ~test_FileUtil(){
        };
        virtual void SetUp() {
            //Called befor every TEST_F(test_FileUtil, *)
        };
        virtual void TearDown() {
            //Called after every TEST_F(test_FileUtil, *)
            //-- stop others
        };

        void test_dirname(const char *path, const char *expect, bool reserve_last_slash = false) {
            EXPECT_STREQ(expect, FileUtil::dirname(path, reserve_last_slash).c_str());
        }

        void test_basename(const char *path, const char *expect) {
            EXPECT_STREQ(expect, FileUtil::basename(path).c_str());
        }
};

// --------------------------------------
// FileUtil::dirname
// --------------------------------------

TEST_F(test_FileUtil, case_dirname_1)
{
    test_dirname("/", "/");
    test_dirname("///", "/");

    test_dirname("/abc", "/");
    test_dirname("/.abc", "/");

    test_dirname("/a/b/c", "/a/b");
    test_dirname("/a/b/c", "/a/b/", true);

    test_dirname("/a/b/c/", "/a/b");
    test_dirname("/a/b/c/", "/a/b/", true);

    test_dirname("abc", ".");
    test_dirname("abc", "./", true);

    test_dirname("./abc", ".");
    test_dirname("./abc", "./", true);
}

TEST_F(test_FileUtil, case_basename_1)
{
    test_basename("/", "/");
    test_basename("///", "/");

    test_basename("/abc", "abc");
    test_basename("/.abc", ".abc");

    test_basename("/a/b/c", "c");

    test_basename("/a/b/c/", "c");

    test_basename("abc", "abc");

    test_basename("./abc", "abc");
}

TEST_F(test_FileUtil, case_scan_dir_1)
{
    int ret = 0;
    std::vector<std::string> file_list;
    ret = FileUtil::scan_dir("./", "test_file_util", &file_list);
    EXPECT_EQ(0, ret);
    EXPECT_EQ(3, (int)file_list.size());
    if (file_list.size() > 0) {
        EXPECT_STREQ("test_file_util.cpp", file_list[0].c_str());
        EXPECT_STREQ("test_file_util", file_list[1].c_str());
        EXPECT_STREQ("test_file_util_test_file_util.o", file_list[2].c_str());
    }
}

