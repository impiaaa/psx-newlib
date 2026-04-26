#define BUFFERING
/*** Newlib system calls ***/

// TODO: make errno a macro that calls syscall__get_errno, so that we don't
// have to keep calling it in here
#include <errno.h>
#undef errno
extern int errno;

#ifdef PCSX
static __inline__ void pcsx_checkKernel(int enable) {
    *((volatile char*)0xBF802088) = enable;
    asm volatile("");
}
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
void *_calloc_r(struct _reent *, size_t n, size_t size) {
    void* ptr = pcsx_msanAlloc(n*size);
    bzero(ptr, n*size);
    return ptr;
}
#endif // MALLOC_PROVIDED
#else // PCSX
static __inline__ void pcsx_checkKernel(int enable) { }
static __inline__ int pcsx_isCheckingKernel() { return 0; }
#endif // PCSX

#define ROMCALL(vector, number, returntype, param_sig, parameters) { \
    register volatile int n asm("t1") = number; \
    __asm__ volatile("" : "=r"(n) : "r"(n)); \
    asm("addiu $sp, $sp, -16" ::: "sp"); \
    returntype ret = ((returntype(*)param_sig)vector)parameters; \
    asm("addiu $sp, $sp, 16"); \
    return ret; \
}

#define ROMCALLV(vector, number, param_sig, parameters) { \
    register volatile int n asm("t1") = number; \
    __asm__ volatile("" : "=r"(n) : "r"(n)); \
    asm("addiu $sp, $sp, -16" ::: "sp"); \
    ((void(*)param_sig)vector)parameters; \
    asm("addiu $sp, $sp, 16"); \
}

#define A0CALL(number, returntype, param_sig, parameters) ROMCALL(0xA0, number, returntype, param_sig, parameters)
#define B0CALL(number, returntype, param_sig, parameters) ROMCALL(0xB0, number, returntype, param_sig, parameters)

struct EvCB {
    unsigned long desc;
    long status;
    long spec;
    long mode;
    long (*FHandler)();
    long system[2];
};
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
static struct EvCB *const * evcb_table_ptr = (struct EvCB *const *)0x120;
static size_t * evcb_table_size = (size_t *)0x124;
static struct iob *const * fcb_table_ptr = (struct iob *const *)0x140;
static struct device_table *const * dcb_table_ptr = (struct device_table *const *)0x150;

// I've checked to make sure that all error codes returned by the BIOS line up
// with the error codes defined in Newlib
static inline int syscall__get_errno(void) B0CALL(0x54, int, (void), ())

#include <stdlib.h>

static inline int syscall_close(int fd) B0CALL(0x36, int, (int), (fd))
int close(int file) {
#ifdef BUFFERING
    pcsx_checkKernel(0);
    struct iob *fcb = &(*fcb_table_ptr)[file];
    if ((fcb->i_fstype & 0x04) != 0 && fcb->i_ma == NULL) {
        free(fcb->i_ma);
        fcb->i_ma = NULL;
    }
    pcsx_checkKernel(1);
#endif
    int ret = syscall_close(file);
    if (ret < 0) {
        errno = syscall__get_errno();
    }
    return ret;
}

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

static inline int syscall_isatty(int fd) B0CALL(0x39, int, (int), (fd))
int isatty(int fd) {
    return syscall_isatty(fd);
}

static inline int syscall_lseek(int fd, int offset, int seektype) B0CALL(0x33, int, (int, int, int), (fd, offset, seektype))
int lseek(int file, int ptr, int dir) {
    if (dir == 2) {
        pcsx_checkKernel(0);
        struct iob *fcb = &(*fcb_table_ptr)[file];
        dir = 0;
        ptr += fcb->i_size;
        pcsx_checkKernel(1);
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
    unsigned uxasync    = flags & 0x0040; // not exactly the same
    unsigned uxcreat    = flags & 0x0200;
    unsigned uxtrunc    = flags & 0x0400;
    unsigned uxnonblock = flags & 0x4000;

    unsigned psread   = uxread;
    unsigned pswrite  = uxwrite;
    unsigned psnblock = uxnonblock >> 12;
    unsigned psappend = uxappend << 5;
    unsigned pscreat  = uxcreat;
    unsigned pstrunc  = uxtrunc;
    unsigned psasync  = uxasync << 9;

    return psread | pswrite | psnblock | psappend | pscreat | pstrunc | psasync | (flags & 0xFFFF0000);
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
    unsigned uxasync  = psasync >> 9;

    return uxread | uxwrite | uxnblock | uxappend | uxcreat | uxtrunc | uxasync | (flags & 0xFFFF0000);
}

#include <fcntl.h>
#include <stdarg.h>
static inline int syscall_open(const char * filename, int flags) B0CALL(0x32, int, (const char *, int), (filename, flags))
int open(const char *name, int flags, ...) {
    int psflags = flags_posix_to_psx(flags & ~3) | ((flags & 3) + 1);
    int ret = syscall_open(name, psflags);
    if (ret < 0) {
        errno = syscall__get_errno();
    }
#ifdef BUFFERING
    else {
        pcsx_checkKernel(0);
        struct iob *fcb = &(*fcb_table_ptr)[ret];
        if ((fcb->i_fstype & 0x04) != 0) {
            fcb->i_ma = NULL;
        }
        pcsx_checkKernel(1);
    }
#endif
    return ret;
}

static int syscall_read(int fd, char *ptr, int len) B0CALL(0x34, int, (int, char *, int), (fd, ptr, len))
static int ensure_buffer(struct iob *fcb, unsigned long target_offset) {
    if (fcb->i_cc != target_offset) {
        unsigned long tmp_offset = fcb->i_offset;
        fcb->i_offset = target_offset;
        int ret = syscall_read(fcb->i_fd, fcb->i_ma, fcb->i_dp->dt_bsize);
        if (ret < 0) {
            return ret;
        }
        fcb->i_cc = fcb->i_offset;
        fcb->i_offset = tmp_offset;
    }
    return 0;
}
#include <stddef.h> // NULL
#include <string.h> // memcpy
int read(int file, char *ptr, int len) {
    int total_read = 0;
#ifdef BUFFERING
    pcsx_checkKernel(0);
    struct iob *fcb = &(*fcb_table_ptr)[file];
    if (fcb->i_flgs == 0) {
        errno = ENOENT;
        pcsx_checkKernel(1);
        return -1;
    }

    if ((fcb->i_fstype & 0x04) != 0) {
        if (fcb->i_ma == NULL) {
            fcb->i_ma = malloc(fcb->i_dp->dt_bsize);
            fcb->i_cc = -1;
        }
        int bsize_mask = fcb->i_dp->dt_bsize - 1;
        if ((fcb->i_offset & bsize_mask) != 0) {
            int ret = ensure_buffer(fcb, fcb->i_offset & ~bsize_mask);
            if (ret < 0) {
                errno = syscall__get_errno();
                pcsx_checkKernel(1);
                return ret;
            }
            unsigned long from_buf = fcb->i_dp->dt_bsize - (fcb->i_offset - fcb->i_cc);
            if (len < from_buf) from_buf = len;
            memcpy(ptr, fcb->i_ma + fcb->i_offset - fcb->i_cc, from_buf);
            ptr += from_buf;
            fcb->i_offset += from_buf;
            len -= from_buf;
            total_read += from_buf;
        }
        unsigned long from_dev = len & ~bsize_mask;
        if (from_dev > 0) {
            int ret = syscall_read(file, ptr, from_dev);
            if (ret < 0) {
                errno = syscall__get_errno();
                pcsx_checkKernel(1);
                return ret;
            }
            ptr += from_dev;
            len -= from_dev;
            total_read += from_dev;
        }
        if (len > 0) {
            int ret = ensure_buffer(fcb, fcb->i_offset);
            if (ret < 0) {
                errno = syscall__get_errno();
                pcsx_checkKernel(1);
                return ret;
            }
            memcpy(ptr, fcb->i_ma, len);
            fcb->i_offset += len;
            total_read += len;
        }
    }
    else
#endif
    {
        total_read = syscall_read(file, ptr, len);
        if (total_read < 0) {
            errno = syscall__get_errno();
        }
    }
    pcsx_checkKernel(1);
    return total_read;
}

#include <ctype.h> // tolower
#include <strings.h> // bzero
int stat(const char *file, struct stat *st) {
    pcsx_checkKernel(0);
    struct device_table *dcb;
    int found = 0;
    int i, device_major_id, device_minor_id = 0;
    for (int device_major_id = 0; device_major_id < 10 && !found; device_major_id++) {
        dcb = &(*dcb_table_ptr)[device_major_id];
        if (dcb->dt_string == NULL) {
            continue;
        }
        for (i = 0; i < 6; i++) {
            if (file[i] == ':' || // filesystem devices
                file[i] == '\0' || // character devices
                dcb->dt_string[i] == '\0') // enumerated devices
            {
                if (dcb->dt_string[i] == '\0') {
                    for (; i < 6 && file[i] != ':' && file[i] != '\0'; i++) {
                        char l = tolower(file[i]);
                        device_minor_id = (device_minor_id << 4) | (l >= 'a' ? l - 'a' + 10 : l - '0');
                    }
                }
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
        pcsx_checkKernel(1);
        return -1;
    }
    
    // trim off device name
    while (file[i] != ':' && file[i] != '\0') {
        i++;
    }
    
    if (dcb->dt_firstfile != NULL && file[i] != '\0') {
        struct DIRENTRY dir_e;
        struct iob fcb;
        fcb.i_unit = device_minor_id;
        asm("addiu $sp, $sp, -16" ::: "sp");
        struct DIRENTRY *found = dcb->dt_firstfile(&fcb, file+i+1, &dir_e);
        asm("addiu $sp, $sp, 16");
        if (found == NULL) {
            errno = ENOENT;
            pcsx_checkKernel(1);
            return -1;
        }
        bzero(st, sizeof(struct stat));
        st->st_size = dir_e.size;
        st->st_blocks = (dir_e.size+dcb->dt_bsize-1)/dcb->dt_bsize;
        st->st_ino = dir_e.head;
    }
    else {
        bzero(st, sizeof(struct stat));
    }
    
    st->st_dev = makedev(device_major_id, device_minor_id);
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
    pcsx_checkKernel(1);
    
    return 0;
}

static inline int syscall_write(int fd, char *ptr, int len) B0CALL(0x35, int, (int, char *, int), (fd, ptr, len))
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

static inline int syscall_erase(const char* filename) B0CALL(0x45, int, (const char *), (filename))
int unlink(char *name) {
    int ret = syscall_erase(name)-1;
    if (ret < 0) {
        errno = syscall__get_errno();
    }
    return ret;
}

/*** Unsupported by hardware ***/

#if 0
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
    register char* sp asm("sp");
    if (sp < _maxstack && sp > _end) _maxstack = sp;
    pcsx_checkKernel(0);
    mem->size = (((*ram_size_ptr)<<20) - 0x10000) - (_end - _ftext) - (__stack - _maxstack);
    pcsx_checkKernel(1);
}

static inline long syscall_CdAsyncSeekL(const unsigned char* sector) A0CALL(0x78, long, (const unsigned char*), (sector))

static inline long syscall_CdAsyncReadSector(unsigned count, void* dest, unsigned mode) A0CALL(0x7E, long, (unsigned, void*, unsigned), (count, dest, mode))

static inline void syscall_IOException(long type, long code) ROMCALLV(0xA0, 0xA1, (long, long), (type, code))

static inline long syscall_TestEvent(long event) B0CALL(0x0B, long, (long), (event))

/*
    From OpenBIOS:
    The original implementation of this function has a critical bug. The block
    of code that computes the MSF of the sector to read is inside the retry
    loop. Because the "sector" variable is modified, it changes on each retry,
    so every try but the first will use the wrong sector number.
    This function is also left in ROM, so it can't be patched directly, and
    patching individual instructions wouldn't be easy anyway. Instead, bring in
    OpenBIOS's fixed implementation and patch in references to it.
*/
static int g_cdEventDNE, g_cdEventEND, g_cdEventERR;
int CdReadSector(int count, int sector, char* buffer) {
    int retries;

    sector += 150;

    int minutes = sector / 4500;
    sector %= 4500;
    uint8_t msf[3] = {(minutes % 10) + (minutes / 10) * 0x10, ((sector / 75) % 10) + ((sector / 75) / 10) * 0x10,
                      ((sector % 75) % 10) + ((sector % 75) / 10) * 0x10};

    for (retries = 0; retries < 10; retries++) {
        int cyclesToWait = 99999;
        while (!syscall_CdAsyncSeekL(msf) && (--cyclesToWait > 0));

        if (cyclesToWait < 1) {
            syscall_IOException(0x44, 0x0b);
            return -1;
        }

        while (!syscall_TestEvent(g_cdEventDNE)) {
            if (syscall_TestEvent(g_cdEventERR)) {
                syscall_IOException(0x44, 0x0c);
                return -1;
            }
        }

        cyclesToWait = 99999;
        while (!syscall_CdAsyncReadSector(count, buffer, 0x80) && (--cyclesToWait > 0));
        if (cyclesToWait < 1) {
            syscall_IOException(0x44, 0x0c);
            return -1;
        }
        while (1) {
            // Here, the original code basically does the following:
            //   if (cyclesToWait < 1) return 1;
            // which is 1) useless, since cyclesToWait never mutates
            // and 2) senseless as we're supposed to return the
            // number of sectors read.
            // An optimizing compiler would cull it out anyway, so
            // it's no use letting it here.
            if (syscall_TestEvent(g_cdEventDNE)) {
                return count;
            }
            if (syscall_TestEvent(g_cdEventERR)) {
                break;
            }
            if (syscall_TestEvent(g_cdEventEND)) {
                syscall_IOException(0x44, 0x17);
                return -1;
            }
        }
        syscall_IOException(0x44, 0x16);
    }

    syscall_IOException(0x44, 0x0c);
    return -1;
}

static inline long syscall_CdGetStatus(void) A0CALL(0xA6, long, (void), ())

long dev_cd_read(struct iob *file, void *buffer_, long size) {
    pcsx_checkKernel(0);
    char *buffer = (char *)buffer_;
    if ((size & 0x7ff) || (file->i_offset & 0x7ff) || (size < 0) || (file->i_offset >= file->i_size)) {
        file->i_errno = EINVAL;
        pcsx_checkKernel(1);
        return -1;
    }

    size >>= 11;
    // The original implementation also re-reads the ToC, but that's a private
    // function, and checks if the disc has changed, but that's a private global
    // variable. We'll just have to hope that the ToC has been read and that the
    // disc hasn't changed.
    if ((syscall_CdGetStatus() & 0x10) || (CdReadSector(size, file->i_head + (file->i_offset >> 11), buffer) != size)) {
        file->i_errno = EBUSY;
        pcsx_checkKernel(1);
        return -1;
    }

    size <<= 11;
    unsigned offset = file->i_offset;
    if (file->i_size < (offset + size)) size = file->i_size - offset;
    file->i_offset = offset + size;

    pcsx_checkKernel(1);
    return size;
}

static void** a0table = (void**)0x00000200;

static void patch_CdReadSector(void) {
    pcsx_checkKernel(0);

    void* oldReadSector = a0table[0xA5];
    if (oldReadSector == (void*)CdReadSector) {
        return;
    }
    a0table[0xA5] = (void*)CdReadSector;

    void* oldDevCdRead = a0table[0x60];
    for (int device_major_id = 0; device_major_id < 10; device_major_id++) {
        struct device_table *dcb = &(*dcb_table_ptr)[device_major_id];
        if ((void*)(dcb->dt_read) == oldDevCdRead) {
            dcb->dt_read = dev_cd_read;
        }
    }
    // CdReadSector does reference global variables g_cdEventDNE, g_cdEventEND,
    // and g_cdEventERR, but those are just event IDs. Search for them in the
    // event table.
    for (unsigned i = 0; i < ((*evcb_table_size)/sizeof(struct EvCB)); i++) {
        struct EvCB *ev =  &(*evcb_table_ptr)[i];
        if (ev->desc == 0xF0000003 && ev->mode == 0x2000) {
            unsigned handle = 0xF1000000 | i;
            switch (ev->spec) {
                case 0x20:
                    g_cdEventDNE = handle;
                    break;
                case 0x80:
                    g_cdEventEND = handle;
                    break;
                case 0x8000:
                    g_cdEventERR = handle;
                    break;
            }
        }
    }
    a0table[0x60] = (void*)dev_cd_read;
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

static inline int syscall_SetMem(int megabytes) A0CALL(0x9F, int, (int), (megabytes))

typedef struct {
    void (*ra)(void);
    int* sp;
    int* fp;
    int regfile[8];
    int* gp;
} jmp_buf;

static inline jmp_buf* syscall_ResetEntryInt() B0CALL(0x18, jmp_buf*, (void), ())

static inline void syscall_HookEntryInt(jmp_buf* f) ROMCALLV(0xB0, 0x19, (jmp_buf*), (f))

register unsigned int cp0status asm ("c0r12");

void hardware_init_hook(void) {
    // Crash on out-of-bounds memory access instead of wrapping around and
    // corrupting memory.
    // TODO: get amount of memory by checking for wraparound
    // A(B4h) GetSystemInfo just returns ram size global variable
    // though, ld, elf2psexe, & BIOS expect the initial sp to be set at compile time
    syscall_SetMem(2);

    // maybe needed for events to work?
    syscall_ResetEntryInt();

    patch_CdReadSector();

    // TODO: enable and test memcard, perhaps add patches

    // Enable the GTE.
    // The BIOS already enables it, but Newlib crt0.S disables it again, since
    // it uses .MIPS.abiflags to enable processor features.
    // Enabling the GTE needs to happen after the .MIPS.abiflags stuff but
    // before calling any global constructors. hardware_init_hook fits the bill.
    cp0status |= 0x40000000;

    // Finally, the BIOS calls enterCriticalSection before executing the EXE,
    // but file functions rely on events triggered by interrupts, so we need
    // them back on.
    exitCriticalSection();
}

void software_init_hook(void) {
    pcsx_checkKernel(0);

    // Minor optimization: Enable I-cache on ROM calls
    for (int i = 0; i < 192; i++) {
        ((unsigned*)a0table)[i] &= 0x9FFFFFFFu;
    }

#ifdef PCSX
    int t2 = *(volatile int*)0xBF80101C;
    while (t2<0x80000)
        t2 += 0x10000;
    *(volatile int*)0xBF80101C = t2;
#ifdef MALLOC_PROVIDED
    pcsx_initMsan();
    pcsx_resetMsan();
#endif
    pcsx_checkKernel(1);
#endif
}


/*** Not required, but nice to have ***/

static inline int syscall_ioctl(int fd, unsigned long request, void* arg) B0CALL(0x37, int, (int, unsigned long, void*), (fd, request, arg))
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
static inline void syscall__exit(int code) ROMCALLV(0xA0, 0x3A, (int), (code))
void _exit(int code) {
    syscall__exit(code);
    __builtin_unreachable();
}
*/

static inline void syscall_puts(char *ptr) ROMCALLV(0xB0, 0x3F, (char *), (ptr))
void print(char *ptr) {
    syscall_puts(ptr);
}

static inline int syscall_cd(const char* dirname) B0CALL(0x40, int, (const char*), (dirname))
int chdir(const char *dirname) {
    int ret = syscall_cd(dirname)-1;
    if (ret < 0) {
        errno = syscall__get_errno();
    }
    return ret;
}

static inline int syscall_rename(const char* oldpath, const char* newpath) B0CALL(0x44, int, (const char *, const char *), (oldpath, newpath))
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

