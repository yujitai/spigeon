/*******************************************************************
 * Copyright (c) 2019 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file basecmd.h
 * @author yujitai
 * @date 2019/10/29 20:08:40
 ******************************************************************/

#ifndef  __BASECMD_H_
#define  __BASECMD_H_

#include <stdint.h>

namespace zf {

class NewGenericWorker;

struct CmdInfo {
    uint32_t worker_id;
    int cmd_type;
};

class BaseCmd {
public:
    BaseCmd();
    virtual ~BaseCmd();

    virtual bool process(NewGenericWorker* wk);
};

} // namespace zf

#endif  //__BASECMD_H_


