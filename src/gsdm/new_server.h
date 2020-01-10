/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file server.h
 * @author yujitai
 * @date 2020/01/06 20:15:49
 ******************************************************************/

#ifndef  __SERVER_H_
#define  __SERVER_H_

#include <vector>

namespace zf {

class NewGenericWorker;

class NewGenericServer {
public:
    NewGenericServer();
    ~NewGenericServer();

    int initialize();
    void setup_queue(NewGenericWorker* src, std::vector<NewGenericWorker*>& dst, int cmd_type);
    void setup_queue(std::vector<NewGenericWorker*>& src, std::vector<NewGenericWorker*>& dst, int cmd_type);
private:
    std::vector<NewGenericWorker*> _workers;    
};

} // namespace zf

#endif  //__SERVER_H_


