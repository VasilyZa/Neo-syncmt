#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "common.h"
#include "file_descriptor.h"

class ThreadPool {
public:
    ThreadPool(size_t num_threads, std::atomic<size_t>* processed_files_ptr,
               std::atomic<uint64_t>* copied_bytes_ptr, ConflictResolution conflict_resolution,
               bool verbose, std::unordered_set<fs::path>* created_dirs_ptr);
    ~ThreadPool();

    void enqueue(FileTask task);

private:
    std::vector<std::thread> workers;
    std::queue<FileTask> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
    std::atomic<size_t>* processed_files_ptr;
    std::atomic<uint64_t>* copied_bytes_ptr;
    ConflictResolution conflict_resolution;
    bool verbose;
    std::unordered_set<fs::path>* created_dirs_ptr;
    std::mutex created_dirs_mutex;

    [[nodiscard]] static size_t calculate_chunk_size(uint64_t file_size);

    void process_task(const FileTask& task);
    void copy_file(const fs::path& src, const fs::path& dst, std::unordered_set<fs::path>& created_dirs);
    void move_file(const fs::path& src, const fs::path& dst, std::unordered_set<fs::path>& created_dirs);

    static std::mutex cout_mutex;
};

#endif // THREAD_POOL_H
