//
//  BREthereumContract
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 3/5/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#ifndef BR_Ethereum_Contract_h
#define BR_Ethereum_Contract_h

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREthereumFunctionRecord *BREthereumContractFunction;
typedef struct BREthereumEventRecord *BREthereumContractEvent;
typedef struct BREthereumContractRecord *BREthereumContract;


extern BREthereumContract contractERC20;
extern BREthereumContractFunction functionERC20Transfer; // "transfer(address,uint256)"
extern BREthereumContractEvent eventERC20Transfer;       // "Transfer(address indexed _from, address indexed _to, uint256 _value)"

/**
 *
 */
extern const char *
eventGetSelector (BREthereumContractEvent event);

/**
 * Encode an Ehtereum function with arguments.  The specific arguments and their types are
 * defined on a function-by-function basis.  For each function argument, contractEncode() is
 * called with a pair as (uint8_t *bytes, size_t bytesCount).  Thus for example, an ERC20
 * token transfer would be called as:
 *
 * This will be prefaced with '0x'
 *
 * char *address;
 * UInt256 amount;
 *
 * char *encoding contractEncode (contractERC20, functionERC20Transfer,
 *          (uint8_t *) address, strlen(address),
 *          (uint8_t *) &amount, sizeof (UInt256),
 *          NULL);  // end marker -> 'bytes' is never NULL.
 * ...
 * free (encoding);
 */
extern const char *
contractEncode (BREthereumContract contract, BREthereumContractFunction function, ...);

/**
 * Return the function for `encodeing` or NULL.
 *
 * @param contract
 * @param encoding
 * @return
 */
extern BREthereumContractFunction
contractLookupFunctionForEncoding (BREthereumContract contract, const char *encoding);

extern BREthereumContractEvent
contractLookupEventForTopic (BREthereumContract contract, const char *topic);


//
// Contract / Function
//
#include "../base/BREthereumLogic.h"
#include "../util/BRUtil.h"

private_extern UInt256
functionERC20TransferDecodeAmount (BREthereumContractFunction function,
                                   const char *data,
                                   BRCoreParseStatus *status);

private_extern char *
functionERC20TransferDecodeAddress (BREthereumContractFunction function,
                                    const char *data);

private_extern char *
eventERC20TransferDecodeAddress (BREthereumContractEvent event,
                                 const char *topic);

private_extern char *
eventERC20TransferEncodeAddress (BREthereumContractEvent event,
                                 const char *address);

private_extern UInt256
eventERC20TransferDecodeUInt256 (BREthereumContractEvent event,
                                 const char *number,
                                 BRCoreParseStatus *status);

#ifdef __cplusplus
}
#endif

#endif // BR_Ethereum_Contract_h
