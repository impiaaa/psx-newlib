/*** Newlib system calls ***/

#include <errno.h>
#undef errno
extern int errno;

static inline int syscall_get_last_error(void) {
    register volatile int n asm("t1") = 0x54;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(void))0xB0)();
}

static inline int syscall_close(int fd) {
    register volatile int n asm("t1") = 0x36;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xB0)(fd);
}
int close(int file) {
    int ret = syscall_close(file);
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

typedef struct {
    char* name_lower;
    int flags;
    int sector_size;
    char* descr;
} t_dcb;
typedef struct {
    unsigned int access_mode;
    unsigned int device_id;
    char* transfer_buffer;
    int transfer_length;
    int position;
    int flags;
    int error;
    t_dcb* dcb;
    int file_size;
    int logical_block_number;
    int control_block_number;
} t_fcb;
static t_fcb *const * fcb_table_ptr = (t_fcb *const *)0x140;

#include <sys/stat.h>
int fstat(int fd, struct stat *st) {
    if (fd >= 16) {
        errno = EBADF;
        return -1;
    }
    t_fcb *fcb = &(*fcb_table_ptr)[fd];
    if (fcb->access_mode == 0) {
        errno = ENOENT;
        return -1;
    }
    st->st_dev = fcb->device_id;
    st->st_ino = fcb->logical_block_number;
    st->st_mode = (fcb->flags & 1) ? S_IFCHR : S_IFREG; // TODO check if S_IFDIR
    st->st_nlink = 0;
    st->st_uid = 0; // these could be retrieved from directory record SU
    st->st_gid = 0; // section, but dunno if BIOS reads that
    st->st_rdev = 0;
    st->st_size = fcb->file_size;
    st->st_atime = 0;
    st->st_mtime = 0;
    st->st_ctime = 0; // this could come from directory record
    st->st_blksize = fcb->dcb->sector_size;
    st->st_blocks = (st->st_size+st->st_blksize-1)/st->st_blksize;
    return 0;
}

static inline int syscall_get_device_flag() {
    register volatile int n asm("t1") = 0x39;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)())0xB0)();
}
int isatty() {
    return syscall_get_device_flag();
}

// Establish a new name for an existing file, a.k.a. "hardlink." Not supported.
int link(char *existing, char *new) {
    errno = ENOSYS;
    return -1;
}

static inline int syscall_seek(int fd, int offset, int seektype) {
    register volatile int n asm("t1") = 0x33;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int, int, int))0xB0)(fd, offset, seektype);
}
int lseek(int file, int ptr, int dir) {
    if (dir == 2) {
        t_fcb *fcb = &(*fcb_table_ptr)[file];
        dir = 0;
        ptr += fcb->file_size;
    }
    int ret = syscall_seek(file, ptr, dir);
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

static inline int syscall_open(const char * filename, int mode) {
    register volatile int n asm("t1") = 0x32;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *, int))0xB0)(filename, mode);
}
int open(const char *name, int flags, int mode) {
    int access = (flags&3)+1;
    int nowait = 0;
    int create = flags&0x200;
    int noblock = (flags&0x4000)>>1;
    int ret = syscall_open(name, access|nowait|create|noblock);
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

static inline int syscall_read(int fd, char *ptr, int len) {
    register volatile int n asm("t1") = 0x34;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int, char*, int))0xB0)(fd, ptr, len);
}
int read(int file, char *ptr, int len) {
    int ret = syscall_read(file, ptr, len);
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

typedef struct {
    char filename[20];
    int attr;
    int size;
    void* next;
    int sector;
    int unk;
} t_direntry;
static inline t_direntry* syscall_find_first(const char* filename, t_direntry *direntry) {
    register volatile int n asm("t1") = 0x42;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((t_direntry*(*)(const char *, t_direntry *))0xB0)(filename, direntry);
}
extern int strncmp(const char *s1, const char *s2, size_t n);

int stat(const char *file, struct stat *st) {
    t_direntry dir_e;    
    if(syscall_find_first(file, &dir_e) == NULL) {
        errno = ENOENT;
        return -1;
    }
    
    if (strncmp(file, "cdrom:", 6) == 0) {
        st->st_blksize = 2048;
        // TODO check if S_IFDIR
        st->st_mode = S_IFREG;
    }
    else if (strncmp(file, "bu00:", 5) == 0 ||
             strncmp(file, "bu10:", 5) == 0) {
        st->st_blksize = 128;
        st->st_mode = S_IFREG;
    }
    else if (strncmp(file, "tty:", 4) == 0) {
        st->st_blksize = 1;
        st->st_mode = S_IFCHR;
    }
    else {
        errno = EINVAL;
        return -1;
    }
    
    st->st_size = dir_e.size;
    st->st_blocks = (st->st_size+st->st_blksize-1)/st->st_blksize;
    
    return 0;
}

static inline int syscall_write(int fd, char *ptr, int len) {
    register volatile int n asm("t1") = 0x35;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int, char*, int))0xB0)(fd, ptr, len);
}
int write(int file, char *ptr, int len) {
    int ret = syscall_write(file, ptr, len);
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

static inline int syscall_delete(char* filename) {
    register volatile int n asm("t1") = 0x45;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *))0xB0)(filename);
}
int unlink(char *name) {
    int ret = syscall_delete(name)-1;
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

/*** Required by various things ***/

struct s_mem {
    unsigned int size;
    unsigned int icsize;
    unsigned int dcsize;
};

void get_mem_info(struct s_mem *mem) {
    mem->size = 0x1F0000;
}

#include <sys/time.h>
#include <sys/times.h>

struct timeval;

int gettimeofday(struct timeval *ptimeval, void *ptimezone) {
    errno = ENOSYS;
    return -1;
}

static inline int enterCriticalSection() {
    register volatile int n asm("a0") = 1;
    register volatile int r asm("v0");
    __asm__ volatile("syscall\n" : "=r"(n), "=r"(r) : "r"(n) : "at", "v1", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9", "memory");
    return r;
}

static inline void exitCriticalSection() {
    register volatile int n asm("a0") = 2;
    __asm__ volatile("syscall\n" : "=r"(n) : "r"(n) : "at", "v0", "v1", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9", "memory");
}

static inline int syscall_set_memory_size(int megabytes) {
    register volatile int n asm("t1") = 0x9F;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xA0)(megabytes);
}
void hardware_init_hook(void) {
    syscall_set_memory_size(2);
    // needed for BIOS file functions to work
    exitCriticalSection();
}

/*** Not required, but nice to have ***/

static inline int syscall_ioctl(int fd, unsigned long request, void* arg) {
    register volatile int n asm("t1") = 0x37;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int, unsigned long, void*))0xB0)(fd, request, arg);
}
int ioctl(int fd, unsigned long request, void* arg) {
    int ret = syscall_ioctl(fd, request, arg);
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

/*
// supplied by crt0
// TODO uncomment when custom crt0
__attribute__ ((noreturn)) static inline void syscall_exit(int code) {
    register volatile int n asm("t1") = 0x38;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    // I have no idea how to get rid of the "'noreturn' function does return" warning here
    ((void(*)(int))0xB0)(code);
}
__attribute__ ((noreturn)) void _exit(int code) {
    syscall_exit(code);
}
*/

static inline void syscall_print(char *ptr) {
    register volatile int n asm("t1") = 0x3F;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    ((void(*)(char*))0xB0)(ptr);
}
void print(char *ptr) {
    syscall_print(ptr);
}

static inline int syscall_chdir(const char* dirname) {
    register volatile int n asm("t1") = 0x40;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *))0xB0)(dirname);
}
int chdir(const char *dirname) {
    int ret = syscall_chdir(dirname)-1;
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

static inline int syscall_rename(const char* oldpath, const char* newpath) {
    register volatile int n asm("t1") = 0x44;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *, const char *))0xB0)(oldpath, newpath);
}
int _rename(const char *oldpath, const char *newpath) {
    int ret = syscall_rename(oldpath, newpath)-1;
    if (ret < 0) {
        errno = syscall_get_last_error();
    }
    return ret;
}

