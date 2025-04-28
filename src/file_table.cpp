#include "file_table.h"

#include "event.h"

void FileTable::add_file(KFile* file, int offset, int flags, Function<void(int)> w) {
    if (open_files_ >= MAX_OPEN_FILES) {
        create_event<int>(w, TOO_MANY_OPEN_FILES);
        return;
    }

    // Find an empty slot in the file table
    // Start at fd_counter and wrap around.
    // Using for-loop just to be safe.
    file_table_lock_.lockAndRelease([&, file, w]() {
        for (int i = 0; i < MAX_OPEN_FILES; i++) {
            if (!file_table_[fd_counter_].backing_file()) {
                printf("FileTable::add_file: Adding file to fd %d\n", fd_counter_);
                file_table_[fd_counter_].construct(file, offset, flags);
                open_files_++;
                create_event<int>(w, fd_counter_);
                return;
            }
            fd_counter_ = (fd_counter_ + 1) % MAX_OPEN_FILES;
        }
    });
}

UFile& FileTable::get_file(int fd) {
    printf("FileTable::get_file: fd = %d\n", fd);
    K::assert(fd >= 0 && fd < MAX_OPEN_FILES, "Invalid file descriptor");
    return file_table_[fd];
}

void FileTable::remove_file(int fd, Function<void(int)> w) {
    file_table_lock_.lockAndRelease([=]() {
        UFile& ufile = file_table_[fd];
        kclose(ufile.backing_file());
        ufile.reset();
        open_files_--;
        create_event<int>(w, 0);
    });
}