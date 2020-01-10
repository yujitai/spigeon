/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file demo.cpp
 * @author yujitai
 * @date 2020/01/06 10:54:30
 ******************************************************************/

#include <vector>

#include "gsdm/worker.h"
#include "server/thread.h"

int main(int argc, char** argv) 
{
    static int id = 1;

    zf::NewGenericWorker* wk1 = new zf::NewGenericWorker(id++);
    wk1->initialize();
    wk1->setup_queue(2, 1000);

    zf::NewGenericWorker* wk2 = new zf::NewGenericWorker(id++);
    wk2->initialize();
    wk2->setup_queue(1, 1000);

    zf::BaseCmd* cmd = new zf::BaseCmd();
    wk2->send_cmd(wk1, 1000, cmd);

    create_thread(wk1);
    create_thread(wk2);

    getchar();

    return 0;
}

