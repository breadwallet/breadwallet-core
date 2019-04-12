//
//  BREthereumAmount
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/25/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Amount_H
#define BR_Ethereum_Amount_H

#include "ethereum/base/BREthereumBase.h"
#include "ethereum/contract/BREthereumToken.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum Wallet holds either ETHER or TOKENs.
 */
typedef enum {
    AMOUNT_ETHER,
    AMOUNT_TOKEN
} BREthereumAmountType;

/**
 * An Ethereum Wallet holds a specific amount of ETHER or TOKENs
 */
typedef struct BREthereumAmountRecord {
  BREthereumAmountType type;
  union {
    BREthereumEther ether;
    BREthereumTokenQuantity tokenQuantity;
  } u;
} BREthereumAmount;

extern BREthereumAmount
amountCreateEther (BREthereumEther ether);

// TODO: what is 'scale' - replace with 'decimals'?
extern BREthereumAmount
amountCreateToken (BREthereumTokenQuantity tokenQuantity);

extern BREthereumAmountType
amountGetType (BREthereumAmount amount);

/**
 * The amount's ether if holding ETHER; otherwise fatal.
 */
extern BREthereumEther
amountGetEther (BREthereumAmount amount);

/**
 * The amount's tokenQuantity if holding TOKEN; otherwise fatal.
 */
extern BREthereumTokenQuantity
amountGetTokenQuantity (BREthereumAmount amount);

extern BREthereumToken
amountGetToken (BREthereumAmount amount);
    
extern BREthereumComparison
amountCompare (BREthereumAmount a1, BREthereumAmount a2, int *typeMismatch);

/**
 * An estimate of the Gas required to transfer amount.  For ETHER this is 'fixed' as 22000; for
 * TOKEN this is derived from the token's properties.
 */
extern BREthereumGas
amountGetGasEstimate (BREthereumAmount amount);

extern BRRlpItem
amountRlpEncode(BREthereumAmount amount, BRRlpCoder coder);

extern BREthereumAmount
amountRlpDecodeAsEther (BRRlpItem item, BRRlpCoder coder);

extern BREthereumAmount
amountRlpDecodeAsToken (BRRlpItem item, BRRlpCoder coder, BREthereumToken token);

//
// Parsing
//

/**
 *
 * Parse a string of base-10 digits with one optional decimal point into an Ether holding and
 * assign the status.  If status is 'OK' then the holding will contain some amount of Ether;
 * otherwise status will indicate the failure and holding will contain zero Ether.
 *
 * Examples of number are: "12.3", "12000000000", "0.00000000012", "1.000000000023"
 *
 * Status Errors are:
 *     STRANGE_DIGITS: 12a.3f <all characters must be [0-9\.]
 *          UNDERFLOW: 0.1 WEI
 *           OVERFLOW: 1000000 TETHER
 *
 * @param number
 * @param unit
 * @param status
 *
 */
extern BREthereumAmount
amountCreateEtherString (const char *number,
                          BREthereumEtherUnit unit,
                          BRCoreParseStatus *status);

/**
 *
 * Parse a string of base-10 digits with one optional decimal point into an Token holding and
 * assign the status.  If status is 'OK' then the holding will contain some amount of Token;
 * otherwise status will indicate the failure and holding will contain nothing.
 *
 * Examples of number are: "12.3", "12000000000", "0.00000000012", "1.000000000023"
 *
 * Status Errors are:
 *     STRANGE_DIGITS: 12a.3f <all characters must be [0-9\.]
 *          UNDERFLOW: 0.1 WEI
 *           OVERFLOW: 1000000 TETHER
 *
 * @param number
 * @param unit
 * @param status
 *
 */
extern BREthereumAmount
amountCreateTokenQuantityString (BREthereumToken token,
                                const char *number,
                                 BREthereumTokenQuantityUnit unit,
                                 BRCoreParseStatus *status);

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Amount_H
