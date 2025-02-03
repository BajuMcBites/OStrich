## Filesystem design overview

- Unified namespace and filesystem to users, accessible from any host
- Each host is a NFS server for local files, and a NFS client for remote files
- All hosts mounted to root, prepended by special character and HWID to ensure unique filepaths
  - we may have local filesystem appear to start at / for local programs, but the special character and local HWID are prepended in the actual file system. Hopefully this ensures better compatibility with existing programs.
- Passthrough NFS system where each host is the sole authority over file state (may implement local caching for performance later)
- Discovery phase during bootup to find other OStrich instances, communicate, and mount files
- When a Pi is disconnected, prune the file tree locally and kill processes that try to access the pruned files (similar behavior to if you unplugged a USB flash drive)
  - we can detect connection failure with a simple heartbeat monitor, or use raft if we want all hosts to agree on connection failure before any pruning action


## Journaling (WIP)
- Each local filesystem will implement write-ahead logging through a journal. The level of logging is undecided at the time (writeback mode, ordered journaling mode, data journaling mode).
- We may follow a system similar to ext3 compound transactions, where we can bundle multiple small updates to the same location as a single transaction to speed up performance.

## IPC interface for file system calls (WIP)
- readdir
- rmdir
- mkdir
- filesize
- remove
- create
- read
- Write
