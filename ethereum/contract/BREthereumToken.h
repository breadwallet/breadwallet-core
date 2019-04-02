//
//  BREthereumToken
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/15/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
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
tokenGetAddressRaw (BREthereumToken token);
    
/**
 * Return the token address as a '0x'-prefixed string.  DO NOT FREE THIS.
 */
extern const char *
tokenGetAddress (BREthereumToken token);

extern const char *
tokenGetSymbol (BREthereumToken token);

extern const char *
tokenGetName (BREthereumToken token);

extern const char *
tokenGetDescription(BREthereumToken token);

extern int
tokenGetDecimals (BREthereumToken token);

extern BREthereumGas
tokenGetGasLimit (BREthereumToken token);

extern BREthereumGasPrice
tokenGetGasPrice (BREthereumToken token);

extern BREthereumContract
tokenGetContract (BREthereumToken token);

extern BREthereumToken
tokenLookupByAddress (BREthereumAddress address);
    
extern BREthereumToken
tokenLookup (const char *address);

extern int
tokenCount (void);

/**
 * Return a newly allocated array with references to all tokens
 */
extern BREthereumToken *
tokenGetAll (void);

extern void
tokenInstall (const char *address,
              const char *symbol,
              const char *name,
              const char *description,
              int decimals,
              BREthereumGas defaultGasLimit,
              BREthereumGasPrice defaultGasPrice);


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
createTokenQuantity (BREthereumToken token,
                     UInt256 valueAsInteger);

extern BREthereumTokenQuantity
createTokenQuantityString (BREthereumToken token,
                           const char *number,
                           BREthereumTokenQuantityUnit unit,
                           BRCoreParseStatus *status);

extern const BREthereumToken
tokenQuantityGetToken (BREthereumTokenQuantity quantity);

/**
 * A newly allocated string; you own it.
 *
 * @param quantity
 * @param unit
 * @return
 */
extern char *
tokenQuantityGetValueString(const BREthereumTokenQuantity quantity,
                            BREthereumTokenQuantityUnit unit);

extern BREthereumComparison
tokenQuantityCompare (BREthereumTokenQuantity q1, BREthereumTokenQuantity q2, int *typeMismatch);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Token_H

