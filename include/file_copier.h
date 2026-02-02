#ifndef FILE_COPIER_H
#define FILE_COPIER_H

#include "common.h"
#include "thread_pool.h"

class FileCopier {
public:
    FileCopier(size_t num_threads, bool move_mode, bool verbose, ConflictResolution conflict_resolution);

    void process(const std::vector<fs::path>& sources, const fs::path& dst);
    void process(const fs::path& src, const fs::path& dst);

private:
    ThreadPool pool;
    bool move_mode;
    bool verbose;
    ConflictResolution conflict_resolution;
    std::atomic<size_t> total_files;
    std::atomic<size_t> processed_files;
    std::atomic<uint64_t> total_bytes;
    std::atomic<uint64_t> copied_bytes;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point last_progress_update;
    std::unordered_set<fs::path> created_dirs;
    std::mutex cout_mutex;

    void scan_directory_native(const fs::path& src_dir, const fs::path& dst_dir,
                                std::vector<FileTask>& tasks, size_t& scan_counter, bool pipeline = false);
    void update_progress();
    void update_scan_progress(size_t scanned_files, uint64_t scanned_bytes);
    void process_directory(const fs::path& src_dir, const fs::path& dst_dir);
    void process_single_file(const fs::path& src, const fs::path& dst);
    void print_stats(const std::chrono::milliseconds& duration);
};

#endif // FILE_COPIER_H
