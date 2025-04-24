#include "file_table.h"

FileTable::FileTable() {
    files = std::vector<KFile*>(MAX_OPEN_FILES);
}

FileTable::~FileTable() {
    for (int i = 0; i < files.size(); i++) {
        delete files[i];
    }
}

KFile* FileTable::get_file(int fd) {
    return files[fd];
}

int FileTable::add_file(KFile* file) {
    files.push_back(file);
    return files.size() - 1;
}

void FileTable::remove_file(int fd) {
    delete files[fd];
    files[fd] = nullptr;
}
