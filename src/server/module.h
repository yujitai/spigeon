#ifndef _MODULE_H_
#define _MODULE_H_

namespace zf {

/**
 * @brief Module
 *
 **/
class Module {
  public:
    // Provides the default configurations
    virtual int init_conf() = 0;
    // Load configuration file
    virtual int load_conf(const char *filename) = 0;
    // Validate the configurations
    virtual int validate_conf() = 0;
};

} // namespace zf

#endif


