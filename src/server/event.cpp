#include <assert.h>
#include <ev.h>

#include "server/event.h"
#include "util/queue.h"
#include "util/log.h"
#include "util/store_define.h"
#include "util/utils.h"

namespace zf {

/* translate to/from libev mask */
#define TRANS_TO_EV_MASK(mask)                                          \
    (((mask)&EventLoop::READ?EV_READ:0) | ((mask)&EventLoop::WRITE?EV_WRITE:0))
#define TRANS_FROM_EV_MASK(ev_mask)                                     \
    (((ev_mask)&EV_READ?EventLoop::READ:0) | ((ev_mask)&EV_WRITE?EventLoop::WRITE:0))

EventLoop *EventLoop::default_loop = NULL;


unsigned long EventLoop::current_time() {
    if (default_loop)
        return default_loop->now();
    else {
        return (unsigned long)ustime();
    }
}

EventLoop::EventLoop(void *el_owner, bool use_default)
        : owner(el_owner), loop(NULL){
    if (use_default) {
        loop = EV_DEFAULT;
        default_loop = this;
        log_debug("init default event loop");
    } else {
        loop = ev_loop_new(EVFLAG_AUTO);
        log_debug("init event loop");
    }
    ev_set_userdata(loop, (void*)this);
}

EventLoop::~EventLoop() {
    if (loop) {
        stop();
        /* Only destroy the loop created with ev_loop_new */
        if (!ev_is_default_loop(loop)) {  
            ev_loop_destroy(loop);
            log_debug("event loop destroyed");
        } else {
            log_debug("default event loop stopped");
        }
    }
}

void EventLoop::run() {
    if (ev_is_default_loop(loop)) {
        log_debug("default event loop started");
    } else {
        log_debug("event loop started");
    }
    ev_run(loop, 0);
}

void EventLoop::suspend() {
    ev_suspend(loop);
}

void EventLoop::resume() {
    ev_resume(loop);
}

void EventLoop::stop() {
    ev_break(loop, EVBREAK_ALL);
}

void EventLoop::sleep(unsigned long usec) {
    ev_sleep(usec/1000000.0);
}

unsigned long EventLoop::now() {
    return (unsigned long)(ev_now(loop)*1000000);
}

class TimerWatcher {
  public:
    struct ev_timer timer;
    timer_cb_t cb;
    EventLoop *el;
    void *data;
    bool need_repeat;
    TimerWatcher(EventLoop *el, timer_cb_t cb, void *priv_data, bool repeat);
};

TimerWatcher::TimerWatcher(
    EventLoop *eventloop, timer_cb_t callback, void *priv_data, bool repeat)
        : cb(callback), el(eventloop), data(priv_data), need_repeat(repeat) {
    timer.data = (void*)this;
}

void generic_timer_cb(struct ev_loop *el, struct ev_timer *w, int revents) {
    UNUSED(el);
    UNUSED(revents);
    TimerWatcher *watcher = (TimerWatcher*)w;
    watcher->cb(watcher->el, watcher, watcher->data);
}

TimerWatcher *EventLoop::create_timer(timer_cb_t cb, 
        void *priv_data, 
        bool repeat) 
{
    TimerWatcher *w = new TimerWatcher(this, cb, priv_data, repeat);
    ev_init(&(w->timer), generic_timer_cb);
    return w;
}

void EventLoop::start_timer(TimerWatcher *w, unsigned long usec) {
    struct ev_timer *timer = &(w->timer);
    float sec = (float)usec/1000000;
    if (!w->need_repeat) {
        ev_timer_stop(loop, timer);
        ev_timer_set(timer, sec, 0);
        ev_timer_start(loop, timer);
    } else {
        /* We can change the timeout on the fly */
        timer->repeat = sec;
        ev_timer_again(loop, timer);
    }  
}

void EventLoop::stop_timer(TimerWatcher *w) {
    struct ev_timer *timer = &(w->timer);
    ev_timer_stop(loop, timer);
    log_debug("stop_timer");
}

void EventLoop::delete_timer(TimerWatcher *w) {
    stop_timer(w);
    delete w;
}

class IOWatcher {
  public:
    ev_io io;
    io_cb_t cb;
    EventLoop *el;
    void *data;
    IOWatcher(EventLoop *el, io_cb_t cb, void *priv_data);
};

IOWatcher::IOWatcher(EventLoop *eventloop, io_cb_t callback, void *priv_data)
        : cb(callback), el(eventloop), data(priv_data){
    io.data = (void*)this;
}

void generic_io_cb(struct ev_loop *el, struct ev_io *w, int revents) {
    UNUSED(el);
    IOWatcher *watcher = (IOWatcher*)(w->data);
    watcher->cb(watcher->el, watcher, w->fd,
                TRANS_FROM_EV_MASK(revents), watcher->data);
}

IOWatcher *EventLoop::create_io_event(io_cb_t cb, void *data) {
    IOWatcher *w = new IOWatcher(this, cb, data);
    ev_init(&(w->io), generic_io_cb);
    return w;
}


void EventLoop::start_io_event(IOWatcher *w, int fd, int mask) {
    struct ev_io *io = &(w->io);
    /* If there's no change on the events, just return */
    if (ev_is_active(io)) {
        int active_events = TRANS_FROM_EV_MASK(io->events);
        int events = active_events | mask;        
        if (active_events == events) {
            return;    
        }
        events = TRANS_TO_EV_MASK(events);
        ev_io_stop(loop, io);
        ev_io_set(io, fd, events);
        ev_io_start(loop, io);
    } else {
        int events = TRANS_TO_EV_MASK(mask);
        ev_io_set(io, fd, events);
        ev_io_start(loop, io);
    }
}

void EventLoop::stop_io_event(IOWatcher *w, int fd, int mask) {
    struct ev_io *io = &(w->io);
    int active_events = TRANS_FROM_EV_MASK(io->events);
    int events = active_events & (~mask);
    if (active_events == events)
        return;
    events = TRANS_TO_EV_MASK(events);
    ev_io_stop(loop, io);  
    if (events != EV_NONE) {
        ev_io_set(io, fd, events);
        ev_io_start(loop, io);
    }
}

void EventLoop::delete_io_event(IOWatcher *w) {
    struct ev_io *io = &(w->io);
    ev_io_stop(loop, io);
    delete w;
}

} // namespace zf


