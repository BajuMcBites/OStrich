//
// Created by caleb on 4/16/2025.
//

#include "fs_requests.h"

namespace fs {
    void init(BlockManager* block_manager) {
        fileSystem = FileSystem::getInstance(block_manager);
    }

    fs_response_t fs_req_add_dir(fs_req_t* req) {
        fs_response_t resp;
        inode_index_t dir_inode_num =  req->data.add_dir.dir;
        Directory parent_dir = Directory(dir_inode_num, fileSystem->inodeTable, fileSystem->inodeBitmap,
                fileSystem->blockBitmap, fileSystem->blockManager, fileSystem->logManager);
        parent_dir.addDirectoryEntry(req->data.add_dir.name, req->data.add_dir.file_to_add);

        resp.req_type = FS_REQ_ADD_DIR;
        resp.data.add_dir.status = FS_RESP_SUCCESS;
        return resp;
    }

    fs_response_t fs_req_create_file(fs_req_t *req) {
        fs_response_t resp;
        inode_index_t dir_inode_num = req->data.create.cwd;
        Directory parent_dir = Directory(dir_inode_num, fileSystem->inodeTable, fileSystem->inodeBitmap,
                fileSystem->blockBitmap, fileSystem->blockManager, fileSystem->logManager);
        //TODO need to modify createFile()/createDirectory to take permissions and stuff

        void* newFile = nullptr;
        if (req->data.create.is_dir) {
            newFile = parent_dir.createDirectory(req->data.create.name);
        }
        else {
            newFile = parent_dir.createFile(req->data.create.name);
        }




        if (newFile == nullptr) {
            resp.data.create_file.status = FS_RESP_ERROR_EXISTS;
        } else {
            resp.data.create_file.status = FS_RESP_SUCCESS;
            if (req->data.create.is_dir) {
                resp.data.create_file.inode_index = (static_cast<Directory*>(newFile))->getInodeNumber();
            }
            else {
                resp.data.create_file.inode_index = (static_cast<File*>(newFile))->getInodeNumber();
            }
        }
        resp.req_type = FS_REQ_CREATE_FILE;
        return resp;
    }

    fs_response_t fs_req_remove_file(fs_req_t *req) {
        fs_response_t resp;
        inode_index_t dir_inode_num = req->data.remove.inode_index;
        Directory parent_dir = Directory(dir_inode_num, fileSystem->inodeTable, fileSystem->inodeBitmap,
                fileSystem->blockBitmap, fileSystem->blockManager, fileSystem->logManager);
        parent_dir.removeDirectoryEntry(req->data.remove.name);
        resp.req_type = FS_REQ_REMOVE_FILE;
        resp.data.remove_file.status = FS_RESP_SUCCESS;
        return resp;
    }





}

