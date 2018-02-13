#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
#AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_INIT(binary-sync, 0.1, info@rangee.com)
AM_INIT_AUTOMAKE([1.9 foreign subdir-objects])

AC_CONFIG_SRCDIR([src/bs_data.c])
AC_CONFIG_HEADERS([config.h])

AC_LIBSOURCES([binary-sync])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_SEARCH_LIBS([truncate], [], [], [
  AC_MSG_ERROR([unable to find the truncate() function])
])
AC_SEARCH_LIBS([adler32], [z], [], [
  AC_MSG_ERROR([unable to find the adler32() function])
])

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h stddef.h stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
