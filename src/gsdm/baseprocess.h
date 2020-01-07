/*******************************************************************
 * Copyright (c) 2020 Zuoyebang.com, Inc. All Rights Reserved
 * 
 * @file baseprocess.h
 * @author yujitai
 * @date 2020/01/06 20:46:29
 ******************************************************************/

#ifndef  __BASEPROCESS_H_
#define  __BASEPROCESS_H_

#include <stddef.h>
#include <stdint.h>

namespace zf {

class NewGenericWorker;

class BaseProcess {
public:
    BaseProcess();
    ~BaseProcess();

    /**
     * This is called by the framework when data is available for processing, 
     * directly from the network i/o layer.
     */
    virtual bool process(const uint8_t* buffer, size_t len);
private:
    NewGenericWorker* const _wk = nullptr;
};

} // namespace zf

#endif  //__BASEPROCESS_H_


