#include "fs.h"

#include <cstdint>

#include "../filesystem/filesys/fs_requests.h"
#include "atomic.h"
#include "event.h"
#include "filesys_compat/vector"
#include "hash.h"
#include "libk.h"
#include "locked_queue.h"
#include "ramfs.h"
#include "utils.h"

static Queue<FileListNode> open_files;
Lock* file_list_lock = nullptr;

volatile uint64_t inode_numbers = 0;
SpinLock inode_number_lock;

void init_file_list_lock() {
    if (file_list_lock == nullptr) {
        inode_number_lock.lock();
        if (file_list_lock == nullptr) {
            file_list_lock = new Lock;
        }
        inode_number_lock.unlock();
    }
}

string clean_path_string(string& file_path) {
    string file_name_copy = file_path;
    std::vector<string> path_parts;

    // Split the path into parts.
    string cur_part = "";
    for (size_t i = 0; i < file_name_copy.length(); i++) {
        if (file_name_copy[i] == '/' && cur_part.length() > 0) {
            if (cur_part == ".") {
                // Do nothing.
            } else if (cur_part == "..") {
                path_parts.pop_back();
            } else {
                path_parts.push_back(cur_part);
            }
            cur_part = "";
        } else if (file_name_copy[i] != '/') {
            cur_part += file_name_copy[i];
        }
    }
    path_parts.push_back(cur_part);

    // Join the parts back together.
    // slow but correct lmao.
    string cleaned_path = "/";
    for (size_t i = 0; i < path_parts.size(); i++) {
        printf("path_parts[%d]: %s\n", i, path_parts[i].c_str());
        cleaned_path += path_parts[i];
        if (i < path_parts.size() - 1) {
            cleaned_path += "/";
        }
    }

    return cleaned_path;
}

/**
 * opens a file specified by file name and returns a file* to it
 *
 * This as of right now does not verify the file exists, only checks if
 */
void kfopen(string file_name, Function<void(File*)> w) {
    init_file_list_lock();

    // Minify the path.
    string cleaned_file_name = clean_path_string(file_name);

    printf("kfopen(): opening %s\n", cleaned_file_name.c_str());

    // Get file hash.
    auto file_hash = cleaned_file_name.hash();

    /* TODO: verify file exists and get characteristics */
    file_list_lock->lock([=]() {
        mem_inode_t* file_inode = nullptr;
        open_files.remove_if_and_free_node([&](fileListNode* n) {
            // Don't remove the inode if we are about to use it.
            if (n->file_hash == file_hash) {
                file_inode = n->inode;
                return false;
            }
            return n->inode->refs == 0;
        });

        // We found the inode in the list.
        file* f = new file;
        if (file_inode != nullptr) {
            file_list_lock->unlock();

            // Link file struct to pre-existing in-mem inode.
            f->inode = file_inode;

            // Update inode ref count.
            file_inode->increment_ref_count_atomic(1);

            create_event<file*>(w, f);
            return;
        }

        // Get inode number from file system or device.
        if (!cleaned_file_name.starts_with("/dev/")) {
            // Callback issued once we get a response from the filesystem.
            auto callback = [=](fs::fs_response_t resp) mutable {
                // want better error handling here.
                if (resp.data.open.status == fs::FS_RESP_ERROR_NOT_FOUND) {
                    create_event<file*>(w, nullptr);
                    return;
                }

                // Populate file / inode.
                f->inode = new mem_inode_t(resp.data.open.inode_index, 1);
                f->file_type = FileType::FILESYSTEM;

                // file_name is copied.
                open_files.add(new fileListNode(f->inode, cleaned_file_name));

                // Release list lock.
                file_list_lock->unlock();

                // Create event.
                create_event(w, f);
            };

            fs::issue_fs_request<fs::FS_REQ_OPEN>(callback, cleaned_file_name);
        } else if (cleaned_file_name.starts_with("/dev/ramfs/")) {
            file* f = new file;
            f->file_type = FileType::DEVICE;
            f->name = cleaned_file_name;
            create_event<file*>(w, f);
        } else {
            K::assert(false, "other device files not supported!");
        }
    });
}

void kfclose(File* file) {
    file->inode->increment_ref_count_atomic(-1);
    delete file;
}

void read_fs(File* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {
    K::assert(file->file_type == FileType::FILESYSTEM, "read_fs(): File type is incorrect");

    if (!file->inode) {
        create_event<int>(w, INVALID_FILE);
        return;
    }

    auto callback = [=](fs::fs_response_t resp) {
        if (resp.data.read.status == fs::FS_RESP_SUCCESS) {
            printf("read_fs(): Successfully read %d bytes\n", resp.data.read.bytes_read);
        } else {
            K::assert(false, "read_fs(): Failed to read from file");
        }
        create_event<int>(w, resp.data.read.bytes_read);
    };

    fs::issue_fs_request<fs::FS_REQ_READ>(callback, file->inode->inode_number, buf, offset, n);
}

void write_fs(File* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {
    K::assert(file->file_type == FileType::FILESYSTEM, "write_fs(): File type is incorrect");

    if (!file->inode) {
        create_event<int>(w, INVALID_FILE);
        return;
    }

    auto callback = [=](fs::fs_response_t resp) {
        if (resp.data.write.status == fs::FS_RESP_SUCCESS) {
            printf("write_fs(): Successfully wrote %d bytes\n", resp.data.write.bytes_written);
        }
    };

    fs::issue_fs_request<fs::FS_REQ_WRITE>(callback, file->inode->inode_number, buf, offset, n);
}

void read_dev(File* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {
    bool is_ramfs = false;
    if (is_ramfs) { /* ramfs file */
        int ramfs_index = get_ramfs_index(file->name.c_str());
        if (ramfs_index == -1) {
            create_event<int>(w, INVALID_FILE);
            return;
        }

        uint64_t file_size = ramfs_size(ramfs_index);
        if (offset >= file_size) {
            create_event<int>(w, INVALID_OFFSET);
            return;
        }

        int64_t read_n = K::min(n, file_size - offset);

        int err = ramfs_read(buf, offset, read_n, ramfs_index);
        if (err != 0) {
            create_event<int>(w, OTHER_FAIL);
            return;
        }

        create_event<int>(w, SUCCESSFUL_READ);
        return; /* done reading from ramfs */
    } else {
        create_event<int>(w, NOT_IMPLEMENTED_FS);
        return;
    }
}

/**
 * kread should read a file into the specified buffer by routing a read call to the
 * correct filesystem handler, ie userspace main fs, device read
 */
void kread(File* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {
    // characteristics are in file struct.
    switch (file->file_type) {
        case FileType::DEVICE:
            read_dev(file, offset, buf, n, w);
            break;
        case FileType::FILESYSTEM:
            read_fs(file, offset, buf, n, w);
            break;
        default:
            K::assert(false, "non device file not supported");
    }
}

/**
 * kwrite should write a file from the specified buffer by routing a write call to the
 * correct filesystem handler, ie userspace main fs, device write
 */
void kwrite(File* file, uint64_t offset, char* buf, uint64_t n, Function<void(int)> w) {
    // characteristics are in file struct.
    switch (file->file_type) {
        case FileType::DEVICE:
            K::assert(false, "device write not supported");
            break;
        case FileType::FILESYSTEM:
            write_fs(file, offset, buf, n, w);
            break;
        default:
            K::assert(false, "non device file not supported");
    }
}