#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>


extern int _open(const char* pathname, int flags, mode_t mode);
extern int _close(int fd);
extern int _write(int fd, const void* buf, size_t count);
extern int _read(int fd, void* buf, size_t count);

char str_buf[1<<13]; // :skull:
char cwd[1024];
int ptr = 0;
char** argv[16];

const int num_users = 3;

char* users[3] = {"root", "user1", "user2"};
char* pws[3] = {"very_secure_password", "password1", "password2"};

void puts_concat(int len, ...) {
    va_list al;
    va_start(al,len);
    for (int i = 0; i < len; i++) {
        char* str=va_arg(al, char *);
        int str_len = strlen(str);
        write(1, str, str_len);
    }
}

int getln(char** buf, size_t* len) {
    size_t buf_size = 256;
    char* _buf = &str_buf[ptr];
    ptr += buf_size;
    memset(_buf, 0, buf_size);
    int _len = 0;
    read(0, _buf, buf_size);
    for (; _len < buf_size; _len++)
        if (_buf[_len] == '\n' || _buf[_len] == '\r') break;
    *len = _len;
    *buf = _buf;
    return 0;
}

int user_login() {
    puts_concat(1, "User Name: ");
    char *user = NULL;
    size_t user_len = 0; 
    if (getln(&user, &user_len) == -1){
        perror("readline");
        exit(EXIT_FAILURE);
    }
    user[strcspn(user, "\r\n")] = 0;
    char *pw = NULL;
    size_t pw_len = 0; 
    puts_concat(1, "Password: ");
    if (getln(&pw, &pw_len) == -1){
        perror("readline");
        exit(EXIT_FAILURE);
    }
    pw[strcspn(pw, "\r\n")] = 0;

    for (int i = 0; i < num_users; i++) {
        if (strcmp(user, users[i]) == 0 && strcmp(pw, pws[i]) == 0) return i;
    }
    puts_concat(1, "Unsuccessful login!\n");
    return -1;
}

char** parse_exec(char* line) {
    size_t argv_buf_size = 16;
    const char* delim = " \t\r\n\a";
    int argc = 0;
    char* token;
    if (!argv) {
        perror("malloc broken\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, delim);
    while (token != NULL) {
        argv[argc] = token;
        argc++;

        if (argc >= argv_buf_size) {
            if (!argv) {
                perror("too many args");
            }
        }

        token = strtok(NULL, delim);
    }
    argv[argc] = "\0";
    return argv;
}

void execute(char** argv, int redir_in, int redir_out) {
    char* prog_name = argv[0];
    pid_t pid, wpid;
    int status;
    /* if (strcmp(prog_name, "cd") == 0) {
        if (argv[1] == NULL) {
            while (strcmp("/", cwd)) {
                chdir("..");
                getcwd(cwd, sizeof(cwd));  
            }
        } else {
            if (chdir(argv[1]) != 0) {
                perror("could not find directory");
            }
            getcwd(cwd, sizeof(cwd));  
        }
        return;
    }*/
    if (strcmp(prog_name, "open") == 0) {
        char* pathname = argv[1];
        int flags = atoi(argv[2]);
        int mode = atoi(argv[3]);
        _open(pathname, flags, mode);

        return;
    } else if (strcmp(prog_name, "close") == 0) {
        int fd = atoi(argv[1]);
        _close(fd);
        return;
    } else if (strcmp(prog_name, "write") == 0) {
        int fd = atoi(argv[1]);
        char* buf = argv[2];
        int count = atoi(argv[3]);
        _write(fd, buf, count);
        return;
    } else if (strcmp(prog_name, "read") == 0) {
        char* buf = &str_buf[ptr];
        ptr += 256;
        int fd = atoi(argv[1]);
        int count = atoi(argv[2]);
        _read(fd, buf, count);
        puts_concat(1, buf);
        return;
    } else if (strcmp(prog_name, "checkpoint") == 0) {
        checkpoint();
        return;
    } else if (strcmp(prog_name, "mount") == 0) {
        int ckptid = atoi(argv[1]);
        mount(ckptid);
        return;
    } else if (strcmp(prog_name, "ls") == 0) {
        ls();
        return;
    } else if (strcmp(prog_name, "delete") == 0) {
        char* path = argv[1];
        del(path);
        return;
    }
    pid = fork();
    if (pid == 0) {
        puts_concat(1, prog_name);
        if (execve(prog_name, argv, 0) == -1) {
            perror("exec");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("fork");
    } else {
        wait(&status);
    }
}

int main() {
    int uid = -1;
    while (uid < 0) uid = user_login();
    // chdir("~");
    // getcwd(cwd, sizeof(cwd));  
    while (1) {
        puts_concat(2, cwd, "$ ");
        // read
        char *line = NULL;
        size_t bufsize = 0; // have getline allocate a buffer for us

        if (getln(&line, &bufsize) == -1){
            if (feof(stdin)) {
                exit(EXIT_SUCCESS);
            } else  {
                perror("readline");
                exit(EXIT_FAILURE);
            }
        }
        // parse
        int fd_in = 0, fd_out = 0;
        char** argv = parse_exec(line);
        execute(argv, fd_in, fd_out);
    }
}
