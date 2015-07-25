#ifndef CLANG_CONFIG_H
#define CLANG_CONFIG_H

/* Bug report URL. */
#define BUG_REPORT_URL "http://llvm.org/bugs/"

/* Multilib suffix for libdir. */
#define CLANG_LIBDIR_SUFFIX ""

/* Relative directory for resource files */
#define CLANG_RESOURCE_DIR ""

/* Directories clang will search for headers */
#define C_INCLUDE_DIRS ""

/* Default <path> to all compiler invocations for --sysroot=<path>. */
/* #undef DEFAULT_SYSROOT */

/* Directory where gcc is installed. */
#define GCC_INSTALL_PREFIX ""

/* Define if we have libxml2 */
/* #undef CLANG_HAVE_LIBXML */

/* The LLVM product name and version */
#define BACKEND_PACKAGE_STRING "LLVM 3.6.2"

/* Linker version detected at compile time. */
/* #undef HOST_LINK_VERSION */

#endif /* CLANG_CONFIG_H */
