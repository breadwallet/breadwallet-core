//
//  BRCryptoBase.h
//  BRCore
//
//  Created by Ed Gamble on 3/19/19.
//  Copyright © 2019 breadwallet. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRCryptoBase_h
#define BRCryptoBase_h

#include <inttypes.h>
#include <stdatomic.h>
#include "support/BRBase.h"
#include "support/BRInt.h"
#include "support/BRSyncMode.h"
// temporary
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRCryptoWalletRecord *BRCryptoWallet;

    typedef struct BRCryptoWalletManagerRecord *BRCryptoWalletManager;

    // Cookies are used as markers to match up an asynchronous operation
    // request with its corresponding event.
    typedef void *BRCryptoCookie;

    typedef enum {
        CRYPTO_FALSE = 0,
        CRYPTO_TRUE  = 1
    } BRCryptoBoolean;

#define AS_CRYPTO_BOOLEAN(zeroIfFalse)   ((zeroIfFalse) ? CRYPTO_TRUE : CRYPTO_FALSE)

    typedef BRBlockHeight BRCryptoBlockChainHeight;

    // Private-ish
    typedef enum {
        BLOCK_CHAIN_TYPE_BTC,
        BLOCK_CHAIN_TYPE_ETH,
        BLOCK_CHAIN_TYPE_GEN
    } BRCryptoBlockChainType;


    /// MARK: Reference Counting

    typedef struct {
        _Atomic(unsigned int) count;
        void (*free) (void *);
    } BRCryptoRef;

#if !defined (CRYPTO_REF_DEBUG)
#define CRYPTO_REF_DEBUG 0
#endif

#define DECLARE_CRYPTO_GIVE_TAKE(type, preface)                                   \
  extern type preface##Take (type obj);                                           \
  extern type preface##TakeWeak (type obj);                                       \
  extern void preface##Give (type obj)

#define IMPLEMENT_CRYPTO_GIVE_TAKE(type, preface)                                 \
  extern type                                                                     \
  preface##Take (type obj) {                                                      \
    unsigned int _c = atomic_fetch_add (&obj->ref.count, 1);                      \
    /* catch take after release */                                                \
    assert (0 != _c);                                                             \
    return obj;                                                                   \
  }                                                                               \
  extern type                                                                     \
  preface##TakeWeak (type obj) {                                                  \
    unsigned int _c = atomic_load(&obj->ref.count);                               \
    /* keep trying to take unless object is released */                           \
    while (_c != 0 &&                                                             \
           !atomic_compare_exchange_weak (&obj->ref.count, &_c, _c + 1)) {}       \
    if (0 != CRYPTO_REF_DEBUG && 0 == _c) { printf ("CRY: Missed: %s\n", #type); }\
    return (_c != 0) ? obj : NULL;                                                \
  }                                                                               \
  extern void                                                                     \
  preface##Give (type obj) {                                                      \
    unsigned int _c = atomic_fetch_sub (&obj->ref.count, 1);                      \
    /* catch give after release */                                                \
    assert (0 != _c);                                                             \
    if (1 == _c) {                                                                \
        if (0 != CRYPTO_REF_DEBUG) { printf ("CRY: Release: %s\n", #type); }      \
        obj->ref.free (obj);                                                      \
    }                                                                             \
  }

#define CRYPTO_AS_FREE(release)     ((void (*) (void *)) release)

#define CRYPTO_REF_ASSIGN(release)  (BRCryptoRef) { 1, CRYPTO_AS_FREE (release) }

#if !defined (private_extern)
#  define private_extern          extern
#endif

#ifdef __cplusplus
}
#endif

#endif /* BRCryptoBase_h */
