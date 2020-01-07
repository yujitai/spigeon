/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file server.cpp
 * @author yujitai
 * @date 2020/01/06 20:29:25
 ******************************************************************/

#include "gsdm/server.h"

namespace zf {

NewGenericServer::NewGenericServer() {}

NewGenericServer::~NewGenericServer() {}

int NewGenericServer::initialize() {
    return 0;
}

void NewGenericServer::setup_queue(NewGenericWorker* src, std::vector<NewGenericWorker*>& dst, int cmd_type) {

}

void NewGenericServer::setup_queue(std::vector<NewGenericWorker*>& src, std::vector<NewGenericWorker*>& dst, int cmd_type) {

}

} // namespace zf


