# PIC support for Forge's GCC containers

The prebuilt devkitARM libstdc++.a and libm.a contain absolute addresses in
read-only sections. MHGU's nnrtld relocates these sections without making them
writable, so importing those archives crashes before forge_init.

Forge uses the game's C allocation functions and exported termination/math
functions. C++ allocation is bound locally: MHGU's exported operator new uses an
engine heap that is not initialized when Forge's DT_INIT executes. Importing
that symbol caused v4 to dereference a null engine heap before game startup.
The following upstream sources use Forge's -fPIC and -fno-exceptions flags:

- src/c++98/tree.cc: red-black tree traversal and insertion.
- src/c++11/hashtable_c++0x.cc and src/shared/hashtable-aux.cc: prime rehash policy.
- src/c++11/functexcept.cc: GCC's no-exceptions failure helpers (abort on failure).
- libsupc++/new_*.cc and del_*.cc: matching local new/delete implementations,
  including arrays, sized deletes and aligned allocation. The version script
  keeps these implementations local, so the loader cannot redirect them to the
  game's engine allocator. They continue to use the C malloc/free family.

The upstream files are unchanged GCC 16.1.0 sources. origins.json records their
URLs and SHA-256 hashes. COPYING3 and COPYING.RUNTIME accompany these sources.
The .cpp files are wrappers so the existing Forge Makefile compiles them without
renaming upstream files. This subset does not implement a general C++ runtime.

Build with devkitARM GCC 16.1.0 to match the installed container headers. Every
build links with -z text and checks both ELF and NSO relocation destinations.
