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

// MARK: - BTC Mainnet

DEFINE_NETWORK (btcMainnet,  "bitcoin-mainnet", "Bitcoin", "mainnet", true, 595000, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("bitcoin-mainnet", "30", "10m", 10 * 60 * 1000)
DEFINE_CURRENCY ("bitcoin-mainnet",     "bitcoin-mainnet:__native__",   "Bitcoin",  "btc",  "native",   NULL,   true)
    DEFINE_UNIT ("bitcoin-mainnet:__native__",      "Satoshi",      "sat",      0,      "SAT")
    DEFINE_UNIT ("bitcoin-mainnet:__native__",      "Bitcoin",      "btc",      8,      "₿")
DEFINE_ADDRESS_SCHEMES  ("bitcoin-mainnet", CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT,   CRYPTO_ADDRESS_SCHEME_BTC_LEGACY)
DEFINE_MODES            ("bitcoin-mainnet", CRYPTO_SYNC_MODE_API_ONLY,          CRYPTO_SYNC_MODE_P2P_ONLY)

// MARK: - BTC Testnet

DEFINE_NETWORK (btcTestnet,  "bitcoin-testnet", "Bitcoin Testnet", "testnet", false, 1575000, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("bitcoin-testnet", "30", "10m", 10 * 60 * 1000)
DEFINE_CURRENCY ("bitcoin-testnet",     "bitcoin-testnet:__native__",   "Bitcoin",  "btc",  "native",   NULL,   true)
    DEFINE_UNIT ("bitcoin-testnet:__native__",      "Satoshi",          "sat",      0,      "SAT")
    DEFINE_UNIT ("bitcoin-testnet:__native__",      "Bitcoin Testnet",  "btc",      8,      "₿")
DEFINE_ADDRESS_SCHEMES  ("bitcoin-testnet", CRYPTO_ADDRESS_SCHEME_BTC_SEGWIT,   CRYPTO_ADDRESS_SCHEME_BTC_LEGACY)
DEFINE_MODES            ("bitcoin-testnet", CRYPTO_SYNC_MODE_API_ONLY,          CRYPTO_SYNC_MODE_P2P_ONLY)

// MARK: - BCH Mainnet

DEFINE_NETWORK (bchMainnet,  "bitcoincash-mainnet", "Bitcoin Cash", "mainnet", true, 600000, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("bitcoincash-mainnet", "30", "10m", 10 * 60 * 1000)
DEFINE_CURRENCY ("bitcoincash-mainnet",     "bitcoincash-mainnet:__native__",   "Bitcoin Cash",  "bch",  "native",   NULL,   true)
    DEFINE_UNIT ("bitcoincash-mainnet:__native__",      "Satoshi",      "sat",      0,      "SAT")
    DEFINE_UNIT ("bitcoincash-mainnet:__native__",      "Bitcoin",      "bch",      8,      "BCH")
DEFINE_ADDRESS_SCHEMES  ("bitcoincash-mainnet", CRYPTO_ADDRESS_SCHEME_BTC_LEGACY)
DEFINE_MODES            ("bitcoincash-mainnet", CRYPTO_SYNC_MODE_API_ONLY, CRYPTO_SYNC_MODE_P2P_ONLY)

// MARK: - BCH Testnet

DEFINE_NETWORK (bchTestnet,  "bitcoincash-testnet", "Bitcoin Cash Testnet", "testnet", false, 1325000, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("bitcoincash-testnet", "30", "10m", 10 * 60 * 1000)
DEFINE_CURRENCY ("bitcoincash-testnet",     "bitcoincash-testnet:__native__",   "Bitcoin Cash Testnet",  "bch",  "native",   NULL,   true)
    DEFINE_UNIT ("bitcoincash-testnet:__native__",      "Satoshi",              "sat",      0,      "SAT")
    DEFINE_UNIT ("bitcoincash-testnet:__native__",      "Bitcoin Cash Testnet", "bch",      8,      "BCH")
DEFINE_ADDRESS_SCHEMES  ("bitcoincash-testnet", CRYPTO_ADDRESS_SCHEME_BTC_LEGACY)
DEFINE_MODES            ("bitcoincash-testnet", CRYPTO_SYNC_MODE_API_ONLY,  CRYPTO_SYNC_MODE_P2P_ONLY)

// MARK: - ETH Mainnet

DEFINE_NETWORK (ethMainnet,  "ethereum-mainnet", "Ethereum", "mainnet", true, 8570000, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("ethereum-mainnet", "2000000000", "1m", 1 * 60 * 1000)
DEFINE_CURRENCY ("ethereum-mainnet",     "ethereum-mainnet:__native__",   "Ethereum",  "eth",  "native",   NULL,   true)
    DEFINE_UNIT ("ethereum-mainnet:__native__",      "Wei",         "wei",      0,      "WEI")
    DEFINE_UNIT ("ethereum-mainnet:__native__",      "Gwei",        "gwei",     9,      "GWEI")
    DEFINE_UNIT ("ethereum-mainnet:__native__",      "Ether",       "eth",     18,      "Ξ")
DEFINE_CURRENCY ("ethereum-mainnet",    "ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",  "BRD Token",    "brd",  "erc20",   "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",   true)
    DEFINE_UNIT ("ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",      "BRD Token INT",         "BRDI",      0,      "BRDI")
    DEFINE_UNIT ("ethereum-mainnet:0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",      "BRD Token",             "BRD",       18,     "BRD")
DEFINE_ADDRESS_SCHEMES  ("ethereum-mainnet", CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT)
DEFINE_MODES            ("ethereum-mainnet", CRYPTO_SYNC_MODE_API_ONLY, CRYPTO_SYNC_MODE_API_WITH_P2P_SEND, CRYPTO_SYNC_MODE_P2P_ONLY)

// MARK: - ETH Testnet/Ropsten

DEFINE_NETWORK (ethRopsten,  "ethereum-ropsten", "Ethereum Ropsten", "testnet", false, 6415000, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("ethereum-ropsten", "2000000000", "1m", 1 * 60 * 1000)
DEFINE_CURRENCY ("ethereum-ropsten",     "ethereum-ropsten:__native__",   "Ethereum Ropsten",  "eth",  "native",   NULL,   true)
    DEFINE_UNIT ("ethereum-ropsten:__native__",      "Wei",         "wei",      0,      "WEI")
    DEFINE_UNIT ("ethereum-ropsten:__native__",      "Gwei",        "gwei",     9,      "GWEI")
    DEFINE_UNIT ("ethereum-ropsten:__native__",      "Ether",       "eth",     18,      "Ξ")
DEFINE_CURRENCY ("ethereum-ropsten",    "ethereum-ropsten:0x7108ca7c4718efa810457f228305c9c71390931a",  "BRD Token Testnet",    "brd",  "erc20",   "0x7108ca7c4718efa810457f228305c9c71390931a",   true)
    DEFINE_UNIT ("ethereum-ropsten:0x7108ca7c4718efa810457f228305c9c71390931a",      "BRD Token INT",         "BRDI",      0,      "BRDI")
    DEFINE_UNIT ("ethereum-ropsten:0x7108ca7c4718efa810457f228305c9c71390931a",      "BRD Token",             "BRD",       18,     "BRD")
DEFINE_ADDRESS_SCHEMES  ("ethereum-ropsten", CRYPTO_ADDRESS_SCHEME_ETH_DEFAULT)
DEFINE_MODES            ("ethereum-ropsten", CRYPTO_SYNC_MODE_API_ONLY, CRYPTO_SYNC_MODE_API_WITH_P2P_SEND, CRYPTO_SYNC_MODE_P2P_ONLY)

// MARK: XRP Mainnet

#if defined (NEED_XRP)
DEFINE_NETWORK (xrpMainnet,  "ripple-mainnet", "Ripple", "mainnet", true, 0, 6)
DEFINE_NETWORK_FEE_ESTIMATE ("ripple-mainnet", "10", "1m", 1 * 60 * 1000)
DEFINE_CURRENCY ("ripple-mainnet",     "ripple-mainnet:__native__",   "Ripple",  "xrp",  "native",   NULL,   true)
    DEFINE_UNIT ("ripple-mainnet:__native__",      "drop",      "drop",      0,      "drop")
    DEFINE_UNIT ("ripple-mainnet:__native__",      "xrp",       "xrp",       6,      "xrp")
DEFINE_ADDRESS_SCHEMES  ("ripple-mainnet", CRYPTO_ADDRESS_SCHEME_GEN_DEFAULT)
DEFINE_MODES            ("ripple-mainnet", CRYPTO_SYNC_MODE_API_ONLY)
#endif

#undef DEFINE_NETWORK
#undef DEFINE_NETWORK_FEE_ESTIMATE
#undef DEFINE_CURRENCY
#undef DEFINE_UNIT
#undef DEFINE_ADDRESS_SCHEMES
#undef DEFINE_MODES
