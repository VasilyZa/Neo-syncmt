#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <cstring>
#include <algorithm>
#include <iomanip>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <cerrno>
#include <dirent.h>
#include <unordered_set>
#include <liburing.h>
#include <memory>
#include <functional>

namespace fs = std::filesystem;

#ifndef VERSION
#define VERSION "1.1.3"
#endif

#ifndef GIT_COMMIT
#define GIT_COMMIT "unknown"
#endif

#ifndef BUILD_DATE
#define BUILD_DATE __DATE__
#endif

constexpr size_t DEFAULT_THREADS = 4;
constexpr size_t MIN_CHUNK_SIZE = 64 * 1024;
constexpr size_t MAX_CHUNK_SIZE = 16 * 1024 * 1024;
constexpr size_t SMALL_FILE_THRESHOLD = 1024 * 1024;
constexpr size_t LARGE_FILE_THRESHOLD = 100 * 1024 * 1024;
constexpr size_t PROGRESS_UPDATE_INTERVAL_MS = 100;

extern bool use_chinese;

enum class ConflictResolution {
    OVERWRITE,
    SKIP,
    ERROR
};

struct FileTask {
    fs::path src;
    fs::path dst;
    bool is_move;
    ConflictResolution conflict_resolution;
};

#endif // COMMON_H
