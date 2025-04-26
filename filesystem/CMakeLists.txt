cmake_minimum_required(VERSION 3.27)
project(MockFilesys)

set(CMAKE_CXX_STANDARD 17)

#Replaced main.cpp with test class, change back later as needed
add_executable(MockFilesys main.cpp
        interface/FakeDiskDriver.h
        interface/FakeDiskDriver.cpp
        interface/BlockManager.h
        interface/BlockManager.cpp
        filesys/Block.h
        filesys/FileSystem.cpp
        filesys/FileSystem.h
        filesys/BitmapManager.cpp
        filesys/BitmapManager.h
        filesys/InodeTable.cpp
        filesys/InodeTable.h
        filesys/File.cpp
        filesys/File.h
        filesys/Directory.cpp
        filesys/Directory.h
        filesys/LogManager.cpp
        filesys/LogManager.h
        filesys/fs_requests.cpp
        filesys/fs_requests.h
)

target_compile_definitions(MockFilesys PRIVATE NOT_KERNEL)
