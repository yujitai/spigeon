/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file file.h
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/02 18:48:59
 * @brief 
 **/

#ifndef __STORE_UTIL_FILE_H__
#define __STORE_UTIL_FILE_H__

#include <string>
#include <vector>
#include <stdint.h>
namespace store {

class FileUtil {
public:
    static int exists(const std::string &path);
    static int file_size(int fd, uint64_t *out_size);
    static int file_size(const std::string &path, uint64_t *out_size);
    static std::string dirname(const std::string &pathname, bool reserve_last_slash = false);
    static std::string basename(const std::string &pathname);
    static int create_parent_dir(const std::string &path);
    static int create_dir(const std::string &path);
    static int scan_dir(const std::string &dir_path, const std::string &prefix,
            std::vector<std::string> *out_file_list);
}; // end of FileUtil

} // end of namespace

#endif //__STORE_UTIL_FILE_H__

