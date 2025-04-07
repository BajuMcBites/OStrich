#include <errno.h>
#include <stdio.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>

// --- Process Control ---
// exit -- Exit a program without cleaning up files
void _exit(int val) {
    unsigned int reset = 256;
    void (*func_ptr)(unsigned int) = (void (*)(unsigned int))0x00100068;
    func_ptr(reset);
    // trap to kernel and call syscall number 5
}

// execve -- Transfer control to a new process
// Minimal implementation
int _execve(char *name, char **argv, char **env) {
    errno = ENOMEM;
    return -1;
}

// environ - A pointer to a list of environment variables and their values
// Minimal implementation
char *__env[1] = {0};
char **environ = __env;

// getpid -- Process-ID
// Return 1 by default
int _getpid(void) {
    return 1;
}

// fork -- Create a new process
// Minimal implementation
int _fork(void) {
    errno = ENOTSUP;  // EAGAIN?
    return -1;
}

// kill -- Send a signal
int _kill(int pid, int sig) {
    if (pid == 1) _exit(sig);

    errno = EINVAL;
    return -1;
}

// wait -- Wait for a child process
// Minimal implementation
int _wait(int *status) {
    errno = ECHILD;
    return -1;
}

// --- I/O ---

// isatty - Query whether output stream is a terminal
int _isatty(int fd) {
    if (fd == 0 || fd == 1 || fd == 2)
        return 1;
    else
        return 0;
}

// close - Close a file
// Minimal implementation
int _close(int file) {
    return -1;
}

// link - Establish a new name for an existing file
// Minimal implementation
int _link(char *old, char *new) {
    errno = EMLINK;
    return -1;
}

// lseek - Set position in a file
// Minimal implementation
int _lseek(int file, int ptr, int dir) {
    return -1;
}

// open - Open a file
// Minimal implementation
int _open(const char *name, int flags, ...) {
    return -1;
}

// read - Read from a file
int _read(int file, char *ptr, int len) {
    if (file == 0)  // STDIN
    {
        int (*func_ptr)(char *, int) = (int (*)(char *, int))0x00100010;
        len = func_ptr(ptr, len);
        ptr[len] = '\n';   // BareMetal does not add a newline after keyboard input ...
        ptr[len + 1] = 0;  // ... but C expects it.
        len += 1;
    }
    return len;
}

// write - Write to a file
int _write(int file, char *ptr, int len) {
    // asm (...)
    if (file == 1 || file == 2)  // STDOUT = 1, STDERR = 2
    {
        void (*func_ptr)(char *, int) = (void (*)(char *, int))0x00100018;
        func_ptr(ptr, len);
    } else {
        // File!
        return -1;
    }
    return len;
}

// fstat - Status of an open file.
// Minimal implementation
int _fstat(int file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

// stat - Status of a file
// Minimal implementation
int _stat(const char *file, struct stat *st) {
    st->st_mode = S_IFCHR;
    return 0;
}

// unlink - Remove a file's directory entry
int _unlink(char *name) {
    errno = ENOENT;
    return -1;
}

// --- Memory ---

/* _end is set in the linker command file */
// extern caddr_t _end;

// #define PAGE_SIZE 2097152ULL
// #define PAGE_MASK 0xFFFFFFFFFFE00000ULL
// #define HEAP_ADDR (((unsigned long long)&_end + PAGE_SIZE) & PAGE_MASK)

/*
 * sbrk -- changes heap size size. Get nbytes more
 *         RAM. We just increment a pointer in what's
 *         left of memory on the board.
 */
// sbrk - Increase program data space

caddr_t _sbrk(int incr) {
    //	asm volatile ("xchg %bx, %bx"); // Debug
    extern caddr_t __bss_end__; /* Defined by the linker */
    static caddr_t *heap_end;
    caddr_t *prev_heap_end;
    //	write (2, "sbrk\n", 5);
    if (heap_end == 0) {
        //		write (2, "sbrk end\n", 9);
        heap_end = &__bss_end__;
    }
    prev_heap_end = heap_end;
    //	if (heap_end + incr > stack_ptr) {
    //		write (2, "Heap and stack collision\n", 25);
    //		abort ();
    //	}
    heap_end += incr;
    return (caddr_t)prev_heap_end;
}
caddr_t sbrk(int incr) {
    return _sbrk(incr);
}

// --- Other ---

// gettimeofday --
int _gettimeofday(struct timeval *p, void *z) {
    // 	unsigned char bcd;
    // 	struct tm t;

    // //	outportbyte(0x70, 0x32); // Century
    // //	bcd = inportbyte(0x71);
    // 	outportbyte(0x70, 0x09); // Year
    // 	bcd = inportbyte(0x71);
    // 	t.tm_year = 100 + ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F); // Years since
    // 1900 	outportbyte(0x70, 0x08); // Month 	bcd = inportbyte(0x71); 	t.tm_mon =
    // (((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F)) - 1; // Months since January
    // outportbyte(0x70, 0x07); // Day 	bcd = inportbyte(0x71); 	t.tm_mday = ((bcd & 0xF0) >>
    // 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F); 	outportbyte(0x70, 0x04); // Hour 	bcd
    // = inportbyte(0x71); 	t.tm_hour = ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd &
    // 0x0F); 	outportbyte(0x70, 0x02); // Minute 	bcd = inportbyte(0x71); 	t.tm_min =
    // ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F); 	outportbyte(0x70, 0x00); //
    // Second 	bcd = inportbyte(0x71); 	t.tm_sec = ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3)
    // + (bcd & 0x0F); 	t.tm_isdst = -1;

    // 	p->tv_sec = (long) mktime(&t);
    // 	p->tv_usec = 0;
    return 0;
}

// times - Timing information for current process.
clock_t _times(struct tms *buf) {
    // get current process time
    unsigned long long (*func_ptr)(void) = (unsigned long long (*)(void))0x00100060;
    unsigned long long proc_time = func_ptr();

    /*
     * Process time is assumed to be the CPU time charged for
     * the execution of user instructions of the calling process.
     * This is not necessary accurate since CPU time may also be
     * charged for execution by the system on behalf of the calling
     * process (i.e. when a syscall is executed); the ability to
     * differentiate user and system time should be added in future
     * development.
     */
    buf->tms_utime = proc_time;
    buf->tms_stime = 0;
    buf->tms_cutime = 0;
    buf->tms_cutime = 0;

    return proc_time;
}

// NOTE:
// I'm still working on understanding how this works and whether we need this.
// For now, I'm going to leave this here it links to libc.
/*
Called like so:

Iterate over all the init routines.
void __libc_init_array (void)
{
  size_t count;
  size_t i;

  count = __preinit_array_end - __preinit_array_start;
  for (i = 0; i < count; i++)
    __preinit_array_start[i] ();

  _init ();

  count = __init_array_end - __init_array_start;
  for (i = 0; i < count; i++)
    __init_array_start[i] ();
}
*/
void _init() {
}

/*
Called like so:

void __libc_fini_array (void)
{
  size_t count;
  size_t i;

  count = __fini_array_end - __fini_array_start;
  for (i = count; i > 0; i--)
    __fini_array_start[i-1] ();

  _fini ();
}
*/
void _fini() {
}