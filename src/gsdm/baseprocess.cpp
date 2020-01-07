/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file baseprocess.cpp
 * @author yujitai
 * @date 2020/01/06 21:42:07
 ******************************************************************/

#include "gsdm/baseprocess.h"

namespace zf {

BaseProcess::BaseProcess() {}

BaseProcess::~BaseProcess() {}

bool BaseProcess::process(const uint8_t* buffer, size_t len) {
    // cout << "BaseProcess::process" << endl;
    return false;
}

} // namespace zf


