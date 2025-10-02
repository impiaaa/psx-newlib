/*** Newlib system calls ***/

// TODO: make errno a macro that calls syscall__get_errno, so that we don't
// have to keep calling it in here
#include <errno.h>
#undef errno
extern int errno;

#ifdef PCSX
static __inline__ void pcsx_checkKernel(int enable) { *((volatile char*)0xBF802088) = enable; }
static __inline__ int pcsx_isCheckingKernel() { return *((volatile char* const)0xBF802088) != 0; }
#ifdef MALLOC_PROVIDED
#include <stdlib.h>
#include <strings.h>
#include <assert.h>
#include <stdio.h>
static __inline__ void pcsx_initMsan() { *((volatile char* const)0xBF802089) = 0; }
static __inline__ void pcsx_resetMsan() { *((volatile char* const)0xBF802089) = 1; }
static __inline__ void* pcsx_msanAlloc(size_t size) {
    register size_t a0 asm("a0") = size;
    void* ret;
    __asm__ volatile("lw %0, 0x208C(%1)" : "=r"(ret) : "r"(0xBF800000), "r"(a0));
    return ret;
}
static __inline__ void pcsx_msanFree(void* ptr) { *((void* volatile* const)0xBF80208C) = ptr; }
static __inline__ void* pcsx_msanRealloc(void* ptr, size_t size) {
    register void* a0 asm("a0") = ptr;
    register size_t a1 asm("a1") = size;
    void* ret;
    __asm__ volatile("lw %0, 0x2090(%1)" : "=r"(ret) : "r"(0xBF800000), "r"(a0), "r"(a1));
    return ret;
}

void* malloc(size_t size) {
    return pcsx_msanAlloc(size);
}
void free(void* ptr) {
    pcsx_msanFree(ptr);
}
void* realloc(void* ptr, size_t size) {
    return pcsx_msanRealloc(ptr, size);
}
void *calloc(size_t n, size_t size) {
    void* ptr = pcsx_msanAlloc(n*size);
    bzero(ptr, n*size);
    return ptr;
}
void* _malloc_r(struct _reent *, size_t size) {
    return pcsx_msanAlloc(size);
}
void _free_r(struct _reent *, void* ptr) {
    pcsx_msanFree(ptr);
}
void* _realloc_r(struct _reent *, void* ptr, size_t size) {
    return pcsx_msanRealloc(ptr, size);
}
void *memalign(size_t align, size_t nbytes) {
    assert(align >= 4);
    return pcsx_msanAlloc(nbytes);
}
#endif // MALLOC_PROVIDED

void software_init_hook(void) {
    int t2 = *(volatile int*)0xBF80101C;
    while (t2<0x80000)
        t2 += 0x10000;
    *(volatile int*)0xBF80101C = t2;
#ifdef MALLOC_PROVIDED
    pcsx_initMsan();
    pcsx_resetMsan();
#endif
    pcsx_checkKernel(1);
}
#else
static __inline__ void pcsx_checkKernel(int enable) { }
static __inline__ int pcsx_isCheckingKernel() { return 0; }
#endif

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

struct DIRENTRY {
    char name[20];
    long attr;
    long size;
    struct DIRENTRY *next;
    long head;
    char system[4];
};
struct iob;
struct device_table {
    char *dt_string;
    int dt_type;
    int dt_bsize;
    char *dt_desc;
    void (*dt_init)(void);
    long (*dt_open)(struct iob *, char *, unsigned long);
    long (*dt_strategy)(struct iob *, long);
    long (*dt_close)(struct iob *);
    long (*dt_ioctl)(struct iob *, long, long);
    long (*dt_read)(struct iob *, void *, long);
    long (*dt_write)(struct iob *, void *, long);
    long (*dt_delete)(struct iob *);
    long (*dt_undelete)(struct iob *);
    struct DIRENTRY * (*dt_firstfile)(struct iob *, const char *, struct DIRENTRY *);
    struct DIRENTRY * (*dt_nextfile)(struct iob *, struct DIRENTRY *);
    long (*dt_format)(struct iob *);
    long (*dt_cd)(struct iob *, char *);
    long (*dt_rename)(struct iob *, char *, struct iob *, char *);
    void (*dt_remove)(void);
    int (*dt_else)(struct iob *, char *);
};
struct iob {
    int i_flgs;
    int i_unit;
    char *i_ma;
    unsigned int i_cc;
    unsigned long i_offset;
    int i_fstype;
    int i_errno;
    struct device_table *i_dp;
    unsigned long i_size;
    long i_head;
    long i_fd;
};
static struct iob *const * fcb_table_ptr = (struct iob *const *)0x140;
static struct device_table *const * dcb_table_ptr = (struct device_table *const *)0x150;

#include <sys/stat.h>
#define makedev(major, minor) (((minor) & 0xffff) | (((major) & 0xffff) << 16))
int fstat(int fd, struct stat *st) {
    if (fd < 0 || fd >= 16) {
        errno = EBADF;
        return -1;
    }
    pcsx_checkKernel(0);
    struct iob *fcb = &(*fcb_table_ptr)[fd];
    if (fcb->i_flgs == 0) {
        errno = ENOENT;
        pcsx_checkKernel(1);
        return -1;
    }
    st->st_dev = makedev(fcb->i_dp - (*dcb_table_ptr), fcb->i_unit);
    st->st_ino = fcb->i_head;
    st->st_mode = 0;
    // TODO check if S_IFDIR
    if (fcb->i_fstype&0x01) {
        // DTTYPE_CHAR character device
        st->st_mode |= S_IFCHR;
    }
    if (fcb->i_fstype&0x02) {
        // DTTYPE_CONS can be console
    }
    if (fcb->i_fstype&0x04) {
        // DTTYPE_BLOCK block device
        //st->st_mode |= S_IFBLK;
    }
    if (fcb->i_fstype&0x08) {
        // DTTYPE_RAW raw device that uses fs switch
    }
    if (fcb->i_fstype&0x10) {
        // DTTYPE_FS
        st->st_mode |= S_IFREG;
    }
    st->st_nlink = 0;
    st->st_uid = 0; // these could be retrieved from directory record SU
    st->st_gid = 0; // section, but dunno if BIOS reads that
    st->st_rdev = (fcb->i_fstype & 0x10) ? 0 : fcb->i_unit;
    st->st_size = fcb->i_size;
    st->st_atime = 0;
    st->st_mtime = 0;
    st->st_ctime = 0; // this could come from directory record
    st->st_blksize = fcb->i_dp->dt_bsize;
    st->st_blocks = fcb->i_dp->dt_bsize == 0 ? 0 : (st->st_size+st->st_blksize-1)/st->st_blksize;
    pcsx_checkKernel(1);
    return 0;
}

static inline int syscall_isatty(int fd) {
    register volatile int n asm("t1") = 0x39;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xB0)(fd);
}
int isatty(int fd) {
    return syscall_isatty(fd);
}

static inline int syscall_lseek(int fd, int offset, int seektype) {
    register volatile int n asm("t1") = 0x33;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int, int, int))0xB0)(fd, offset, seektype);
}
int lseek(int file, int ptr, int dir) {
    if (dir == 2) {
        pcsx_checkKernel(0);
        struct iob *fcb = &(*fcb_table_ptr)[file];
        pcsx_checkKernel(1);
        dir = 0;
        ptr += fcb->i_size;
    }
    int ret = syscall_lseek(file, ptr, dir);
    if (ret < 0) {
        errno = syscall__get_errno();
    }
    return ret;
}

static unsigned flags_posix_to_psx(unsigned flags) {
    unsigned uxread     = flags & 0x0001;
    unsigned uxwrite    = flags & 0x0002;
    unsigned uxappend   = flags & 0x0008;
    unsigned uxcreat    = flags & 0x0200;
    unsigned uxtrunc    = flags & 0x0400;
    unsigned uxsync     = flags & 0x2000;
    unsigned uxnonblock = flags & 0x4000;

    unsigned psread   = uxread;
    unsigned pswrite  = uxwrite;
    unsigned psnblock = uxnonblock >> 12;
    unsigned psappend = uxappend << 5;
    unsigned pscreat  = uxcreat;
    unsigned pstrunc  = uxtrunc;
    unsigned psasync  = uxsync == 0x2000 ? 0 : 0x8000;

    return psread | pswrite | psnblock | psappend | pscreat | pstrunc | psasync;
}

static unsigned flags_psx_to_posix(unsigned flags) {
    unsigned psread   = flags & 0x0001;
    unsigned pswrite  = flags & 0x0002;
    unsigned psnblock = flags & 0x0004;
    unsigned psappend = flags & 0x0100;
    unsigned pscreat  = flags & 0x0200;
    unsigned pstrunc  = flags & 0x0400;
    unsigned psasync  = flags & 0x8000;

    unsigned uxread   = psread;
    unsigned uxwrite  = pswrite;
    unsigned uxnblock = psnblock << 12;
    unsigned uxappend = psappend >> 5;
    unsigned uxcreat  = pscreat;
    unsigned uxtrunc  = pstrunc;
    unsigned uxasync  = psasync == 0x8000 ? 0 : 0x2000;

    return uxread | uxwrite | uxnblock | uxappend | uxcreat | uxtrunc | uxasync;
}

#include <fcntl.h>
#include <stdarg.h>
static inline int syscall_open(const char * filename, int mode) {
    register volatile int n asm("t1") = 0x32;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *, int))0xB0)(filename, mode);
}
int open(const char *name, int flags, ...) {
    va_list args;
    va_start(args, flags);
    int mode = va_arg(args, int);
    int psmode = flags_posix_to_psx(mode & ~3) | ((mode & 3) + 1);
    int ret = syscall_open(name, psmode);
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
    struct device_table *dcb;
    int found = 0;
    int i, device_major_id;
    for (int device_major_id = 0; device_major_id < 10 && !found; device_major_id++) {
        pcsx_checkKernel(0);
        dcb = &(*dcb_table_ptr)[device_major_id];
        pcsx_checkKernel(1);
        if (dcb->dt_string == NULL) {
            continue;
        }
        for (i = 0; i < 6; i++) {
            if (file[i] == ':' || // filesystem devices
                file[i] == '\0' || // character devices
                dcb->dt_string[i] == '\0') // enumerated devices
            {
                found = 1;
                break;
            }
            if (tolower(file[i]) != dcb->dt_string[i]) {
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
    
    if (dcb->dt_firstfile != NULL && file[i] != '\0') {
        struct DIRENTRY dir_e;
        struct iob fcb;
        if (dcb->dt_firstfile(&fcb, file+i+1, &dir_e) == NULL) {
            errno = ENOENT;
            return -1;
        }
        bzero(st, sizeof(struct stat));
        st->st_size = dir_e.size;
        st->st_blocks = (dir_e.size+dcb->dt_bsize-1)/dcb->dt_bsize;
        st->st_dev = makedev(device_major_id, fcb.i_unit);
    }
    else {
        bzero(st, sizeof(struct stat));
        st->st_dev = makedev(device_major_id, 0);
    }
    
    st->st_mode = 0;
    // TODO check if S_IFDIR
    if (dcb->dt_type&0x01) {
        // DTTYPE_CHAR character device
        st->st_mode |= S_IFCHR;
    }
    if (dcb->dt_type&0x02) {
        // DTTYPE_CONS can be console
    }
    if (dcb->dt_type&0x04) {
        // DTTYPE_BLOCK block device
        //st->st_mode |= S_IFBLK;
    }
    if (dcb->dt_type&0x08) {
        // DTTYPE_RAW raw device that uses fs switch
    }
    if (dcb->dt_type&0x10) {
        // DTTYPE_FS
        st->st_mode |= S_IFREG;
    }
    st->st_blksize = dcb->dt_bsize;
    
    return 0;
}

static inline int syscall_write(int fd, char *ptr, int len) {
    register volatile int n asm("t1") = 0x35;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int, char*, int))0xB0)(fd, ptr, len);
}
int write(int file, char *ptr, int len) {
    int ret = syscall_write(file, ptr, len);
#ifdef PCSX
    if (file == 1) {
        return len;
    }
#endif
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
extern char __stack[];
char* _maxstack = __stack;
static const unsigned int* ram_size_ptr = (const unsigned int*)0x60;

void get_mem_info(struct s_mem *mem) {
    // FIXME: get_mem_info is called in crt0 to set up the stack before
    // hardware_init_hook, where we would have detected the memory size
    pcsx_checkKernel(0);
    mem->size = (((*ram_size_ptr)<<20) - 0x10000) - (_end - _ftext) - (__stack - _maxstack);
    pcsx_checkKernel(1);
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
    void (*ra)(void);
    int* sp;
    int* fp;
    int regfile[8];
    int* gp;
} jmp_buf;

static inline jmp_buf* syscall_ResetEntryInt() {
    register volatile int n asm("t1") = 0x18;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((jmp_buf*(*)(void))0xB0)();
}

static inline void syscall_HookEntryInt(jmp_buf* f) {
    register volatile int n asm("t1") = 0x19;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((void(*)(jmp_buf*))0xB0)(f);
}

void hardware_init_hook(void) {
    // crash on out-of-bounds memory access instead of wrapping around and
    // corrupting memory
    // TODO: get amount of memory by checking for wraparound
    // A(B4h) GetSystemInfo just returns ram size global variable
    // though, ld, elf2psexe, & BIOS expect the initial sp to be set at compile time
    syscall_SetMem(2);
    
    // maybe needed for events to work?
    syscall_ResetEntryInt();
    
    // TODO: override read in CD-ROM DCB
    // https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/cdrom/filesystem.c
    // https://github.com/grumpycoders/pcsx-redux/blob/main/src/mips/openbios/cdrom/helpers.c
    // can't patch A(0xA5) / CdReadSector / cdromBlockReading, routine is in ROM
    
    // TODO: enable and test memcard, perhaps add patches
    
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

int fcntl(int fd, int op, ...) {
    va_list args;
    int arg;
    if (fd < 0 || fd >= 16) {
        errno = EBADF;
        return -1;
    }
    pcsx_checkKernel(0);
    struct iob *fcb = &(*fcb_table_ptr)[fd];
    switch (op) {
        case F_DUPFD:
        case F_SETFD:
        case F_SETFL:
#if __BSD_VISIBLE || __POSIX_VISIBLE >= 200112
        case F_SETOWN:
#endif /* __BSD_VISIBLE || __POSIX_VISIBLE >= 200112 */
        case F_GETLK:
        case F_SETLK:
        case F_SETLKW:
#if __POSIX_VISIBLE >= 200809
        case F_DUPFD_CLOEXEC:
#endif
            va_start(args, op);
            arg = va_arg(args, int);
            break;
    }
    int ret;
    switch (op) {
        //case F_DUPFD:
        // TODO
        //    break;
        case F_GETFD:
            ret = 0;
            break;
        case F_SETFD:
            ret = 0;
            break;
        case F_GETFL:
            ret = flags_psx_to_posix(fcb->i_flgs);
            break;
        case F_SETFL:
            fcb->i_flgs = flags_posix_to_psx(arg);
            ret = 0;
            break;
        default:
            errno = ENOSYS;
            ret = -1;
            break;
    }
    pcsx_checkKernel(1);
    return ret;
}

