# initial information
AC_INIT([metachain], [1], [office@trustchaintechnologies.io])

# check for the source directory
AC_CONFIG_SRCDIR([src/defines.h])

# automake initialization version ])= 1.9
AM_INIT_AUTOMAKE([1.9])

# set macro dir
AC_CONFIG_MACRO_DIR([m4])

# check for C++ preprocessor and compiler
AC_PROG_CXXCPP
AC_PROG_CXX

# use C++ compiler checks
AC_LANG([C++])

# header checks
AC_HEADER_STDC
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([string])
AC_CHECK_HEADERS([iostream])
AC_CHECK_HEADERS([time.h])
AC_CHECK_HEADERS([fstream])
AC_CHECK_HEADERS([vector])
AC_CHECK_HEADERS([map])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([limits])
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADERS([atomic])
AC_CHECK_HEADERS([chrono])
AC_CHECK_HEADERS([mutex])
AC_CHECK_HEADERS([thread])
AC_CHECK_HEADERS([queue])

# checks for typedefs, structures and compiler charactaristics
AC_TYPE_SIZE_T

# final files
AC_CONFIG_FILES([Makefile])

# check and use boost library
AX_BOOST_BASE([1.48], [], [AC_MSG_ERROR([TCT needs Boost, but it was not found in your system])])
AX_BOOST_CHRONO
AX_BOOST_DATE_TIME
AX_BOOST_IOSTREAMS
AX_BOOST_LOCALE
AX_BOOST_REGEX
AX_BOOST_SIGNALS
AX_BOOST_THREAD
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM

# preprocessor defines
AC_DEFINE(_LINUX)
AC_DEFINE(HAVE_DAEMON)
AC_DEFINE(_DEBUG)
AC_DEFINE(HAVE_WORKING_BOOST_SLEEP_FOR)

# end
AC_OUTPUT

