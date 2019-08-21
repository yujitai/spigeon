#ifndef _STATUS_H_
#define _STATUS_H_

#include <string>
#include <stdio.h>
#include <stdint.h>

#include "util/slice.h"

namespace zf {

class Status {
  private:
    enum Code {
        kOk = 0,

        kBadRequest = 400,
        kUnauthorized = 401,
        kForbidden = 403,
        kNotFound = 404,
        kUnknownMethod = 405,
        kRequestTimeout = 408,
        kEntityTooLarge = 413,
        kUnsupportedType = 415,
        kTooManyRequests = 429,

        kInternalError = 500,
        kNotImplemented = 501,
        kServiceUnavailable = 503,
        kServiceTimeout = 504,
        kVersionNotSupported = 505
    };

  public:
    // Create a success status.
    Status() : state_(NULL) { }
    ~Status() { delete[] state_; }

    // Copy the specified status.
    Status(const Status& s);
    void operator=(const Status& s);

    // Return a success status.
    static Status OK() { return Status(); }

    // Return error status of an appropriate type.
    static Status BadRequest(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kBadRequest, msg, msg2);
    }

    static Status Unauthorized(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kUnauthorized, msg, msg2);
    }

    static Status Forbidden(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kForbidden, msg, msg2);
    }
    
    static Status NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kNotFound, msg, msg2);
    }

    static Status UnknownMethod(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kUnknownMethod, msg, msg2);
    }

    static Status RequestTimeout(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kRequestTimeout, msg, msg2);
    }

    static Status EntityTooLarge(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kEntityTooLarge, msg, msg2);
    }

    static Status UnsupportedType(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kUnsupportedType, msg, msg2);
    }

    static Status TooManyRequest(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kTooManyRequests, msg, msg2);
    }

    static Status InternalError(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kInternalError, msg, msg2);
    }

    static Status NotImplemented(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kNotImplemented, msg, msg2);
    }

    static Status ServiceUnavailable(const Slice& msg,
                                     const Slice& msg2 = Slice()) {
        return Status(kServiceUnavailable, msg, msg2);
    }

    static Status ServiceTimeout(const Slice& msg, const Slice& msg2 = Slice()) {
        return Status(kServiceTimeout, msg, msg2);
    }
    static Status VersionNotSupported(const Slice& msg,
                                      const Slice& msg2 = Slice()) {
        return Status(kVersionNotSupported, msg, msg2);
    }    
    // Returns true iff the status indicates success.
    bool ok() const { return (state_ == NULL); }

    bool isBadRequest() const { return code() == kBadRequest; }

    bool isForbidden() const { return code() == kForbidden; }

    bool isNotFound() const { return code() == kNotFound; }

    bool isUnknownMethod() const { return code() == kUnknownMethod; }

    bool isEntityTooLarge() const { return code() == kEntityTooLarge; }

    bool isUnsupportedType() const { return code() == kUnsupportedType; }

    bool isInternalError() const { return code() == kInternalError; }

    bool isVersionNotSupported() const { return code() == kVersionNotSupported; }
    
    
    // Return a string representation of this status suitable for
    // printing.
    // Returns the string "OK" for success.
    std::string toString() const;

    Code code() const {
        if (state_ == NULL)
            return kOk;
        else {
            uint16_t c;
            memcpy(&c, state_ + 4, sizeof(uint16_t));
            return static_cast<Code>(c);
        }
    }

  private:
    // OK status has a NULL state_.  Otherwise, state_ is a new[]
    // array
    // of the following form:
    //    state_[0..3] == length of message
    //    state_[4]    == code
    //    state_[5..]  == message
    const char* state_;



    Status(Code code, const Slice& msg, const Slice& msg2);
    static const char* CopyState(const char* s);
};

inline Status::Status(const Status& s) {
    state_ = (s.state_ == NULL) ? NULL : CopyState(s.state_);
}
inline void Status::operator=(const Status& s) {
    // The following condition catches both aliasing (when this ==
    // &s),
    // and the common case where both s and *this are ok.
    if (state_ != s.state_) {
        delete[] state_;
        state_ = (s.state_ == NULL) ? NULL : CopyState(s.state_);
    }
}

}  // namespace zf

#endif  // _STATUS_H_
