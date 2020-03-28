/*****************************************************************
* Copyright (C) 2020 Zuoyebang.com, Inc. All Rights Reserved.
* 
* @file lock_unittest.cpp
* @author yujitai(yujitai@zuoyebang.com)
* @date 2020/03/23
* @brief 
*****************************************************************/

#include <unistd.h>

#include <iostream> 
using namespace std;

#include <thread>   
#include <vector>   

int global_counter(0);

pthread_mutex_t mutex;
pthread_rwlock_t rwlock;

namespace zframework {

void writer(int id) {
    while (global_counter < 1024) {
        // pthread_rwlock_wrlock(&rwlock);
        pthread_mutex_lock(&mutex);
        cout << "writer " << id << "=====>global_counter=" << (global_counter+=20) << endl;
        pthread_mutex_unlock(&mutex);
        // pthread_rwlock_unlock(&rwlock);
        sleep(1);
    }
}

int reader(int id) {
    while (global_counter < 1024) {
        // pthread_rwlock_rdlock(&rwlock);
        pthread_mutex_lock(&mutex);
        cout << "reader " << id << "=====>global_counter=" << global_counter << endl;
        pthread_mutex_unlock(&mutex);
        // pthread_rwlock_unlock(&rwlock);
        sleep(1);
    }
}

// zf::RWLock rwlock;

void test_rw_lock() {
    std::vector<std::thread> threads;

    for(int i = 1; i <= 5; ++i) 
        threads.push_back(std::thread(writer, i));
    for(int i = 1; i <= 5; ++i) 
        threads.push_back(std::thread(reader, i));
    for (auto& thread : threads) 
        thread.join();
}

} // namespace zframework

int main() {
    pthread_rwlock_init(&rwlock, NULL);
    pthread_mutex_init(&mutex, NULL);

    zframework::test_rw_lock();

    return 0;
}



