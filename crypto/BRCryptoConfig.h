//
//  BRCryptoConfig.h
//  BRCore
//
//  Created by Ed Gamble on 12/9/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#if !defined DEFINE_NETWORK
#define DEFINE_NETWORK(symbol, networkId, name, network, isMainnet, height, confirmations)
#endif

#if !defined DEFINE_NETWORK_FEE_ESTIMATE
#define DEFINE_NETWORK_FEE_ESTIMATE(networkId, amount, tier, confirmationTimeInMilliseconds)
#endif

#if !defined DEFINE_CURRENCY
#define DEFINE_CURRENCY(networkId, currencyId, name, code, type, address, verified)
#endif

#if !defined DEFINE_UNIT
#define DEFINE_UNIT(currencyId, name, code, decimals, symbol)
#endif

#if !defined DEFINE_ADDRESS_SCHEMES
#define DEFINE_ADDRESS_SCHEMES(networkId, defaultScheme, otherSchemes...)
#endif

#if !defined DEFINE_MODES
#define DEFINE_MODES(networkId, defaultMode, otherModes...)
#endif

// MARK: - BTC

#define NETWORK_NAME    "Bitcoin"
DEFINE_NETWORK (btcMainnet,  "bitcoin-mainnet", NETWORK_NAME, "mainnet", true, 612691, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("bitcoin-mainnet", "18", "10m", 10 * 60 * 1000)
DEFINE_CURRENCY ("bitcoin-mainnet",     "bitcoin-mainnet:__native__",   NETWORK_NAME,  "btc",  "native",   NULL,   true)
    DEFINE_UNIT ("bitcoin-mainnet:__native__",      "Satoshi",    "sat",      0,      "SAT")
    DEFINE_UNIT ("bitcoin-mainnet:__native__",      NETWORK_NAME, "btc",      8,      "₿")
DEFINE_ADDRESS_SCHEMES  ("bitcoin-mainnet", CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT,   CRYPTO_ADDRESS_SCHEME_BTC_LEGACY)
DEFINE_MODES            ("bitcoin-mainnet", CRYPTO_SYNC_MODE_API_ONLY,          CRYPTO_SYNC_MODE_P2P_ONLY)

DEFINE_NETWORK (btcTestnet,  "bitcoin-testnet", NETWORK_NAME, "testnet", false, 1660401, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("bitcoin-testnet", "18", "10m", 10 * 60 * 1000)
DEFINE_CURRENCY ("bitcoin-testnet",     "bitcoin-testnet:__native__",   NETWORK_NAME,  "btc",  "native",   NULL,   true)
    DEFINE_UNIT ("bitcoin-testnet:__native__",      "Satoshi",    "sat",      0,      "SAT")
    DEFINE_UNIT ("bitcoin-testnet:__native__",      NETWORK_NAME, "btc",      8,      "₿")
DEFINE_ADDRESS_SCHEMES  ("bitcoin-testnet", CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT,   CRYPTO_ADDRESS_SCHEME_BTC_LEGACY)
DEFINE_MODES            ("bitcoin-testnet", CRYPTO_SYNC_MODE_API_ONLY,          CRYPTO_SYNC_MODE_P2P_ONLY)
#undef NETWORK_NAME

// MARK: - BCH

#define NETWORK_NAME    "Bitcoin Cash"
DEFINE_NETWORK (bchMainnet,  "bitcoincash-mainnet", NETWORK_NAME, "mainnet", true, 617640, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("bitcoincash-mainnet", "2", "10m", 20 * 60 * 1000)
DEFINE_CURRENCY ("bitcoincash-mainnet",     "bitcoincash-mainnet:__native__",   NETWORK_NAME,  "bch",  "native",   NULL,   true)
    DEFINE_UNIT ("bitcoincash-mainnet:__native__",      "Satoshi",    "sat",      0,      "SAT")
    DEFINE_UNIT ("bitcoincash-mainnet:__native__",      NETWORK_NAME, "bch",      8,      "BCH")
DEFINE_ADDRESS_SCHEMES  ("bitcoincash-mainnet", CRYPTO_ADDRESS_SCHEME_BTC_LEGACY)
DEFINE_MODES            ("bitcoincash-mainnet", CRYPTO_SYNC_MODE_API_ONLY, CRYPTO_SYNC_MODE_P2P_ONLY)

DEFINE_NETWORK (bchTestnet,  "bitcoincash-testnet", NETWORK_NAME, "testnet", false, 1353306, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("bitcoincash-testnet", "2", "10m", 20 * 60 * 1000)
DEFINE_CURRENCY ("bitcoincash-testnet",     "bitcoincash-testnet:__native__",   NETWORK_NAME,  "bch",  "native",   NULL,   true)
    DEFINE_UNIT ("bitcoincash-testnet:__native__",      "Satoshi",    "sat",      0,      "SAT")
    DEFINE_UNIT ("bitcoincash-testnet:__native__",      NETWORK_NAME, "bch",      8,      "BCH")
DEFINE_ADDRESS_SCHEMES  ("bitcoincash-testnet", CRYPTO_ADDRESS_SCHEME_BTC_LEGACY)
DEFINE_MODES            ("bitcoincash-testnet", CRYPTO_SYNC_MODE_API_ONLY,  CRYPTO_SYNC_MODE_P2P_ONLY)
#undef NETWORK_NAME

// MARK: - ETH

#define NETWORK_NAME    "Ethereum"
DEFINE_NETWORK (ethMainnet,  "ethereum-mainnet", NETWORK_NAME, "mainnet", true, 9273958, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("ethereum-mainnet", "25000000000", "1m", 1 * 60 * 1000)
DEFINE_CURRENCY ("ethereum-mainnet",     "ethereum-mainnet:__native__",   NETWORK_NAME,  "eth",  "native",   NULL,   true)
    DEFINE_UNIT ("ethereum-mainnet:__native__",      "Wei",         "wei",      0,      "WEI")
    DEFINE_UNIT ("ethereum-mainnet:__native__",      "Gwei",        "gwei",     9,      "GWEI")
    DEFINE_UNIT ("ethereum-mainnet:__native__",      "Ether",       "eth",     18,      "Ξ")
DEFINE_CURRENCY ("ethereum-mainnet",    "ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",  "BRD Token",    "brd",  "erc20",   "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",   true)
    DEFINE_UNIT ("ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",      "BRD Token INT",         "brdi",      0,      "BRDI")
    DEFINE_UNIT ("ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",      "BRD Token",             "brd",       18,     "BRD")
DEFINE_ADDRESS_SCHEMES  ("ethereum-mainnet", CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT)
DEFINE_MODES            ("ethereum-mainnet", CRYPTO_SYNC_MODE_API_ONLY, CRYPTO_SYNC_MODE_API_WITH_P2P_SEND, CRYPTO_SYNC_MODE_P2P_ONLY)

DEFINE_NETWORK (ethRopsten,  "ethereum-ropsten", NETWORK_NAME, "testnet", false, 7119019, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("ethereum-ropsten", "17500000000", "1m", 1 * 60 * 1000)
DEFINE_CURRENCY ("ethereum-ropsten",     "ethereum-ropsten:__native__",   NETWORK_NAME,  "eth",  "native",   NULL,   true)
    DEFINE_UNIT ("ethereum-ropsten:__native__",      "Wei",         "wei",      0,      "WEI")
    DEFINE_UNIT ("ethereum-ropsten:__native__",      "Gwei",        "gwei",     9,      "GWEI")
    DEFINE_UNIT ("ethereum-ropsten:__native__",      "Ether",       "eth",     18,      "Ξ")
DEFINE_CURRENCY ("ethereum-ropsten",    "ethereum-ropsten:0x7108ca7c4718efa810457f228305c9c71390931a",  "BRD Token Testnet",    "brd",  "erc20",   "0x7108ca7c4718efa810457f228305c9c71390931a",   true)
    DEFINE_UNIT ("ethereum-ropsten:0x7108ca7c4718efa810457f228305c9c71390931a",      "BRD Token INT",         "brdi",      0,      "BRDI")
    DEFINE_UNIT ("ethereum-ropsten:0x7108ca7c4718efa810457f228305c9c71390931a",      "BRD Token",             "brd",       18,     "BRD")
DEFINE_ADDRESS_SCHEMES  ("ethereum-ropsten", CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT)
DEFINE_MODES            ("ethereum-ropsten", CRYPTO_SYNC_MODE_API_ONLY, CRYPTO_SYNC_MODE_API_WITH_P2P_SEND, CRYPTO_SYNC_MODE_P2P_ONLY)
#undef NETWORK_NAME

// MARK: XRP

#define NETWORK_NAME    "Ripple"
DEFINE_NETWORK (xrpMainnet,  "ripple-mainnet", NETWORK_NAME, "mainnet", true, 52713481, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("ripple-mainnet", "10", "1m", 1 * 60 * 1000)
DEFINE_CURRENCY ("ripple-mainnet",     "ripple-mainnet:__native__",   NETWORK_NAME,  "xrp",  "native",   NULL,   true)
    DEFINE_UNIT ("ripple-mainnet:__native__",      "Drop",       "drop",      0,      "DROP")
    DEFINE_UNIT ("ripple-mainnet:__native__",      NETWORK_NAME, "xrp",       6,      "XRP")
DEFINE_ADDRESS_SCHEMES  ("ripple-mainnet", CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT)
DEFINE_MODES            ("ripple-mainnet", CRYPTO_SYNC_MODE_API_ONLY)

DEFINE_NETWORK (xrpTestnet,  "ripple-testnet", NETWORK_NAME, "testnet", false, 0, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("ripple-testnet", "10", "1m", 1 * 60 * 1000)
DEFINE_CURRENCY ("ripple-testnet",     "ripple-testnet:__native__",   NETWORK_NAME,  "xrp",  "native",   NULL,   true)
    DEFINE_UNIT ("ripple-testnet:__native__",      "Drop",       "drop",      0,      "DROP")
    DEFINE_UNIT ("ripple-testnet:__native__",      NETWORK_NAME, "xrp",       6,      "XRP")
DEFINE_ADDRESS_SCHEMES  ("ripple-testnet", CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT)
DEFINE_MODES            ("ripple-testnet", CRYPTO_SYNC_MODE_API_ONLY)
#undef NETWORK_NAME

// MARK: HBAR Mainnet

// MARK: XLM Mainnet

#undef DEFINE_NETWORK
#undef DEFINE_NETWORK_FEE_ESTIMATE
#undef DEFINE_CURRENCY
#undef DEFINE_UNIT
#undef DEFINE_ADDRESS_SCHEMES
#undef DEFINE_MODES
