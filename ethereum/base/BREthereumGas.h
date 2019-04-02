//
//  BREthereumGas
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/24/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Gas_H
#define BR_Ethereum_Gas_H

#include "support/BRInt.h"
#include "BREthereumEther.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Ethereum Gas is a measure of the work associated with a transaction.  Transactions submitted to
 * the Ethereum P2P network require computation; gas is a measure of that computation.
 */
typedef struct BREthereumGasStruct {
    uint64_t amountOfGas;
} BREthereumGas;

extern BREthereumGas
gasCreate(uint64_t gas);

extern BREthereumComparison
gasCompare (BREthereumGas e1, BREthereumGas e2);

extern BRRlpItem
gasRlpEncode (BREthereumGas gas, BRRlpCoder coder);

extern BREthereumGas
gasRlpDecode (BRRlpItem item, BRRlpCoder coder);

/**
 * Ethereum Gas Price is the amount of Ether for one Gas - aka Ether/Gas.  The total cost for
 * an Ethereum transaction is the Gas Price * Gas (used).
 *
 * "If you Gas Price is too low, nobody will process your transaction".  You'll want a Gas Price
 * that is high enough to ensure the transaction is processed within your desired time frame; but
 * not so high that you overpay with no advantage (in confirmation time).
 */
typedef struct BREthereumGasPriceStruct {
    BREthereumEther etherPerGas;
} BREthereumGasPrice;

/**
 * Create EthereumGasPrice as `ether` per one Gas.  A typical value would be ~2 GWEI / Gas.
 *
 * @param ether
 * @return
 */
extern BREthereumGasPrice
gasPriceCreate(BREthereumEther ether);

extern BREthereumComparison
gasPriceCompare (BREthereumGasPrice e1, BREthereumGasPrice e2);

/**
 * Compute the Gas Cost (in Ether) for a given Gas Price and Gas.  This can overflow; on overflow
 * the returned Ether is 0(!).
 *
 * @param price The Ether/Gas
 * @param gas  The Gas
 * @param overflow Set to 1 if overflow; 0 otherwise. MUST NOT BE NULL.
 * @return
 */
extern BREthereumEther
gasPriceGetGasCost(BREthereumGasPrice price, BREthereumGas gas, int *overflow);

extern BRRlpItem
gasPriceRlpEncode (BREthereumGasPrice price, BRRlpCoder coder);

extern BREthereumGasPrice
gasPriceRlpDecode (BRRlpItem item, BRRlpCoder coder);
    
#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Gas_H
