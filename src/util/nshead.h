#ifndef __NSHEAD_H_
#define __NSHEAD_H_

namespace zf {

static const unsigned int NSHEAD_MAGICNUM = 0xfb709394;
struct nshead_t
{
    unsigned short id;              
    unsigned short version;         
    unsigned int   log_id;
    char           provider[16];
    unsigned int   magic_num;
    unsigned int   reserved;      
    unsigned int   body_len;
}; 

} // namespace zf

#endif


