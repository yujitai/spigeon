/***************************************************************************
 * 
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * $Id$ 
 * 
 **************************************************************************/
 
 
 
/**
 * @file iconfig.h
 * @author yujitai(yujitai@zuoyebang.com)
 * @version $Revision$ 
 * @brief 
 *  
 **/


#ifndef  __ICONFIG_H_
#define  __ICONFIG_H_

namespace zf {

// TODO:refine it
struct Options {};

/**
 * @brief Server config interface
 *
 **/
class IConfig {
public:
    // Init default configurations
    virtual int init_conf() = 0;

    // Init specified configurations
    virtual int init_conf(struct Options& o) = 0;

    // Load configuration file
    virtual int load_conf(const char* filename) = 0;

    // Validate the configurations
    virtual int validate_conf() = 0;
};

} // namespace zf

#endif  //__ICONFIG_H_


