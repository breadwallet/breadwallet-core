//
//  BRGenericBase.h
//  Core
//
//  Created by Ed Gamble on 6/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BRGenericBase_h
#define BRGenericBase_h

#include "ethereum/util/BRUtil.h"

#ifdef __cplusplus
extern "C" {
#endif

    typedef struct BRGenericAccountRecord  *BRGenericAccount;
    typedef struct BRGenericNetworkRecord  *BRGenericNetwork;
    typedef struct BRGenericAddressRecord  *BRGenericAddress;
    typedef struct BRGenericFeeBasisRecord *BRGenericFeeBasis;
    typedef struct BRGenericTransferRecord *BRGenericTransfer;
    typedef struct BRGenericWalletRecord   *BRGenericWallet;
    typedef struct BRGenericManagerRecord  *BRGenericManager;

    // MARK: - Generic Hash

    typedef struct {
        UInt256 value;
    } BRGenericHash;

    static inline int
    genericHashEqual (BRGenericHash gen1, BRGenericHash gen2) {
        return eqUInt256 (gen1.value, gen2.value);
    }

    static inline int
    genericHashIsEmpty (BRGenericHash gen) {
        return eqUInt256 (gen.value, UINT256_ZERO);
    }

    static inline char *
    genericHashAsString (BRGenericHash gen) {
        return encodeHexCreate (NULL, gen.value.u8, sizeof (gen.value.u8));
    }

    static inline uint32_t
    genericHashSetValue (BRGenericHash gen) {
        return gen.value.u32[0];
    }

    // MARK: Generic API Sync Type

    typedef enum {
        GENERIC_SYNC_TYPE_TRANSACTION,
        GENERIC_SYNC_TYPE_TRANSFER
    } BRGenericAPISyncType;

#ifdef __cplusplus
}
#endif

#endif /* BRGenericBase_h */
