#ifndef _EVENT_H_
#define _EVENT_H_
#include <pthread.h>
#include <string>
#include "util/queue.h"

struct ev_loop;

namespace store {
enum {
    EVENT_ERROR = 1,
    EVENT_OK = 0
};


class IOWatcher;
class TimerWatcher;
class CustomWatcher;
class EventLoop;

// callback type
typedef void (*io_cb_t)(EventLoop *el, IOWatcher *w, int fd, int revents,
                        void *priv_data);

typedef void (*timer_cb_t)(EventLoop *el, TimerWatcher *w, void *priv_data);
typedef void (*custom_cb_t)(EventLoop *el, void *priv_data, void *data);


class EventLoop {
  public:
    enum {
        READ = 0x1,
        WRITE = 0x2
    };

    EventLoop(void *owner, bool use_default);
    ~EventLoop();
  
    void run();
    void suspend();
    void resume();
    void stop();
    void sleep(unsigned long usec);

    unsigned long now();                // get current time
    // timer
    TimerWatcher *create_timer(timer_cb_t cb, void *priv_data, bool repeat);
    void start_timer(TimerWatcher *w, unsigned long usec);
    void stop_timer(TimerWatcher *w);
    void delete_timer(TimerWatcher *w);
    // io
    IOWatcher *create_io_event(io_cb_t cb, void *priv_data);
    void start_io_event(IOWatcher *w, int fd, int mask);
    void stop_io_event(IOWatcher *w, int fd, int mask);
    void delete_io_event(IOWatcher *w);
  
    /* Inter threads custom events */
    CustomWatcher *create_custom_event(const std::string &desc);
    void wait_custom_event(CustomWatcher *w, custom_cb_t cb, void *priv_data);
    void signal_custom_event(CustomWatcher *w);
    void delete_custom_event(CustomWatcher *w);
    /* Global current time */
    static unsigned long current_time();
    /* reference to owner of the EventLoop */
    void *owner;

    inline struct ev_loop* getLoop() {
        return loop; 
    }   
  
  private:
    friend class HttpClient;
    struct ev_loop *loop;
    /* Reference to the default loop, not thread safe,
       only used for current_time()  */
    static EventLoop *default_loop;     
};

}
#endif

