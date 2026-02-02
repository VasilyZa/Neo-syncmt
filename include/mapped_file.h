#ifndef MAPPED_FILE_H
#define MAPPED_FILE_H

#include "common.h"

class MappedFile {
    void* addr;
    size_t length;
    int fd;

public:
    MappedFile(void* addr, size_t length, int fd)
        : addr(addr), length(length), fd(fd) {}

    ~MappedFile() {
        if (addr != MAP_FAILED) {
            msync(addr, length, MS_SYNC);
            munmap(addr, length);
        }
        if (fd != -1) {
            close(fd);
        }
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept
        : addr(other.addr), length(other.length), fd(other.fd) {
        other.addr = MAP_FAILED;
        other.length = 0;
        other.fd = -1;
    }

    MappedFile& operator=(MappedFile&& other) noexcept {
        if (this != &other) {
            if (addr != MAP_FAILED) {
                msync(addr, length, MS_SYNC);
                munmap(addr, length);
            }
            if (fd != -1) {
                close(fd);
            }
            addr = other.addr;
            length = other.length;
            fd = other.fd;
            other.addr = MAP_FAILED;
            other.length = 0;
            other.fd = -1;
        }
        return *this;
    }

    void* data() const { return addr; }
    size_t size() const { return length; }
};

#endif // MAPPED_FILE_H
