#define ROMCALL(vector, number, returntype, param_sig, parameters) { \
    register volatile int n asm("t1") = number; \
    __asm__ volatile("" : "=r"(n) : "r"(n)); \
    asm("addiu $sp, $sp, -16"); \
    returntype ret = ((returntype(*)param_sig)vector)parameters; \
    asm("addiu $sp, $sp, 16"); \
    return ret; \
}

#define ROMCALLV(vector, number, param_sig, parameters) { \
    register volatile int n asm("t1") = number; \
    __asm__ volatile("" : "=r"(n) : "r"(n)); \
    asm("addiu $sp, $sp, -16"); \
    ((void(*)param_sig)vector)parameters; \
    asm("addiu $sp, $sp, 16"); \
}

#define A0CALL(number, returntype, param_sig, parameters) ROMCALL(0xA0, number, returntype, param_sig, parameters)

#include <stddef.h>
unsigned long strtoul(const char *src, char **src_end, int base) A0CALL(0x0C, unsigned long, (const char *, char **, int), (src, src_end, base))
long int strtol(const char *src, char **src_end, int base) A0CALL(0x0D, long int, (const char *, char **, int), (src, src_end, base))
int abs(int val) A0CALL(0x0E, int, (int), (val))
long int labs(long int val) A0CALL(0x0F, long int, (long int), (val))
int atoi(const char *src) A0CALL(0x10, int, (const char *), (src))
long atol(const char *src) A0CALL(0x11, long, (const char *), (src))
#include <setjmp.h>
#if _JBLEN != 12
#error jmp_buf does not have the right size to use the BIOS implementation
#endif
int setjmp(jmp_buf buf) A0CALL(0x13, int, (jmp_buf), (buf))
void longjmp(jmp_buf buf, int param) {
    ROMCALLV(0xA0, 0x14, (jmp_buf, int), (buf, param))
    __builtin_unreachable();
}
char *strcat(char *dst, const char *src) A0CALL(0x15, char *, (char *, const char *), (dst, src))
char *strncat(char *dst, const char *src, size_t maxlen) A0CALL(0x16, char *, (char *, const char *, size_t), (dst, src, maxlen))
int strcmp(const char *str1, const char *str2) A0CALL(0x17, int, (const char *, const char *), (str1, str2))
int strncmp(const char *str1, const char *str2, size_t maxlen) A0CALL(0x18, int, (const char *, const char *, size_t), (str1, str2, maxlen))
char *strcpy(char *dst, const char *src) A0CALL(0x19, char *, (char *, const char *), (dst, src))
char *strncpy(char *dst, const char *src, size_t maxlen) A0CALL(0x1A, char *, (char *, const char *, size_t), (dst, src, maxlen))
size_t strlen(const char *src) A0CALL(0x1B, size_t, (const char *), (src))
char *index(const char *src, int chr) A0CALL(0x1C, char *, (const char *, int), (src, chr))
char *rindex(const char *src, int chr) A0CALL(0x1D, char *, (const char *, int), (src, chr))
char *strchr(const char *src, int chr) A0CALL(0x1E, char *, (const char *, int), (src, chr))
char *strrchr(const char *src, int chr) A0CALL(0x1F, char *, (const char *, int), (src, chr))
/* buggy
char *strpbrk(const char *s, const char *accept) A0CALL(0x20, char *, (const char *, const char *), (s, accept))
*/
size_t strspn(const char *src, const char *list) A0CALL(0x21, size_t, (const char *, const char *), (src, list))
size_t strcspn(const char *src, const char *list) A0CALL(0x22, size_t, (const char *, const char *), (src, list))
char *strtok(char *src, const char *list) A0CALL(0x23, char *, (char *, const char *), (src, list))
/* buggy
char *strstr(const char *haystack, const char *needle) A0CALL(0x24, char *, (const char *, const char *), (haystack, needle))
*/
int toupper(int chr) A0CALL(0x25, int, (int), (chr))
int tolower(int chr) A0CALL(0x26, int, (int), (chr))
/* slow, run from ROM
void bcopy(const void *src, void *dest, size_t size) ROMCALLV(0xA0, 0x27, (const void *, void *, size_t), (src, dest, size))
void bzero(void *s, size_t size) ROMCALLV(0xA0, 0x28, (void *, size_t), (s, size))
void *memcpy(void *dest, const void *src, size_t size) A0CALL(0x2A, void *, (void *, const void *, size_t), (dest, src, size))
void *memset(void *s, int c, size_t size) A0CALL(0x2B, void *, (void *, int, size_t), (s, c, size))
void *memchr(const void *s, int c, size_t size) A0CALL(0x2E, void *, (const void*, int, size_t), (s, c, size))
*/
int rand(void) A0CALL(0x2F, int, (void), ())
void srand(unsigned int seed) ROMCALLV(0xA0, 0x30, (unsigned int), (seed))
#if defined(MALLOC_PROVIDED) && !defined(PCSX)
void qsort(void *base, size_t nel, size_t width, int (*callback)(const void *, const void *)) ROMCALLV(0xA0, 0x31, (void *, size_t, size_t, int (*)(const void *, const void *)), (base, nel, width, callback))
void *malloc(size_t size) A0CALL(0x33, void *, (size_t), (size))
void free(void *ptr) ROMCALLV(0xA0, 0x34, (void *), (ptr))
#endif
void *bsearch(const void *key, const void *base, size_t nel, size_t width, int (*callback)(const void *, const void *)) A0CALL(0x36, void *,(const void *, const void *, size_t, size_t, int (*)(const void *, const void *)), (key, base, nel, width, callback))
#if defined(MALLOC_PROVIDED) && !defined(PCSX)
void *calloc(size_t nmemb, size_t size) A0CALL(0x37, void *, (size_t, size_t), (nmemb, size))
void *realloc(void *ptr, size_t size) A0CALL(0x38, void *, (void *, size_t), (ptr, size))
void InitHeap(unsigned long *head, unsigned long size) ROMCALLV(0xA0, 0x39, (unsigned long *, unsigned long), (head, size))

extern char _end[];
struct s_mem {
  unsigned int size;
  unsigned int icsize;
  unsigned int dcsize;
} mem;
extern void get_mem_info(struct s_mem *);
void software_init_hook(void) {
    get_mem_info(&mem);
    InitHeap(_end, mem.size);
}
#endif
int getchar(void) A0CALL(0x3B, int, (void), ())
int putchar(int c) A0CALL(0x3C, int, (int), (c))
char *gets(char *s) A0CALL(0x3D, char *, (char *), (s))
int puts(const char *s) A0CALL(0x3E, int, (const char *), (s))

