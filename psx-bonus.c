#include <stddef.h>
unsigned long strtoul(const char *src, char **src_end, int base) {
    register volatile int n asm("t1") = 0x0C;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((unsigned long(*)(const char *, char **, int))0xA0)(src, src_end, base);
}
long int strtol(const char *src, char **src_end, int base) {
    register volatile int n asm("t1") = 0x0D;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((long int(*)(const char *, char **, int))0xA0)(src, src_end, base);
}
int abs(int val) {
    register volatile int n asm("t1") = 0x0E;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xA0)(val);
}
long int labs(long int val) {
    register volatile int n asm("t1") = 0x0F;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((long int(*)(long int))0xA0)(val);
}
int atoi(const char *src) {
    register volatile int n asm("t1") = 0x10;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *))0xA0)(src);
}
long atol(const char *src) {
    register volatile int n asm("t1") = 0x11;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((long(*)(const char *))0xA0)(src);
}
#include <setjmp.h>
#if _JBLEN != 12
#error jmp_buf does not have the right size to use the BIOS implementation
#endif
int setjmp(jmp_buf buf) {
    register volatile int n asm("t1") = 0x13;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(jmp_buf))0xA0)(buf);
}
void longjmp(jmp_buf buf, int param) {
    register volatile int n asm("t1") = 0x14;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    ((void(*)(jmp_buf, int))0xA0)(buf, param);
}
char *strcat(char *dst, const char *src) {
    register volatile int n asm("t1") = 0x15;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(char *, const char *))0xA0)(dst, src);
}
char *strncat(char *dst, const char *src, size_t maxlen) {
    register volatile int n asm("t1") = 0x16;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(char *, const char *, size_t))0xA0)(dst, src, maxlen);
}
int strcmp(const char *str1, const char *str2) {
    register volatile int n asm("t1") = 0x17;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *, const char *))0xA0)(str1, str2);
}
int strncmp(const char *str1, const char *str2, size_t maxlen) {
    register volatile int n asm("t1") = 0x18;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *, const char *, size_t))0xA0)(str1, str2, maxlen);
}
char *strcpy(char *dst, const char *src) {
    register volatile int n asm("t1") = 0x19;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(char *, const char *))0xA0)(dst, src);
}
char *strncpy(char *dst, const char *src, size_t maxlen) {
    register volatile int n asm("t1") = 0x1A;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(char *, const char *, size_t))0xA0)(dst, src, maxlen);
}
size_t strlen(const char *src) {
    register volatile int n asm("t1") = 0x1B;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((size_t(*)(const char *))0xA0)(src);
}
char *index(const char *src, int chr) {
    register volatile int n asm("t1") = 0x1C;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(const char *, int))0xA0)(src, chr);
}
char *rindex(const char *src, int chr) {
    register volatile int n asm("t1") = 0x1D;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(const char *, int))0xA0)(src, chr);
}
char *strchr(const char *src, int chr) {
    register volatile int n asm("t1") = 0x1E;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(const char *, int))0xA0)(src, chr);
}
char *strrchr(const char *src, int chr) {
    register volatile int n asm("t1") = 0x1F;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(const char *, int))0xA0)(src, chr);
}
size_t strspn(const char *src, const char *list) {
    register volatile int n asm("t1") = 0x21;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((size_t(*)(const char *, const char *))0xA0)(src, list);
}
size_t strcspn(const char *src, const char *list) {
    register volatile int n asm("t1") = 0x22;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((size_t(*)(const char *, const char *))0xA0)(src, list);
}
char *strtok(char *src, const char *list) {
    register volatile int n asm("t1") = 0x23;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(char *, const char *))0xA0)(src, list);
}
int toupper(int chr) {
    register volatile int n asm("t1") = 0x25;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xA0)(chr);
}
int tolower(int chr) {
    register volatile int n asm("t1") = 0x26;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xA0)(chr);
}
int rand(void) {
    register volatile int n asm("t1") = 0x2F;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(void))0xA0)();
}
void srand(unsigned int seed) {
    register volatile int n asm("t1") = 0x30;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((void(*)(unsigned int))0xA0)(seed);
}
#ifdef MALLOC_PROVIDED
void qsort(void *base, size_t nel, size_t width, int (*callback)(const void *, const void *)) {
    register volatile int n asm("t1") = 0x31;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((void(*)(void *, size_t, size_t, int (*)(const void *, const void *)))0xA0)(base, nel, width, callback);
}
#endif
void *bsearch(const void *key, const void *base, size_t nel, size_t width, int (*callback)(const void *, const void *)) {
    register volatile int n asm("t1") = 0x36;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((void *(*)(const void *, const void *, size_t, size_t, int (*)(const void *, const void *)))0xA0)(key, base, nel, width, callback);
}
int getchar(void) {
    register volatile int n asm("t1") = 0x3B;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(void))0xA0)();
}
int putchar(int c) {
    register volatile int n asm("t1") = 0x3C;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(int))0xA0)(c);
}
char *gets(char *s) {
    register volatile int n asm("t1") = 0x3D;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((char *(*)(char *))0xA0)(s);
}
int puts(const char *s) {
    register volatile int n asm("t1") = 0x3E;
    __asm__ volatile("" : "=r"(n) : "r"(n));
    return ((int(*)(const char *))0xA0)(s);
}

