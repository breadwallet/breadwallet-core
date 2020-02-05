//
//  BREthereumToken
//  Core Ethereum
//
//  Created by Ed Gamble on 3/15/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Token_H
#define BR_Ethereum_Token_H

#include "ethereum/base/BREthereumBase.h"
#include "BREthereumContract.h"

// For tokenBRD define defaults for Gas Limit and Price.  These are arguably never up to date
// and thus should be changed in programmatically using walletSetDefaultGas{Price,Limit}().
#define TOKEN_BRD_DEFAULT_GAS_LIMIT  92000
#define TOKEN_BRD_DEFAULT_GAS_PRICE_IN_WEI_UINT64 (2000000000) // 2.0 GWEI

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum ERC20 Token
 */
typedef struct BREthereumTokenRecord *BREthereumToken;

extern BREthereumAddress
ethTokenGetAddressRaw (BREthereumToken token);

/**
 * Return the token address as a '0x'-prefixed string.  DO NOT FREE THIS.
 */
extern const char *
ethTokenGetAddress (BREthereumToken token);

/**
 * Check if `token` has `address` which must be a '0x'-prefixed hex string.
 */
extern BREthereumBoolean
ethTokenHasAddress (BREthereumToken token,
                    const char *address);

extern const char *
ethTokenGetSymbol (BREthereumToken token);

extern const char *
ethTokenGetName (BREthereumToken token);

extern const char *
ethTokenGetDescription(BREthereumToken token);

extern int
ethTokenGetDecimals (BREthereumToken token);

extern BREthereumGas
ethTokenGetGasLimit (BREthereumToken token);

extern BREthereumGasPrice
ethTokenGetGasPrice (BREthereumToken token);

extern BREthereumContract
ethTokenGetContract (BREthereumToken token);

extern BREthereumHash
ethTokenGetHash (BREthereumToken token);

extern BREthereumToken
ethTokenCreate (const char *address,
                const char *symbol,
                const char *name,
                const char *description,
                int decimals,
                BREthereumGas defaultGasLimit,
                BREthereumGasPrice defaultGasPrice);

extern void
ethTokenRelease (BREthereumToken token);

extern void
ethTokenUpdate (BREthereumToken token,
                const char *symbol,
                const char *name,
                const char *description,
                int decimals,
                BREthereumGas defaultGasLimit,
                BREthereumGasPrice defaultGasPrice);

extern BRRlpItem
ethTokenRlpEncode (BREthereumToken token,
                   BRRlpCoder coder);

extern BREthereumToken
ethTokenRlpDecode (BRRlpItem item,
                   BRRlpCoder coder);

extern BRSetOf(BREthereumToken)
ethTokenSetCreate (size_t capacity);

//
// Token Quantity
//

/**
 * A BREthereumTokenQuantityUnit defines the (external) representation of a token quantity
 */
typedef enum {
    TOKEN_QUANTITY_TYPE_DECIMAL,
    TOKEN_QUANTITY_TYPE_INTEGER
} BREthereumTokenQuantityUnit;

/**
 * A BREthereumTokenQuantity defines a token amount.
 *
 */
typedef struct {
    BREthereumToken token;
    UInt256 valueAsInteger;
} BREthereumTokenQuantity;

extern BREthereumTokenQuantity
ethTokenQuantityCreate (BREthereumToken token,
                        UInt256 valueAsInteger);

extern BREthereumTokenQuantity
ethTokenQuantityCreateString (BREthereumToken token,
                              const char *number,
                              BREthereumTokenQuantityUnit unit,
                              BRCoreParseStatus *status);

extern const BREthereumToken
ethTokenQuantityGetToken (BREthereumTokenQuantity quantity);

/**
 * A newly allocated string; you own it.
 *
 * @param quantity
 * @param unit
 * @return
 */
extern char *
ethTokenQuantityGetValueString(const BREthereumTokenQuantity quantity,
                               BREthereumTokenQuantityUnit unit);

extern BREthereumComparison
ethTokenQuantityCompare (BREthereumTokenQuantity q1, BREthereumTokenQuantity q2, int *typeMismatch);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Token_H

