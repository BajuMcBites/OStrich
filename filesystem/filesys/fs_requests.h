#ifndef FS_REQUESTS_H
#define FS_REQUESTS_H

#include "Block.h"
#include "Directory.h"
#include "FileSystem.h"
#include "../interface/BlockManager.h"
//#include "ring_buffer.h"

namespace fs {
    // Singleton instance of FileSystem
    inline FileSystem* fileSystem;

    // Function to initialize the filesystem - must be run before calling any other function and takes a BlockManager
    void init(BlockManager* block_manager);



    // Enum for different request types
    typedef enum {
        FS_REQ_ADD_DIR,
        FS_REQ_CREATE_FILE,
        FS_REQ_REMOVE_FILE,
        FS_REQ_READ_DIR,
        FS_REQ_OPEN,
        FS_REQ_WRITE
    } fs_req_type_t;

    // Enum for different response status codes
    typedef enum {
        FS_RESP_SUCCESS,
        FS_RESP_ERROR_NOT_FOUND,
        FS_RESP_ERROR_EXISTS,
        FS_RESP_ERROR_PERMISSION,
        FS_RESP_ERROR_INVALID,
        FS_RESP_ERROR_FULL
    } fs_resp_status_t;

    // Structures for different request types
    typedef struct {
        inode_index_t dir;
        inode_index_t file_to_add;
        char *name;
    } fs_req_add_dir_t;

    typedef struct {
        inode_index_t cwd;
        bool is_dir;
        char *name;
        uint16_t permissions;
    } fs_req_create_file_t;

    typedef struct {
        inode_index_t inode_index;
        char *name; // TODO: remove this later, we wanna use the inode index instead.
    } fs_req_remove_file_t;

    typedef struct {
        inode_index_t inode_index;
    } fs_req_read_dir_t;

    typedef struct {
        char *path;
    } fs_req_open_t;

    typedef struct {
        inode_index_t inode_index;
        char *buf;
        int offset;
        int n_bytes;
    } fs_req_write_t;

    typedef struct {
        inode_index_t inode_index;
        char *buf;
        int offset;
        int n_bytes;
    } fs_req_read_t;

    // Union of all request types
    typedef union {
        fs_req_add_dir_t add_dir;
        fs_req_create_file_t create;
        fs_req_remove_file_t remove;
        fs_req_read_dir_t read_dir;
        fs_req_open_t open;
        fs_req_write_t write;
    } fs_req_data_t;

    // Structures for different response types
    typedef struct {
        fs_resp_status_t status;
    } fs_resp_add_dir_t;

    typedef struct {
        fs_resp_status_t status;
        inode_index_t inode_index;
    } fs_resp_create_file_t;

    typedef struct {
        fs_resp_status_t status;
    } fs_resp_remove_file_t;

    typedef struct {
        fs_resp_status_t status;
        int entry_count;
        dirEntry_t entries[DIRECTORY_ENTRIES_PER_BLOCK];
    } fs_resp_read_dir_t;

    typedef struct {
        fs_resp_status_t status;
        inode_index_t inode_index;
        uint16_t permissions;
    } fs_resp_open_t;

    typedef struct {
        fs_resp_status_t status;
        int bytes_written;
    } fs_resp_write_t;

    typedef struct {
        fs_resp_status_t status;
        int bytes_read;
    } fs_resp_read_t;

    // Union of all response types
    typedef union {
        fs_resp_add_dir_t add_dir;
        fs_resp_create_file_t create_file;
        fs_resp_remove_file_t remove_file;
        fs_resp_read_dir_t read_dir;
        fs_resp_open_t open;
        fs_resp_write_t write;
        fs_resp_read_t read;
    } fs_response_data_t;

    // Main request structure
    typedef struct {
        fs_req_type_t req_type;
        fs_req_data_t data;
        // Private ring buffer would be implementation-specific
        void *ring_buffer;
    } fs_req_t;

    // Main response structure
    typedef struct {
        fs_req_type_t req_type;
        fs_response_data_t data;
    } fs_response_t;




    // Function prototypes
    fs_response_t fs_req_add_dir(fs_req_t *req);

    fs_response_t fs_req_create_file(fs_req_t *req);

    fs_response_t fs_req_remove_file(fs_req_t *req);

    fs_response_t fs_req_read_dir(fs_req_t *req);

    fs_response_t fs_req_open(fs_req_t *req);

    fs_response_t fs_req_write(fs_req_t *req);

    fs_response_t fs_req_read(fs_req_t *req);
} // namespace fs

#endif // FS_REQUESTS_H
