#include <stdint.h>
#include <stdlib.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>

#define ramfs_id 0xf99482ea8f5ecce2

struct ramfs_file {
    char file_name[16];
    uint64_t size;
    uint64_t start;
};

int copy_file_contents(FILE* dest, FILE* src) {
    unsigned char buffer[4096];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        size_t bytes_written = fwrite(buffer, 1, bytes_read, dest);

        if (bytes_written != bytes_read) {
            printf("error copying files");
            return -1;
        }
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {  // Check if at least one filename is provided
        std::cerr << "Usage: " << argv[0] << " <file1> <file2> ... <fileN>" << std::endl;
        return 1;  // Indicate an error
    }

    char ramfs_signiture[8];
    *((uint64_t*)ramfs_signiture) = ramfs_id;

    std::vector<std::string> filenames;
    printf("building ramfs.img:\n");
    for (int i = 1; i < argc; ++i) {
        filenames.push_back(argv[i]);
        printf("---%s\n", argv[i]);
    }

    ramfs_file* ramfs_file_structs = new ramfs_file[filenames.size()];
    FILE** files = new FILE*[filenames.size()];
    uint64_t total_size = 0;

    for (int i = 0; i < filenames.size(); i++) {
        ramfs_file* curr_file = ramfs_file_structs + i;
        strncpy(curr_file->file_name, filenames[i].c_str(), 16);
        curr_file->file_name[15] = '\0';

        curr_file->start = total_size;

        FILE* file = fopen(filenames[i].c_str(), "rb");
        *(files + i) = file;

        if (file == NULL) {
            printf("error opening file %s\n", filenames[i].c_str());
            return -1;
        }

        if (fseek(file, 0, SEEK_END) != 0) {  // Seek to the end
            printf("error seeking file %s", filenames[i].c_str());
            fclose(file);
            return -1;
        }

        long size = ftell(file);  // Get the current position (file size)

        if (fseek(file, 0, SEEK_SET) != 0) {  // SEEK_SET for beginning
            printf("error seeking to beginning file %s", filenames[i].c_str());
            fclose(file);
            return -1;
        }

        curr_file->size = size;
        total_size += size;
    }

    const char* filename = "../ramfs.img";
    FILE* out_file = fopen(filename, "wb");
    uint64_t file_count = filenames.size();

    uint64_t bytes_written = fwrite(ramfs_signiture, 1, 8, out_file);
    bytes_written += fwrite(&file_count, 1, 8, out_file);
    bytes_written +=
        fwrite((char*)ramfs_file_structs, 1, sizeof(ramfs_file) * filenames.size(), out_file);

    for (int i = 0; i < filenames.size(); i++) {
        if (copy_file_contents(out_file, *(files + i)) != 0) {
            return -1;
        }
        fclose(*(files + i));
    }

    fclose(out_file);

    return 0;
}