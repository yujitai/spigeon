#ifndef _SCOPED_PTR_
#define _SCOPED_PTR_

namespace store {

template <class T>
class scoped_ptr {
  public:
    typedef T element_type;
    explicit scoped_ptr(element_type *t): _t(t) {}
    ~scoped_ptr() { delete _t; };
    element_type &operator*() const { return *_t; }        
    element_type *operator->() const { return _t; }
    T *get() const { return _t; }
  private:
    // non-copyable
    scoped_ptr(const scoped_ptr &);
    scoped_ptr &operator = (const scoped_ptr &);

    T *_t;
};

}

#endif
