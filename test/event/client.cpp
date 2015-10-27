#include <string.h>
#include <cstdio>
#include <pthread.h>
#include "util/nshead.h"
#include "util/network.h"
#include "util/slice.h"
#include "util/zmalloc.h"
#include "util/log.h"
using namespace store;

const int thread_num = 1;
int counter[thread_num];

char *host = (char*)"127.0.0.1";
int port = 8080;
Slice mock_reply(const Slice& request) {
  int reply_len = sizeof(nshead_t)+request.size();
  char *buffer = (char*)zmalloc(reply_len);
  nshead_t header;
  header.body_len = request.size();
  header.magic_num = NSHEAD_MAGICNUM;
  memcpy(buffer, (void*)&header, sizeof(nshead_t));
  memcpy(buffer+sizeof(nshead_t), request.data(), request.size());
  
  return Slice(buffer, reply_len);

}
void* run_bench(void *arg) {
  long tid = (long)arg;
  counter[tid] = 0;
  Slice ping = mock_reply(Slice("ping"));
  char read_buffer[1024];
  int s = tcp_connect(host, port);
  if (s == NET_ERROR) {
    printf("connect error\n");
    return NULL;
  }
    
  while(1) {
   
    int nwrite = write_data(s, ping.data(), ping.size());
    if (nwrite != ping.size()) {
      printf("write error, %d\n", nwrite);
      exit(1);
    }
    int nread =  read_data(s, read_buffer, 1024);
    if (nread != ping.size()) {
      printf("read error: %d\n", nread);
      exit(1);
    }
    counter[tid]++;
  
  }
}

int main() {

  pthread_t threads[thread_num];
  for (int i = 0; i < thread_num; i++) {
    pthread_create(&threads[i], NULL, run_bench, (void*)i);
  }
  int last_sum = 0;
  while (1) {
    int sum = 0;
    for (int i = 0; i < thread_num; i++) {
      sum += counter[i];
    }

    printf("%d\n", sum-last_sum);
    last_sum = sum;


    sleep(1);
  }
}
