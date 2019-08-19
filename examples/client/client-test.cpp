/***************************************************************************
 * 
 * Copyright (c) 2013 Baidu.com, Inc. All Rights Reserved
 * 
 **************************************************************************/
 
/**
 * @file client-test.cpp
 * @author li_zhe(li_zhe@baidu.com)
 * @date 2013/04/24 22:48:05
 * @brief 
 **/

#include <store_framework.h>


void response_cb(const store::HttpClientResponse *resp, void *data) {
    UNUSED(data);
    if (!resp) {
        printf("null\n");
    } else if (resp->is_timeout) {
        printf("timeout\n");
    } else {
        printf("status: %d, len:%lu\n", resp->status_code, resp->response.size());
    }
}

int main() {
    int ret = 0;

    store::EventLoop el(NULL, true);

    store::HttpClient client(&el, response_cb, "http://www.sina.com.cn");
    ret = client.init();
    if (ret != 0) {
        printf("client init failed\n");
        return -1;
    }

    ret = client.get("/", NULL);
    if (ret != 0) {
        printf("client get failed\n");
        return -1;
    }

    el.run();

    return 0;
}

