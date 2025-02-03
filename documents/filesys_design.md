## Filesystem design overview

- Unified namespace and filesystem to users, accessible from any host
- Each host is a NFS server for local files, and a NFS client for remote files
- All hosts mounted to root, prepended by HWID to ensure unique filepaths
- Passthrough NFS system where each host is the sole authority over file state