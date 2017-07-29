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

# check for c++11
AX_CXX_COMPILE_STDCXX([11], [noext], [mandatory], [nodefault])

# check the host
AC_CANONICAL_HOST

# define the OS
use_pkgconfig=yes
case $host in
  *mingw*)
	AC_MSG_ERROR([Not supporting build for mingw yet])
     ;;
  *darwin*)
    AC_MSG_ERROR([Not supporting build for darwin yet])
     ;;
   *linux*)
     	TARGET_OS=linux
	 CPPFLAGS="$CPPFLAGS -D_LINUX -DHAVE_DAEMON"
     ;;
   *freebsd*)
    AC_MSG_ERROR([Not supporting build for freebsd yet]) 
     ;;
   *openbsd*)
    AC_MSG_ERROR([Not supporting build for openbsd yet]) 
     ;;
   *)
     OTHER_OS=`echo ${host_os} | awk '{print toupper($0)}'`
     AC_MSG_WARN([Guessing LevelDB OS as OS_${OTHER_OS}, please check whether this is correct, if not add an entry to configure.ac.])
	TARGET_OS = "$OTHER_OS"
	CPPFLAGS="$CPPFLAGS -D_${OTHER_OS}"
     ;;
esac

if test x$use_pkgconfig = xyes; then
  m4_ifndef([PKG_PROG_PKG_CONFIG], [AC_MSG_ERROR(PKG_PROG_PKG_CONFIG macro not found. Please install pkg-config and re-run autogen.sh.)])
  m4_ifdef([PKG_PROG_PKG_CONFIG], [
  PKG_PROG_PKG_CONFIG
  if test x"$PKG_CONFIG" = "x"; then
    AC_MSG_ERROR(pkg-config not found.)
  fi
  ])
fi

# Enable debug
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
                    [use debug compiler flags and macros (default is no)])],
    [enable_debug=$enableval],
    [enable_debug=no])
if test "x$enable_debug" = xyes; then
    CPPFLAGS="$CPPFLAGS -D_DEBUG"
    if test "x$GCC" = xyes; then
        CFLAGS="$CFLAGS -g3 -O0"
    fi

    if test "x$GXX" = xyes; then
        CXXFLAGS="$CXXFLAGS -g3 -O0"
    fi
fi

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
AC_CHECK_HEADERS([endian.h sys/endian.h byteswap.h stdio.h stdlib.h unistd.h strings.h sys/types.h sys/stat.h sys/select.h sys/prctl.h])
AC_CHECK_HEADER([openssl/crypto.h],,AC_MSG_ERROR(libcrypto headers missing))
AC_CHECK_LIB([crypto],      [main],CRYPTO_LIBS=-lcrypto, AC_MSG_ERROR(libcrypto missing))
AC_CHECK_HEADER([openssl/ssl.h],, AC_MSG_ERROR(libssl headers missing),)
AC_CHECK_LIB([ssl],         [main],SSL_LIBS=-lssl, AC_MSG_ERROR(libssl missing))

# checks for typedefs, structures and compiler charactaristics
AC_TYPE_SIZE_T

# check for endianness
AC_C_BIGENDIAN

# Check for pthread compile/link requirements
AX_PTHREAD

# check for existing headers
AC_CHECK_DECLS([le16toh, le32toh, le64toh, htole16, htole32, htole64, be16toh, be32toh, be64toh, htobe16, htobe32, htobe64],,,
		[#if HAVE_ENDIAN_H
                 #include <endian.h>
                 #elif HAVE_SYS_ENDIAN_H
                 #include <sys/endian.h>
                 #endif])

AC_CHECK_DECLS([bswap_16, bswap_32, bswap_64],,,
		[#if HAVE_BYTESWAP_H
                 #include <byteswap.h>
                 #endif])

# check for daemon(3)
AC_CHECK_DECLS([daemon])

# Check for MSG_NOSIGNAL
AC_MSG_CHECKING(for MSG_NOSIGNAL)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/socket.h>]],
 [[ int f = MSG_NOSIGNAL; ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_MSG_NOSIGNAL, 1,[Define this symbol if you have MSG_NOSIGNAL]) ],
 [ AC_MSG_RESULT(no)]
)

# Check for MSG_DONTWAIT
AC_MSG_CHECKING(for MSG_DONTWAIT)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <sys/socket.h>]],
 [[ int f = MSG_DONTWAIT; ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_MSG_DONTWAIT, 1,[Define this symbol if you have MSG_DONTWAIT]) ],
 [ AC_MSG_RESULT(no)]
)

# Check for malloc_info (for memory statistics information in getmemoryinfo)
AC_MSG_CHECKING(for getmemoryinfo)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <malloc.h>]],
 [[ int f = malloc_info(0, NULL); ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_MALLOC_INFO, 1,[Define this symbol if you have malloc_info]) ],
 [ AC_MSG_RESULT(no)]
)

# check for randomness
AC_MSG_CHECKING(for Linux getrandom syscall)
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[#include <unistd.h>
  #include <sys/syscall.h>
  #include <linux/random.h>]],
 [[ syscall(SYS_getrandom, nullptr, 32, 0); ]])],
 [ AC_MSG_RESULT(yes); AC_DEFINE(HAVE_SYS_GETRANDOM, 1,[Define this symbol if the Linux getrandom system call is available]) ],
 [ AC_MSG_RESULT(no)]
)

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

BOOST_LIBS="$BOOST_LDFLAGS $BOOST_SYSTEM_LIB $BOOST_FILESYSTEM_LIB $BOOST_PROGRAM_OPTIONS_LIB $BOOST_THREAD_LIB $BOOST_CHRONO_LIB"

# Boost >= 1.50 uses sleep_for rather than the now-deprecated sleep, however
# it was broken from 1.50 to 1.52 when backed by nanosleep. Use sleep_for if
# a working version is available, else fall back to sleep. sleep was removed
# after 1.56.
# If neither is available, abort.
m4_pattern_allow(DBOOST_AC_USE_STD_ATOMIC) # otherwise it's treated like a macro
BOOST_CPPFLAGS="-DBOOST_SP_USE_STD_ATOMIC -DBOOST_AC_USE_STD_ATOMIC $BOOST_CPPFLAGS"

TEMP_LIBS="$LIBS"
LIBS="$BOOST_LIBS $LIBS"
TEMP_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
  #include <boost/thread/thread.hpp>
  #include <boost/version.hpp>
  ]],[[
  #if BOOST_VERSION >= 105000 && (!defined(BOOST_HAS_NANOSLEEP) || BOOST_VERSION >= 105200)
      boost::this_thread::sleep_for(boost::chrono::milliseconds(0));
  #else
   choke me
  #endif
  ]])],
  [boost_sleep=yes;
     AC_DEFINE(HAVE_WORKING_BOOST_SLEEP_FOR, 1, [Define this symbol if boost sleep_for works])],
  [boost_sleep=no])
LIBS="$TEMP_LIBS"
CPPFLAGS="$TEMP_CPPFLAGS"

if test x$boost_sleep != xyes; then
TEMP_LIBS="$LIBS"
LIBS="$BOOST_LIBS $LIBS"
TEMP_CPPFLAGS="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
AC_LINK_IFELSE([AC_LANG_PROGRAM([[
  #include <boost/version.hpp>
  #include <boost/thread.hpp>
  #include <boost/date_time/posix_time/posix_time_types.hpp>
  ]],[[
  #if BOOST_VERSION <= 105600
      boost::this_thread::sleep(boost::posix_time::milliseconds(0));
  #else
   choke me
  #endif
  ]])],
  [boost_sleep=yes; AC_DEFINE(HAVE_WORKING_BOOST_SLEEP, 1, [Define this symbol if boost sleep works])],
  [boost_sleep=no])
LIBS="$TEMP_LIBS"
CPPFLAGS="$TEMP_CPPFLAGS"
fi

if test x$boost_sleep != xyes; then
  AC_MSG_ERROR(No working boost sleep implementation found.)
fi

save_CXXFLAGS="${CXXFLAGS}"
CXXFLAGS="${CXXFLAGS} ${CRYPTO_CFLAGS} ${SSL_CFLAGS}"
AC_CHECK_DECLS([EVP_MD_CTX_new],,,[AC_INCLUDES_DEFAULT
#include <openssl/x509_vfy.h>
])
CXXFLAGS="${save_CXXFLAGS}"

AC_SUBST(LIBTOOL_APP_LDFLAGS)
AC_SUBST(BOOST_LIBS)
AC_SUBST(CRYPTO_LIBS)
AC_SUBST(SSL_LIBS)
AC_SUBST(EVENT_LIBS)

# end
AC_OUTPUT

echo 
echo "Options used to compile and link:"
echo "  debug enabled (--enable-debug) = $enable_debug"
echo
echo "  target OS     = $TARGET_OS"
echo
echo "  CC            = $CC"
echo "  CFLAGS        = $CFLAGS"
echo "  CPPFLAGS      = $CPPFLAGS"
echo "  CXX           = $CXX"
echo "  CXXFLAGS      = $CXXFLAGS"
echo "  LDFLAGS       = $LDFLAGS"
echo 
echo "Experiencing problems in config or build? Please report them in the forums <https://forum.trustchaintechnologies.io> or to our support team <https://support.trustchaintechnologies.io>"
echo "As with all bug reports or build problems, please include your OS type and version, g++ version, output of ./configure and all other logs that may be usefull"
echo
