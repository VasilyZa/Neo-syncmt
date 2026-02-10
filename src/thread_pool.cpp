#include "thread_pool.h"

std::mutex ThreadPool::cout_mutex;

ThreadPool::ThreadPool(size_t num_threads, std::atomic<size_t>* processed_files_ptr,
           std::atomic<uint64_t>* copied_bytes_ptr, ConflictResolution conflict_resolution,
           bool verbose, std::unordered_set<fs::path>* created_dirs_ptr)
    : stop(false), processed_files_ptr(processed_files_ptr),
      copied_bytes_ptr(copied_bytes_ptr), conflict_resolution(conflict_resolution),
      verbose(verbose), created_dirs_ptr(created_dirs_ptr) {
    workers.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers.emplace_back([this] {
            for (;;) {
                FileTask task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });
                    if (this->stop && this->tasks.empty())
                        return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                process_task(task);
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
        worker.join();
}

void ThreadPool::enqueue(FileTask task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

size_t ThreadPool::calculate_chunk_size(uint64_t file_size) {
    if (file_size < SMALL_FILE_THRESHOLD) {
        return MIN_CHUNK_SIZE;
    } else if (file_size < LARGE_FILE_THRESHOLD) {
        size_t adaptive_size = file_size / 10;
        adaptive_size = std::max(adaptive_size, MIN_CHUNK_SIZE);
        adaptive_size = std::min(adaptive_size, MAX_CHUNK_SIZE);
        return adaptive_size;
    } else {
        size_t adaptive_size = file_size / 100;
        adaptive_size = std::max(adaptive_size, static_cast<size_t>(1024 * 1024));
        adaptive_size = std::min(adaptive_size, MAX_CHUNK_SIZE);
        return adaptive_size;
    }
}

void ThreadPool::process_task(const FileTask& task) {
    try {
        if (fs::exists(task.dst)) {
            switch (task.conflict_resolution) {
                case ConflictResolution::SKIP:
                    if (verbose) {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        if (use_chinese) {
                            std::cout << "跳过已存在的文件: " << task.dst << std::endl;
                        } else {
                            std::cout << "Skipping existing file: " << task.dst << std::endl;
                        }
                    }
                    (*processed_files_ptr)++;
                    return;
                case ConflictResolution::OVERWRITE:
                    if (verbose) {
                        std::lock_guard<std::mutex> lock(cout_mutex);
                        if (use_chinese) {
                            std::cout << "覆盖已存在的文件: " << task.dst << std::endl;
                        } else {
                            std::cout << "Overwriting existing file: " << task.dst << std::endl;
                        }
                    }
                    break;
                case ConflictResolution::ERROR:
                    if (use_chinese) {
                        throw std::runtime_error(std::string("目标文件已存在: ") +
                                               task.dst.string());
                    } else {
                        throw std::runtime_error(std::string("Destination file already exists: ") +
                                               task.dst.string());
                    }
            }
        }

        uint64_t file_size = fs::file_size(task.src);
        if (task.is_move) {
            move_file(task.src, task.dst, *created_dirs_ptr);
        } else {
            copy_file(task.src, task.dst, *created_dirs_ptr);
        }
        (*processed_files_ptr)++;
        (*copied_bytes_ptr) += file_size;
    } catch (const std::exception& e) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        if (use_chinese) {
            std::cerr << "处理 " << task.src << " 时出错: " << e.what() << std::endl;
        } else {
            std::cerr << "Error processing " << task.src << ": " << e.what() << std::endl;
        }
    }
}

void ThreadPool::copy_file(const fs::path& src, const fs::path& dst, std::unordered_set<fs::path>& created_dirs) {
    FileDescriptor src_fd(src.c_str(), O_RDONLY);

    struct stat src_stat;
    if (fstat(src_fd.get(), &src_stat) == -1) {
        throw std::runtime_error(std::string("Failed to stat source file: ") + src.string() +
                               " (errno: " + std::to_string(errno) +
                               " - " + strerror(errno) + ")");
    }

    posix_fadvise(src_fd.get(), 0, src_stat.st_size, POSIX_FADV_SEQUENTIAL);

    fs::path dst_parent = dst.parent_path();
    if (!dst_parent.empty()) {
        std::lock_guard<std::mutex> lock(created_dirs_mutex);
        if (created_dirs.find(dst_parent) == created_dirs.end()) {
            if (!fs::exists(dst_parent)) {
                fs::create_directories(dst_parent);
            }
            created_dirs.insert(dst_parent);
        }
    }

    FileDescriptor dst_fd(dst.c_str(), O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);

    if (src_stat.st_size > 0) {
        if (fallocate(dst_fd.get(), 0, 0, src_stat.st_size) == -1) {
            if (errno != ENOTSUP && errno != EOPNOTSUPP) {
                throw std::runtime_error(std::string("Failed to fallocate: ") + dst.string() +
                                       " (errno: " + std::to_string(errno) +
                                       " - " + strerror(errno) + ")");
            }
        }
    }

    if (src_stat.st_size > 0) {
        off_t offset = 0;
        size_t remaining = src_stat.st_size;
        while (remaining > 0) {
            ssize_t sent = sendfile(dst_fd.get(), src_fd.get(), &offset, remaining);
            if (sent <= 0) {
                if (sent == -1 && errno == EINTR) continue;
                throw std::runtime_error(std::string("sendfile failed: ") + src.string() +
                                       " (errno: " + std::to_string(errno) +
                                       " - " + strerror(errno) + ")");
            }
            remaining -= sent;
        }
    }

    struct timespec times[2];
    times[0] = src_stat.st_atim;
    times[1] = src_stat.st_mtim;
    utimensat(AT_FDCWD, dst.c_str(), times, 0);
}

void ThreadPool::move_file(const fs::path& src, const fs::path& dst, std::unordered_set<fs::path>& created_dirs) {
    fs::path dst_parent = dst.parent_path();
    if (!dst_parent.empty()) {
        std::lock_guard<std::mutex> lock(created_dirs_mutex);
        if (created_dirs.find(dst_parent) == created_dirs.end()) {
            if (!fs::exists(dst_parent)) {
                fs::create_directories(dst_parent);
            }
            created_dirs.insert(dst_parent);
        }
    }

    if (rename(src.c_str(), dst.c_str()) == 0) {
        return;
    }

    if (errno == EXDEV) {
        copy_file(src, dst, created_dirs);
        fs::remove(src);
    } else {
        throw std::runtime_error(std::string("Failed to move file: ") + src.string() +
                               " (errno: " + std::to_string(errno) +
                               " - " + strerror(errno) + ")");
    }
}
