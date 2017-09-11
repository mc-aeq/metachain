AUTOMAKE_OPTIONS = foreign subdir-objects
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = bootstrap

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(LDFLAGS)
AM_CXXFLAGS = $(CXXFLAGS)
AM_CPPFLAGS = $(CPPFLAGS)

NODE_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS) -Idependencies/zlib/contrib/

tctnoded_CPPFLAGS = $(AM_CPPFLAGS) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS) $(NODE_INCLUDES)
tctnoded_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
tctnoded_LDADD = $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS) $(CURL_LIBS) $(ZLIB_LIBS) $(RDB_LIBS)
tctnoded_LDFLAGS = $(AM_LDFLAGS)
tctnoded_SOURCES = \
	src/crypto/ctaes/ctaes.c \
	src/crypto/aes.cpp \
	src/crypto/chacha20.cpp \
	src/crypto/hmac_sha256.cpp \
	src/crypto/hmac_sha512.cpp \
	src/crypto/ripemd160.cpp \
	src/crypto/sha1.cpp \
	src/crypto/sha3.cpp \
	src/crypto/sha256.cpp \
	src/crypto/sha512.cpp \
	src/io/db/mysql.cpp \
	src/io/db/rdb.cpp \
	src/io/zip/ioapi.c \
	src/io/zip/unzip.cpp \
	src/io/zip/zip.cpp \
	src/io/zip/zlib_unzip.c \
	src/io/zip/zlib_zip.c \
	src/io/netPeers.cpp \
	src/io/StorageManager.cpp \
	src/network/CNetAddr.cpp \
        src/network/CService.cpp \
        src/network/CSubnet.cpp \
        src/network/netMessage.cpp \
        src/network/NetworkManager.cpp \
	src/ArgsManager.cpp \
	src/ConvertUTF.c \
	src/cThreadInterrupt.cpp \
	src/functions.cpp \
	src/hash.cpp \
        src/logger.cpp \
        src/MetaChain.cpp \
	src/random.cpp \
	src/scheduler.cpp \
	src/uint256.cpp \
	src/utilstrencodings.cpp \
	node.cpp

bin_PROGRAMS = tctnoded
