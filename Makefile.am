AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = bootstrap

AM_LDFLAGS = $(PTHREAD_CFLAGS) $(LIBTOOL_LDFLAGS) $(LDFLAGS)
AM_CXXFLAGS = $(CXXFLAGS)
AM_CPPFLAGS = $(CPPFLAGS)

NODE_INCLUDES=-I$(builddir) -I$(builddir)/obj $(BDB_CPPFLAGS) $(BOOST_CPPFLAGS) $(CRYPTO_CFLAGS) $(SSL_CFLAGS)

tctnoded_CPPFLAGS = $(AM_CPPFLAGS) $(BITCOIN_INCLUDES) $(MINIUPNPC_CPPFLAGS) $(EVENT_CFLAGS)
tctnoded_CXXFLAGS = $(AM_CXXFLAGS) $(PIE_FLAGS)
tctnoded_LDADD = $(BOOST_LIBS) $(BDB_LIBS) $(SSL_LIBS) $(CRYPTO_LIBS)
tctnoded_SOURCES = \
	src/crypto/ctaes/ctaes.c \
	src/crypto/aes.cpp \
	src/crypto/chacha20.cpp \
	src/crypto/hmac_sha256.cpp \
	src/crypto/hmac_sha512.cpp \
	src/crypto/ripemd160.cpp \
	src/crypto/sha1.cpp \
	src/crypto/sha256.cpp \
	src/crypto/sha512.cpp \
	src/ArgsManager.cpp \
	src/ConvertUTF.c \
	src/cThreadInterrupt.cpp \
	src/hash.cpp \
	src/random.cpp \
	src/scheduler.cpp \
	src/uint256.cpp \
	src/utilstrencodings.cpp \
	src/io/netPeers.cpp \
	src/network/CNetAddr.cpp \
	src/network/CService.cpp \
	src/network/CSubnet.cpp \
	src/network/netMessage.cpp \
	src/network/NetworkManager.cpp \
	src/functions.cpp \
	src/logger.cpp \
	src/MetaChain.cpp \
	node.cpp

bin_PROGRAMS = tctnoded
