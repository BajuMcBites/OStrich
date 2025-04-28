#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <fcntl.h>
#include <stdarg.h>


char cwd[1024];

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
    size_t buf_size = 8;
    char* _buf = malloc(buf_size);
    int _len = 0;
    while(1) {
        if (_len > buf_size) {
            buf_size *= 2;
            _buf = (char*)realloc(_buf, buf_size);
        }
        read(0, &_buf[_len], 1);
        if (_buf[_len] == '\n' || _buf[_len] == '\r') break;
        _len++;
    }
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

char** parse_pipes(char* line) {
    char** arr = (char**)malloc(1 * sizeof(char*));
    arr[0] = line;
    return arr;
    // size_t buf_size = 8;
    // const char* delim = "|";
    // int argc = 0;
    // char** lines = (char**)malloc(buf_size * sizeof(char*));
    // char* token;

    // if (!lines) {
    //     perror("malloc broken\n");
    //     exit(EXIT_FAILURE);
    // }

    // token = strtok(line, delim);
    // while (token != NULL) {
    //     lines[argc] = token;
    //     argc++;

    //     if (argc >= buf_size) {
    //         buf_size *= 2; 
    //         lines = (char**)realloc(lines, buf_size * sizeof(char*));
    //         if (!lines) {
    //             perror("malloc broken\n");
    //             exit(EXIT_FAILURE);
    //         }
    //     }

    //     token = strtok(NULL, delim);
    // }
    // lines[argc] = NULL;
    // return lines;
}

char* parse_redir_right(char* line) {
    return NULL;
    // const char* gt = ">";
    // char* token1 = strtok(line, gt);
    // char* token2 = strtok(NULL, gt);
    // if (token2 == NULL) return NULL;
    // else {
    //     const char* delim = " \t\r\n\a<";
    //     char* ret_tok;
    //     ret_tok = strtok(token2, delim);
    //     return ret_tok;
    // }
}

char* parse_redir_left(char* line) {
    return NULL;
    // const char* lt = "<";
    // char* token1 = strtok(line, lt);
    // char* token2 = strtok(NULL, lt);
    // if (token2 == NULL) return NULL;
    // else {
    //     const char* delim = " \t\r\n\a>";
    //     char* ret_tok;
    //     ret_tok = strtok(token2, delim);
    //     return ret_tok;
    // }
}

char** parse_exec(char* line) {
    size_t argv_buf_size = 8;
    const char* delim = " \t\r\n\a";
    int argc = 0;
    char** argv = (char**)malloc(argv_buf_size * sizeof(char*));
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
            argv_buf_size *= 2; 
            argv = (char**)realloc(argv, argv_buf_size * sizeof(char*));
            if (!argv) {
                perror("malloc broken\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, delim);
    }
    argv[argc] = NULL;
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
    pid = fork();
    if (pid == 0) {
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
        char** lines = parse_pipes(line);
        int next_in = 0;
        for (char** line_ptr = lines; line_ptr[0] != NULL; line_ptr++) {
            line = line_ptr[0];
            char* redir_in = parse_redir_left(line);
            char* redir_out = parse_redir_right(line);
            int fd_in = next_in, fd_out = 0;
            if (redir_in) {
                fd_in = open(redir_in, O_WRONLY | O_TRUNC, 0644);
                if (fd_in < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
            }
            if (redir_out) {
                fd_out = open(redir_out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd_out < 0) {
                    perror("open");
                    exit(EXIT_FAILURE);
                }
            }
            // int pipefd[2];
            // if (line_ptr[1] != NULL) {
            //     if (pipe(pipefd) == -1) {
            //         perror("pipe");
            //         exit(EXIT_FAILURE);
            //     }
            //     fd_out = pipefd[1];
            //     next_in = pipefd[0];
            // }
            // delete before <> 
            char* truncated_line = strtok(line, "<>");
            char** argv = parse_exec(truncated_line);
            execute(argv, fd_in, fd_out);
            if (fd_in)
                close(fd_in);
            if (fd_out)
                close(fd_out);
        }
    }
}