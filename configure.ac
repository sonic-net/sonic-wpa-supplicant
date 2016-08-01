#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([sonic-nas-qos], [1.1.0], [support@dell.com])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([.])
AC_CONFIG_HEADERS([config.h])
ACLOCAL_AMFLAGS=-I m4
AC_CONFIG_MACRO_DIRS([m4])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
LT_INIT([shared])

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
