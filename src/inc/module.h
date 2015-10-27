#ifndef _MODULE_H_
#define _MODULE_H_

namespace store {
class Module {
  public:
    // provides the default configurations
    virtual int init_conf() = 0;
    // load configuration file
    virtual int load_conf(const char *filename) = 0;
    // validate the configurations
    virtual int validate_conf() = 0;
};

}

#endif
