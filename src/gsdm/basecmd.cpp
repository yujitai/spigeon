/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file basecmd.cpp
 * @author yujitai
 * @date 2020/01/06 19:26:28
 ******************************************************************/

#include "gsdm/basecmd.h"

namespace zf {

BaseCmd::BaseCmd() {}

BaseCmd::~BaseCmd() {
}

bool BaseCmd::process(NewGenericWorker* wk) {
    return true;
}

} // namespace zf


