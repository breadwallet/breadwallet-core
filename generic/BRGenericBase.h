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

#include <stdint.h>
#include <math.h>   // fabs() - via static inline
#include "BRCryptoBase.h"
#include "ethereum/util/BRUtil.h"
#include "support/BRArray.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * The installed generic networks.  Use this: a) as the `type` in `BRGenericHandlers` for each
 * network and b) in `cryptoNetworkCreateBuiltin()` when creating the BRCryptoNetwork for this
 * generic network.
 */
#   define GEN_NETWORK_TYPE_XRP        "xrp"
//# define GEN_NETWORK_TYPE_HBAR       "hbar"
//# define GEN_NETWORK_TYPE_XLM        "xlm"

    typedef struct BRGenericAccountRecord  *BRGenericAccount;
    typedef struct BRGenericNetworkRecord  *BRGenericNetwork;
    typedef struct BRGenericAddressRecord  *BRGenericAddress;
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

    // MARK: Generic Fee Basis

    typedef struct {
        UInt256 pricePerCostFactor;
        double  costFactor;
    } BRGenericFeeBasis;

    static inline BRGenericFeeBasis
    genFeeBasisCreate (UInt256 pricePerCostFactor, double costFactor) {
        return (BRGenericFeeBasis) {
            pricePerCostFactor,
            fabs (costFactor)
        };
    }

    static inline UInt256
    genFeeBasisGetPricePerCostFactor (const BRGenericFeeBasis *feeBasis) {
        return feeBasis->pricePerCostFactor;
    }

    static inline double
    genFeeBasisGetCostFactor (const BRGenericFeeBasis *feeBasis) {
        return feeBasis->costFactor;
    }

    static inline UInt256
    genFeeBasisGetFee (const BRGenericFeeBasis *feeBasis, int *overflow) {
        double rem;
        int negative;

        return mulUInt256_Double (feeBasis->pricePerCostFactor,
                                  feeBasis->costFactor,
                                  overflow,
                                  &negative,
                                  &rem);
    }

    static inline int genFeeBasisIsEqual (const BRGenericFeeBasis *fb1,
                                          const BRGenericFeeBasis *fb2) {
        return (eqUInt256 (fb1->pricePerCostFactor, fb2->pricePerCostFactor) &&
                fb1->costFactor == fb2->costFactor);
    }

    // MARK: Generic API Sync Type

    typedef enum {
        GENERIC_SYNC_TYPE_TRANSACTION,
        GENERIC_SYNC_TYPE_TRANSFER
    } BRGenericAPISyncType;

    // MARK: Generic Transfer Direction
    typedef enum {
        GENERIC_TRANSFER_SENT,
        GENERIC_TRANSFER_RECEIVED,
        GENERIC_TRANSFER_RECOVERED
    } BRGenericTransferDirection;

    // MARK: - Generic Transfer State

    typedef enum {
        GENERIC_TRANSFER_SUBMIT_ERROR_ONE,
        GENERIC_TRANSFER_SUBMIT_ERROR_TWO,
    } BRGenericTransferSubmitError;

    typedef enum {
        GENERIC_TRANSFER_STATE_CREATED,
        GENERIC_TRANSFER_STATE_SIGNED,
        GENERIC_TRANSFER_STATE_SUBMITTED,
        GENERIC_TRANSFER_STATE_INCLUDED,
        GENERIC_TRANSFER_STATE_ERRORED,
        GENERIC_TRANSFER_STATE_DELETED,
    } BRGenericTransferStateType;

#define GENERIC_TRANSFER_INCLUDED_ERROR_SIZE     16

    typedef struct {
        BRGenericTransferStateType type;
        union {
            struct {
                uint64_t blockNumber;
                uint64_t transactionIndex;
                uint64_t timestamp;
                BRGenericFeeBasis feeBasis;
                BRCryptoBoolean success;
                char error [GENERIC_TRANSFER_INCLUDED_ERROR_SIZE + 1];
            } included;
            BRGenericTransferSubmitError errored;
        } u;
    } BRGenericTransferState;

#define GENERIC_TRANSFER_BLOCK_NUMBER_UNKNOWN       (UINT64_MAX)
#define GENERIC_TRANSFER_TRANSACTION_INDEX_UNKNOWN  (UINT64_MAX)
#define GENERIC_TRANSFER_TIMESTAMP_UNKNOWN          (UINT64_MAX)

    static inline BRGenericTransferState
    genTransferStateCreateIncluded (uint64_t blockNumber,
                                    uint64_t transactionIndex,
                                    uint64_t timestamp,
                                    BRGenericFeeBasis feeBasis,
                                    BRCryptoBoolean success,
                                    const char *error) {
        BRGenericTransferState result = (BRGenericTransferState) {
            GENERIC_TRANSFER_STATE_INCLUDED,
            { .included = {
                blockNumber,
                transactionIndex,
                timestamp,
                feeBasis,
                success
            }}
        };

        memset (result.u.included.error, 0, GENERIC_TRANSFER_INCLUDED_ERROR_SIZE + 1);
        if (CRYPTO_FALSE == success) {
            strlcpy (result.u.included.error,
                     (NULL == error ? "unknown error" : error),
                     GENERIC_TRANSFER_INCLUDED_ERROR_SIZE + 1);
        }

        return result;

    }

    static inline BRGenericTransferState
    genTransferStateCreateErrored (BRGenericTransferSubmitError error) {
        return (BRGenericTransferState) {
            GENERIC_TRANSFER_STATE_ERRORED,
            { .errored = error }
        };
    }

    static inline BRGenericTransferState
    genTransferStateCreateOther (BRGenericTransferStateType type) {
        return (BRGenericTransferState) { type };
    }

    static inline int
    genTransferStateEqual (BRGenericTransferState state1,
                           BRGenericTransferState state2) {
        return state1.type == state2.type &&
        ((GENERIC_TRANSFER_STATE_INCLUDED != state1.type && GENERIC_TRANSFER_STATE_ERRORED != state1.type) ||
         (GENERIC_TRANSFER_STATE_INCLUDED == state1.type                           &&
          state1.u.included.blockNumber      == state2.u.included.blockNumber      &&
          state1.u.included.transactionIndex == state2.u.included.transactionIndex &&
          state1.u.included.timestamp        == state2.u.included.timestamp        &&
          genFeeBasisIsEqual (&state1.u.included.feeBasis,
                              &state2.u.included.feeBasis))                        ||
         (GENERIC_TRANSFER_STATE_ERRORED == state1.type && state1.u.errored == state2.u.errored));
    }

    static inline int
    genTransferStateHasType (BRGenericTransferState state,
                             BRGenericTransferStateType type) {
        return type == state.type;
    }

    static inline int
    genTransferStateHasTypeOr (BRGenericTransferState state,
                               BRGenericTransferStateType type1,
                               BRGenericTransferStateType type2) {
        return type1 == state.type || type2 == state.type;
    }

    // MARK: - Transfer Attribute

    /// This is an utter duplicate of BRCryptoTransferAttribute - except it doesn't do
    /// reference counting.  We can't use the BRCrypto version becuase of circularities, or just
    /// potential circularties, in header files.
    typedef struct BRGenericTransferAttributeRecord *BRGenericTransferAttribute;

    extern BRGenericTransferAttribute // copies key, val
    genTransferAttributeCreate (const char *key, const char *val, int isRequired);

    extern void // frees key, val
    genTransferAttributeRelease (BRGenericTransferAttribute attribute);

    extern const char *
    genTransferAttributeGetKey (BRGenericTransferAttribute attribute);

    extern const char *
    genTransferAttributeGetVal (BRGenericTransferAttribute attribute);

    extern int
    genTransferAttributeIsRequired (BRGenericTransferAttribute attribute);

    extern void
    genTransferAttributeReleaseAll (OwnershipGiven BRArrayOf(BRGenericTransferAttribute) attributes);

#ifdef __cplusplus
}
#endif

#endif /* BRGenericBase_h */
