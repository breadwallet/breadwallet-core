//
//  BREthereumEther
//  Core Ethereum
//
//  Created by Ed Gamble on 2/21/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Ether_H
#define BR_Ethereum_Ether_H

#include "ethereum/rlp/BRRlp.h"
#include "ethereum/util/BRUtil.h"
#include "BREthereumLogic.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum Ether Unit defines ETH units in powers of 10^3.   The 'base Unit' is WEI - the
 * smallest demonination of ETH and the one in which all integer values of the currency are counted.
 * Common 'derived units' are GWEI (3 => 10^9) and ETHER (6 => 10^18).
 */
typedef enum {
    WEI = 0,

    KWEI = 1,
    ADA  = 1,

    MWEI = 2,

    GWEI    = 3,
    SHANNON = 3,

    SZABO = 4,

    FINNEY = 5,

    ETHER = 6,

    KETHER   = 7,
    GRAND    = 7,
    EINSTEIN = 7,

    METHER = 8,
    GETHER = 9,
    TETHER = 10
} BREthereumEtherUnit;

#define NUMBER_OF_ETHER_UNITS  (1 + TETHER)

/**
 * Ether is the intrinsic currency of Ethereum.  It is represented as an integer value in WEI
 * and is commonly prescribed to have 256 bits.
 */
typedef struct BREthereumEtherStruct {
    UInt256 valueInWEI;
} BREthereumEther;

#define EMPTY_ETHER_INIT  ((const BREthereumEther) { UINT256_ZERO })
    
extern BREthereumEther
ethEtherCreateZero (void);

extern BREthereumEther
ethEtherCreate(const UInt256 value);

extern BREthereumEther
ethEtherCreateUnit(const UInt256 value, BREthereumEtherUnit unit, int *overflow);

extern BREthereumEther
ethEtherCreateNumber (uint64_t number, BREthereumEtherUnit unit);

/**
 * Create Ether from a decimal string in unit.  The `number` must be either an integer or have
 * a single decimal point with at least one preceeding characters.  Thus: 0.001, 1.0000, 12
 * and 12.100 are all valid.  But .1 is invalid (required 0.1).
 *
 * @warn what happens if invalid
 *
 */
extern BREthereumEther
ethEtherCreateString(const char *string, BREthereumEtherUnit unit, BRCoreParseStatus *status);

extern UInt256
ethEtherGetValue(const BREthereumEther ether, BREthereumEtherUnit unit);

/**
 * Return a decimal string representing `ether` in `unit`.
 *
 * @param ether -
 * @param unit -
 *
 * @return A newly allocated string (you own it).
 */
extern char *
ethEtherGetValueString(const BREthereumEther ether, BREthereumEtherUnit unit);

extern BRRlpItem
ethEtherRlpEncode (const BREthereumEther ether, BRRlpCoder coder);

extern BREthereumEther
ethEtherRlpDecode (BRRlpItem item, BRRlpCoder coder);
    
extern BREthereumEther
ethEtherAdd (BREthereumEther e1, BREthereumEther e2, int *overflow);

extern BREthereumEther
ethEtherSub (BREthereumEther e1, BREthereumEther e2, int *negative);

//
// Comparisons
//
extern BREthereumBoolean
ethEtherIsEQ (BREthereumEther e1, BREthereumEther e2);

extern BREthereumBoolean
ethEtherIsGT (BREthereumEther e1, BREthereumEther e2);

extern BREthereumBoolean
ethEtherIsGE (BREthereumEther e1, BREthereumEther e2);

extern BREthereumBoolean
ethEtherIsLT (BREthereumEther e1, BREthereumEther e2);

extern BREthereumBoolean
ethEtherIsLE (BREthereumEther e1, BREthereumEther e2);

extern BREthereumBoolean
ethEtherIsZero (BREthereumEther e);

extern BREthereumComparison
ethEtherCompare (BREthereumEther e1, BREthereumEther e2);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Ether_H */

