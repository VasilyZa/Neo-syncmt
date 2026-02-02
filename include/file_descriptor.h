#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H

#include "common.h"

class FileDescriptor {
    int fd;
public:
    FileDescriptor(const char* path, int flags, mode_t mode = 0) : fd(open(path, flags, mode)) {
        if (fd == -1) {
            throw std::runtime_error(std::string("Failed to open file: ") + path +
                                   " (errno: " + std::to_string(errno) +
                                   " - " + strerror(errno) + ")");
        }
    }

    ~FileDescriptor() {
        if (fd != -1) {
            close(fd);
        }
    }

    FileDescriptor(const FileDescriptor&) = delete;
    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept : fd(other.fd) {
        other.fd = -1;
    }

    FileDescriptor& operator=(FileDescriptor&& other) noexcept {
        if (this != &other) {
            if (fd != -1) {
                close(fd);
            }
            fd = other.fd;
            other.fd = -1;
        }
        return *this;
    }

    operator int() const { return fd; }
    int get() const { return fd; }
};

#endif // FILE_DESCRIPTOR_H
