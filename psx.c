/*** Newlib system calls ***/

#include <errno.h>
#undef errno
extern int errno;

// I've checked to make sure that all error codes returned by the BIOS line up
// with the error codes defined in Newlib
static inline int syscall__get_errno(void) {
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
        errno = syscall__get_errno();
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
typedef struct _dcb t_dcb;
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
struct _dcb {
    char* name_lower;
    int flags;
    int sector_size;
    char* descr;
    void (*init)(void);
    int (*open)(t_fcb*,char*,int);
    int (*in_out)(t_fcb*,int);
    int (*close)(t_fcb*);
    int (*ioctl)(t_fcb*,int,int);
    int (*read)(t_fcb*,void*,int);
    int (*write)(t_fcb*,void*,int);
    int (*erase)(t_fcb*,char*);
    int (*undelete)(t_fcb*,char*);
    t_direntry* (*firstfile)(t_fcb*,const char*,t_direntry*);
    t_direntry* (*nextfile)(t_fcb*,t_direntry*);
    int (*format)(t_fcb*);
    void (*chdir)(t_fcb*,const char *);
    void (*rename)(t_fcb*,const char *,t_fcb*,const char *);
    void (*deinit)(void);
    void (*testdevice)(t_fcb*);
};
static t_fcb *const * fcb_table_ptr = (t_fcb *const *)0x140;
static t_dcb *const * dcb_table_ptr = (t_dcb *const *)0x150;

#include <sys/stat.h>
#define makedev(major, minor) (((minor) & 0xffff) | (((major) & 0xffff) << 16))
int fstat(int fd, struct stat *st) {
    if (fd < 0 || fd >= 16) {
        errno = EBADF;
        return -1;
    }
    t_fcb *fcb = &(*fcb_table_ptr)[fd];
    if (fcb->access_mode == 0) {
        errno = ENOENT;
        return -1;
    }
    st->st_dev = makedev(fcb->dcb - (*dcb_table_ptr), fcb->device_id);
    st->st_ino = fcb->logical_block_number;
    st->st_mode = (fcb->flags & 1) ? S_IFCHR : S_IFREG; // TODO check if S_IFDIR
    st->st_nlink = 0;
    st->st_uid = 0; // these could be retrieved from directory record SU
    st->st_gid = 0; // section, but dunno if BIOS reads that
    st->st_rdev = (fcb->flags & 1) ? fcb->device_id : 0;
    st->st_size = fcb->file_size;
    st->st_atime = 0;
    st->st_mtime = 0;
    st->st_ctime = 0; // this could come from directory record
    st->st_blksize = fcb->dcb->sector_size;
    st->st_blocks = (st->st_size+st->st_blksize-1)/st->st_blksize;
    return 0;
}

static inline int syscall_get_device_flag(int fd) {
    register volatile int n asm("t1") = 0x39;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xB0)(fd);
}
int isatty(int fd) {
    return syscall_get_device_flag(fd);
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
        errno = syscall__get_errno();
    }
    return ret;
}

static inline int syscall_open(const char * filename, int mode) {
    register volatile int n asm("t1") = 0x32;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *, int))0xB0)(filename, mode);
}
int open(const char *name, int flags, int mode) {
    int access   = flags & 3;
    int append   = flags & 0x00008;
    int creat    = flags & 0x00200;
    int excl     = flags & 0x00800;
    int ndelay   = flags & 0x01000;
    int sync     = flags & 0x02000;
    int nonblock = flags & 0x04000;
    int noctty   = flags & 0x08000;
    int direct   = flags & 0x80000;
    
    int psaccess = access + 1;
    int psnblock = (ndelay >> 10) | (nonblock >> 12);
    int pscreat  = creat;
    int psnobuf  = direct >> 5; // unused
    int psnowait = (~(sync >> 2)) & 0x8000;
    int ret = syscall_open(name, psaccess|psnblock|pscreat|psnobuf|psnowait);
    if (ret < 0) {
        errno = syscall__get_errno();
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
        errno = syscall__get_errno();
    }
    return ret;
}

#include <ctype.h> // tolower
#include <strings.h> // bzero
#include <stddef.h> // NULL
int stat(const char *file, struct stat *st) {
    t_dcb *dcb;
    int found = 0;
    int i, device_major_id;
    for (int device_major_id = 0; device_major_id < 10 && !found; device_major_id++) {
        dcb = &(*dcb_table_ptr)[device_major_id];
        if (dcb->name_lower == NULL) {
            continue;
        }
        for (i = 0; i < 6; i++) {
            if (file[i] == ':' || // filesystem devices
                file[i] == '\0' || // character devices
                dcb->name_lower[i] == '\0') // enumerated devices
            {
                found = 1;
                break;
            }
            if (tolower(file[i]) != dcb->name_lower[i]) {
                found = 0;
                break;
            }
        }
    }
    
    if (found == 0) {
        errno = EINVAL;
        return -1;
    }
    
    // trim off device name
    while (file[i] != ':' && file[i] != '\0') {
        i++;
    }
    
    if (dcb->firstfile != NULL && file[i] != '\0') {
        t_direntry dir_e;
        t_fcb fcb;
        if (dcb->firstfile(&fcb, file+i+1, &dir_e) == NULL) {
            errno = ENOENT;
            return -1;
        }
        bzero(st, sizeof(struct stat));
        st->st_size = dir_e.size;
        st->st_blocks = (dir_e.size+dcb->sector_size-1)/dcb->sector_size;
        st->st_dev = makedev(device_major_id, fcb.device_id);
    }
    else {
        bzero(st, sizeof(struct stat));
        st->st_dev = makedev(device_major_id, 0);
    }
    
    // TODO check if S_IFDIR
    if (dcb->flags&0x01) {
        st->st_mode = S_IFCHR;
    }
    else {
        st->st_mode = S_IFREG;
    }
    st->st_blksize = dcb->sector_size;
    
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
        errno = syscall__get_errno();
    }
    return ret;
}

static inline int syscall_erase(char* filename) {
    register volatile int n asm("t1") = 0x45;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *))0xB0)(filename);
}
int unlink(char *name) {
    int ret = syscall_erase(name)-1;
    if (ret < 0) {
        errno = syscall__get_errno();
    }
    return ret;
}

/*** Unsupported by hardware ***/

#if 1
// Establish a new name for an existing file, a.k.a. "hardlink."
int link(char *existing, char *new) {
    errno = ENOSYS;
    return -1;
}

#include <sys/time.h>
#include <sys/times.h>

struct timeval;

int gettimeofday(struct timeval *ptimeval, void *ptimezone) {
    errno = ENOSYS;
    return -1;
}
#endif

/*** Required by various things ***/

struct s_mem {
    unsigned int size;
    unsigned int icsize; // unused
    unsigned int dcsize; // unused
};

extern char _ftext[];
extern char _end[];
static const unsigned int* ram_size_ptr = (const unsigned int*)0x60;

void get_mem_info(struct s_mem *mem) {
    mem->size = (((*ram_size_ptr)<<15) - 0x10000) - (_end - _ftext);
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

static inline int syscall_SetMem(int megabytes) {
    register volatile int n asm("t1") = 0x9F;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xA0)(megabytes);
}

typedef struct {
    void (*function)(void);
    int* sp;
    int* fp;
    int general[8];
    int* gp;
} t_functionState;

static inline t_functionState* syscall_ResetEntryInt() {
    register volatile int n asm("t1") = 0x18;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((t_functionState*(*)(void))0xB0)();
}

static inline void syscall_HookEntryInt(t_functionState* f) {
    register volatile int n asm("t1") = 0x19;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((void(*)(t_functionState*))0xB0)(f);
}

void hardware_init_hook(void) {
    // crash on out-of-bounds memory access instead of wrapping around and
    // corrupting memory
    syscall_SetMem(2);
    
    // maybe needed for events to work?
    syscall_ResetEntryInt();
    
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
        errno = syscall__get_errno();
    }
    return ret;
}

/*
// supplied by crt0
// TODO uncomment when custom crt0
static inline void syscall__exit(int code) {
    register volatile int n asm("t1") = 0x3A;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    ((void(*)(int))0xA0)(code);
}
void _exit(int code) {
    syscall__exit(code);
    __builtin_unreachable();
}
*/

static inline void syscall_puts(char *ptr) {
    register volatile int n asm("t1") = 0x3F;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    ((void(*)(char*))0xB0)(ptr);
}
void print(char *ptr) {
    syscall_puts(ptr);
}

static inline int syscall_cd(const char* dirname) {
    register volatile int n asm("t1") = 0x40;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *))0xB0)(dirname);
}
int chdir(const char *dirname) {
    int ret = syscall_cd(dirname)-1;
    if (ret < 0) {
        errno = syscall__get_errno();
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
        errno = syscall__get_errno();
    }
    return ret;
}

