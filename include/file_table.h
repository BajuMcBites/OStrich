#ifndef _FILE_TABLE_H_
#define _FILE_TABLE_H_

#include "filesys_compat/vector"
#include "fs.h"

#define MAX_OPEN_FILES 64

class FileTable {
   public:
    FileTable();
    ~FileTable();
    KFile* get_file(int fd);
    int add_file(KFile* file);
    void remove_file(int fd);

   private:
    std::vector<KFile*> files;
};

#endif  // _FILE_TABLE_H_
