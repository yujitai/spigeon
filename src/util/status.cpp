#include <stdio.h>
#include "util/status.h"

namespace store {

const char* Status::CopyState(const char* state) {
    uint32_t size;
    memcpy(&size, state, sizeof(size));
    char* result = new char[size + 7];
    memcpy(result, state, size + 6);
    result[size + 6] = '\0';
    return result;
}

Status::Status(Code code, const Slice& msg, const Slice& msg2) {
    assert(code != kOk);
    const uint32_t len1 = msg.size();
    const uint32_t len2 = msg2.size();
    const uint32_t size = len1 + (len2 ? (2 + len2) : 0);

    char* result = new char[size + 7];
    memcpy(result, &size, sizeof(size));
    uint16_t c = code;
    memcpy(result + 4, &c, sizeof(uint16_t));
    memcpy(result + 6, msg.data(), len1);
    if (len2) {
        result[6 + len1] = ':';
        result[7 + len1] = ' ';
        memcpy(result + 8 + len1, msg2.data(), len2);
    }
    result[size + 6] = '\0';
    state_ = result;
}

std::string Status::toString() const {
    if (state_ == NULL) {
        return "OK";
    } else {
        char tmp[30];
        const char* type;
        switch (code()) {
            case kOk:
                type = "OK";
                break;
            case kBadRequest:
                type = "Bad Request: ";
                break;
            case kUnauthorized:
                type = "Unauthorized: ";
                break;
            case kForbidden:
                type = "Forbidden: ";
                break;
            case kNotFound:
                type = "Not Found: ";
                break;
            case kUnknownMethod:
                type = "Unknown Method: ";
                break;
            case kRequestTimeout:
                type = "Request Timeout: ";
                break;
            case kEntityTooLarge:
                type = "Entity Too Large: ";
                break;
            case kUnsupportedType:
                type = "Unsupported Type: ";
                break;
            case kTooManyRequests:
                type = "Too Many Requests: ";
                break;
            case kInternalError:
                type = "Internal Error: ";
                break;
            case kNotImplemented:
                type = "Not Implemented: ";
                break;
            case kServiceUnavailable:
                type = "Service Unavailable: ";
                break;
            case kServiceTimeout:
                type = "Service Timeout: ";
                break;
            case kVersionNotSupported:
                type = "Version Not Supported: ";
                break;
            default:
                snprintf(tmp, sizeof(tmp), "Unknown code(%d): ",
                         static_cast<int>(code()));
                type = tmp;
                break;
        }
        std::string result(type);
        uint32_t length;
        memcpy(&length, state_, sizeof(length));
        result.append(state_ + 6, length);
        return result;
    }
}

}  // namespace store
