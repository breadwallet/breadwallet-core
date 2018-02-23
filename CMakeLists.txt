cmake_minimum_required(VERSION 3.9.1)

add_library(breadwallet-core-lib SHARED
	BRAddress.h
	BRArray.h
	BRBIP32Sequence.h
	BRBIP38Key.h
	BRBIP39Mnemonic.h
	BRBIP39WordsEn.h
	BRBase58.h
	BRBech32.h
	BRBloomFilter.h
	BRChainParams.h
	BRCrypto.h
	BRInt.h
	BRKey.h
	BRMerkleBlock.h
	BRPaymentProtocol.h
	BRPeer.h
	BRPeerManager.h
	BRSet.h
	BRTransaction.h
	BRWallet.h
	bcash/BRBCashAddr.h
	bcash/BRBCashParams.h

	BRAddress.c
	BRBIP32Sequence.c
	BRBIP38Key.c
	BRBIP39Mnemonic.c
	BRBase58.c
	BRBech32.c
	BRBloomFilter.c
	BRCrypto.c
	BRKey.c
	BRMerkleBlock.c
	BRPaymentProtocol.c
	BRPeer.c
	BRPeerManager.c
	BRSet.c
	BRTransaction.c
	BRWallet.c
	bcash/BRBCashAddr.c
	)

add_executable(breadwallet-test test.c)
target_link_libraries(breadwallet-test breadwallet-core-lib)

include_directories(.)
include_directories(secp256k1/include/)
include_directories(secp256k1/src/)
include_directories(secp256k1/)