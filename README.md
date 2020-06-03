This is a syscall implementation and (only slightly modified) linker script for use with **[Newlib](http://www.sourceware.org/newlib/)** on the **[Sony PlayStation](en.wikipedia.org/wiki/PlayStation_(console))**. Newlib is a portable libc/stdlib implementation. It isn't as bloated as glibc, doesn't require Linux like musl, and is complete and standards-compliant unlike a custom libc.

### To configure and compile Newlib for the PSX

1. Copy `psx.c` and `psx.ld` to `libgloss/mips/`.
2. Add them to `libgloss/mips/Makefile.in`, for example with:

        PSXOBJS = psx.o syscalls.o getpid.o kill.o putnum.o
        libpsxsys.a: $(PSXOBJS)
	        ${AR} ${ARFLAGS} $@ $(PSXOBJS)
	        ${RANLIB} $@

3. Configure newlib with these flags:
    * `--target=mipsel-elf`: Required for the platform. The `mipsel-unknown-elf` triple is sometimes recommended in other tutorials, but it does the same thing.
    * `CFLAGS` (or `CFLAGS_FOR_TARGET` for a unified build) set to:
        * `-DHAVE_BLKSIZE`: Required. PSX file I/O can only be done in block units, so this tells Newlib to enable "optimizations" that transform FS calls to block alignment.
        * `-g -O2`: From the default configuration.
        * `-DHAVE_RENAME`: Not required, but you'll need to remove `_rename` from `psx.c` if you don't add the flag.
    * `--prefix=/usr/local/psxsdk` or whatever. Optional but recommended to keep your host system root clean.
    * `--disable-multilib`: Optional, but recommended to keep the prefix clean. Disables building extra, unnecessary big-endian and hard-float versions of target libraries.
    * `--enable-lto`: Optional but recommended. Enables link-time-optimization, building intermediate code so that optimizations can be performed all at once during linking.
    * `--disable-libssp`: Optional. Disables support for stack protection.
    * `--disable-libstdcxx`: Optional. Disables support for GNU libstdc++-v3.
    * `--disable-newlib-hw-fp`: Optional. Enforces disabling hard-float support.
    * `--disable-newlib-io-float`: Optional. Disables floating point in format strings.
    * `--disable-newlib-multithread`: Optional. Disables threading support.
    * `--disable-shared`: Optional. Do not build shared libraries.
    * `--enable-lite-exit`: Optional. Console games don't typically exit, so why spend time there?

For example, here's my full configure line, using a ["unified"](https://www.embecosm.com/appnotes/ean9/html/ch02s01.html) build alongside binutils and GCC:

    ../configure --prefix=/usr/local/psxsdk --target=mipsel-elf \
	    --disable-gcov \
	    --disable-libssp \
	    --disable-libstdcxx \
	    --disable-multilib \
	    --disable-newlib-hw-fp \
	    --disable-newlib-io-float \
	    --disable-newlib-multithread \
	    --disable-nls \
	    --disable-shared \
	    --enable-gold \
	    --enable-languages=c,c++ \
	    --enable-lite-exit \
	    --enable-lto \
	    --enable-serial-configure \
	    --with-float=soft \
	    --with-isl \
	    --with-newlib \
	    --with-no-pic \
	    CFLAGS_FOR_TARGET="-g -O2 -DHAVE_BLKSIZE -DHAVE_RENAME"

Then `make` and `make install` as usual. Add the new prefix to your `PATH` in necessary, then you can use `mipsel-elf-gcc` with `-flto -msoft-float -mips1` to build things, and `mipsel-elf-gcc-ld` with `-nodefaultlibs -Tpsx.ld` to link things.

### Thanks

* [Adrian Siekierka](https://github.com/asiekierka) for [candyk](https://github.com/ChenThread/candyk-packages), which helped me figure out some of the configure flags
* [Nicolas Noble](https://github.com/nicolasnoble) for [Openbios](https://github.com/grumpycoders/pcsx-redux/tree/master/src/mips/openbios), which helped me figure out how some of the BIOS calls work, and also how to do them inline in C
* [Nocash](https://problemkaputt.de/) for the [PSX Specifications](https://problemkaputt.de/psx-spx.htm)
* NextVolume/Tails92 and [Lameguy64](https://github.com/Lameguy64) for their existing PS1 SDKs

### To do

* Make a custom `crt0.s`. The built-in one is great, but 1) supplies `_exit`, which could be done as a syscall, and 2) zeroes `.bss` and does some other system initialization that the BIOS boot sequence already does.
* Consider using the syscall versions of some or all stdlib functions.
* Consider `-O2` vs `-Os` vs `-O3`.

