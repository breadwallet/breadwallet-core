# Copyright 2014-2016 Luke Dashjr
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the standard MIT license.  See COPYING for more details.

lib_LTLIBRARIES = libbreadwallet-core.la
libbreadwallet_core_la_SOURCES = \
	BRAddress.c  \
	BRBIP32Sequence.c  \
	BRBIP38Key.c  \
	BRBIP39Mnemonic.c  \
	BRBase58.c  \
	BRBloomFilter.c  \
	BRCrypto.c  \
	BRKey.c  \
	BRMerkleBlock.c  \
	BRPaymentProtocol.c  \
	BRPeer.c  \
	BRPeerManager.c  \
	BRSet.c  \
	BRTransaction.c  \
	BRWallet.c

libbreadwallet_core_la_CFLAGS = $(libsecp256k1_CFLAGS) $(PTHREAD_CFLAGS)
libbreadwallet_core_la_LDFLAGS = -version-info $(LIBBREADWALLET_CORE_SO_VERSION) -no-undefined
libbreadwallet_core_la_LIBADD = $(libsecp256k1_LIBS) $(PTHREAD_LIBS)

libbreadwallet_core_includedir = $(includedir)/breadwallet-core
libbreadwallet_core_include_HEADERS = \
	BRAddress.h  \
	BRArray.h  \
	BRBIP32Sequence.h  \
	BRBIP38Key.h  \
	BRBIP39Mnemonic.h  \
	BRBIP39WordsEn.h  \
	BRBase58.h  \
	BRBloomFilter.h  \
	BRCrypto.h  \
	BRInt.h  \
	BRKey.h  \
	BRList.h  \
	BRMerkleBlock.h  \
	BRPaymentProtocol.h  \
	BRPeer.h  \
	BRPeerManager.h  \
	BRSet.h  \
	BRTransaction.h  \
	BRWallet.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libbreadwallet-core.pc

dist_noinst_SCRIPTS = autogen.sh
dist_doc_DATA = LICENSE README.md
