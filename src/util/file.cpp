/***************************************************************************
 * 
 * Copyright (c) 2012 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file file.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2012/12/02 18:49:15
 * @brief 
 **/

#include "file.h"
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include<unistd.h>

#include "util/log.h"

namespace zf {

int FileUtil::exists(const std::string &path) {
    if (access(path.c_str(),F_OK) == 0) {
        return 0;
    } else {
        return -1;
    }
}

int FileUtil::file_size(int fd, uint64_t *out_size) {
    struct stat sbuf;
    if (fstat(fd, &sbuf) != 0) {
        *out_size = 0;
        log_fatal("stat for fd(%d) failed. err:%d(%s)", fd, errno, strerror(errno));
        return -1;
    } else {
        *out_size = sbuf.st_size;
        return 0;
    }
}

int FileUtil::file_size(const std::string &path, uint64_t *out_size) {
    struct stat sbuf;
    if (stat(path.c_str(), &sbuf) != 0) {
        *out_size = 0;
        log_fatal("stat for %s failed. err:%d(%s)", path.c_str(), errno, strerror(errno));
        return -1;
    } else {
        *out_size = sbuf.st_size;
        return 0;
    }
}

std::string FileUtil::dirname(const std::string &pathname, bool reserve_last_slash) {
    std::string parent = pathname;
    int32_t last = parent.size() - 1;

    // skip the tracing '/'s
    while (last >= 0 && '/' == parent[last]) {
        --last;
    }
    if (-1 == last) {
        return "/"; // 根目录
    }

    // skip the letters
    while (last >= 0 && '/' != parent[last]) {
        --last;
    }
    if (-1 == last) {
        parent = "./"; // 使用相对路径，并且是当前目录，比如'abc'
        last = 1;
    } else {
        parent[last+1] = '\0';
    }

    if (!reserve_last_slash) {
        //last = strlen(parent) - 1;
        while (last > 0 && '/' == parent[last]) { // last>0 避免根目录
            --last;
        }
        //now last is indexing to an non '/' or is 0
        parent[last+1] = '\0';
    }

    return parent;
}

std::string FileUtil::basename(const std::string &pathname) {
    int last = pathname.size() - 1;

    // skip the tracing '/'s
    while (last >= 0 && '/' == pathname[last]) {
        --last;
    }
    if (-1 == last) {
        return "/"; // 根目录
    }

    // skip the letters
    int len = 0;
    while (last >= 0 && '/' != pathname[last]) {
        --last;
        len++;
    }
    // now last is indexing to an '/' or is -1

    return pathname.substr(last+1, len);
}

int FileUtil::create_parent_dir(const std::string &path) {
    const std::string parent = dirname(path);

    struct stat dirstat;
    if (0 == stat(parent.c_str(), &dirstat)
        && S_ISDIR(dirstat.st_mode)
        && 0 == access(parent.c_str(), R_OK|W_OK|X_OK))
    {
        return 0;
    } else {
        if (0 == access(parent.c_str(), F_OK)) {
            log_warning("invalid path type or permission. path:%s", parent.c_str());
            return -1;
        } else if (0 == create_parent_dir(parent) && 0 == mkdir(parent.c_str(), 0775)) {
            return 0;
        } else {
            log_warning("create dir(%s) failed", parent.c_str());
            return -1;
        }
    }
}

int FileUtil::create_dir(const std::string &path) {
    struct stat dirstat;
    if (0 == stat(path.c_str(), &dirstat)
        && S_ISDIR(dirstat.st_mode)
        && 0 == access(path.c_str(), R_OK|W_OK|X_OK)) 
    {
        return 0;
    } else if(0 == access(path.c_str(), F_OK)) {
        log_warning("invalid path type or permission. path:%s", path.c_str());
        return -1;
    } else if (0 == create_parent_dir(path) && 0 == mkdir(path.c_str(), 0775)) {
        return 0;
    } else {
        log_warning("create dir(%s) failed", path.c_str());
        return -1;
    }
}

int FileUtil::scan_dir(const std::string &dir_path, const std::string &prefix,
    std::vector<std::string> *out_file_list)
{
    int ret = 0;
    DIR *dir = NULL;
    struct dirent cur_dirent;
    struct dirent *result = NULL;
    const int prefix_len = prefix.length();

    out_file_list->clear();

    dir = opendir(dir_path.c_str());
    if (dir == NULL) {
        log_warning("opendir %s failed, err:%d(%s)", dir_path.c_str(), errno, strerror(errno));
        ret = -1;
        goto label_exit;
    }

    while (true) {
        if (0 != readdir_r(dir, &cur_dirent, &result)) {
            log_warning("readdir_r %s failed, err:%d(%s)", dir_path.c_str(), errno, strerror(errno));
            ret = -1;
            goto label_exit;
        }
        if (result == NULL) {
            break;
        }

        if (strcmp(result->d_name, "..") == 0 || strcmp(result->d_name, ".") == 0) {
            continue;
        }

        if (memcmp(result->d_name, prefix.c_str(), prefix_len) == 0) {
            out_file_list->push_back(result->d_name);
        }
    }

label_exit:
    if (dir != NULL) {
        closedir(dir);
    }
    return ret;
}

} // namespace zf


