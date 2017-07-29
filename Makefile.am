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
	src/external/crypto/ctaes/ctaes.c \
	src/external/crypto/aes.cpp \
	src/external/crypto/chacha20.cpp \
	src/external/crypto/hmac_sha256.cpp \
	src/external/crypto/hmac_sha512.cpp \
	src/external/crypto/ripemd160.cpp \
	src/external/crypto/sha1.cpp \
	src/external/crypto/sha256.cpp \
	src/external/crypto/sha512.cpp \
	src/external/ArgsManager.cpp \
	src/external/ConvertUTF.c \
	src/external/cThreadInterrupt.cpp \
	src/external/hash.cpp \
	src/external/random.cpp \
	src/external/scheduler.cpp \
	src/external/uint256.cpp \
	src/external/utilstrencodings.cpp \
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
