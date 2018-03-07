//
//  BREthereumHolding
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/25/18.
//  Copyright (c) 2018 breadwallet LLC
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.

#ifndef BR_Ethereum_Holding_H
#define BR_Ethereum_Holding_H

#include "BREthereumEther.h"
#include "BREthereumGas.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * An Ethereum Wallet holds either ETHER or TOKENs.
 */
typedef enum {
    WALLET_HOLDING_ETHER,
    WALLET_HOLDING_TOKEN
} BREthereumWalletHoldingType;

/**
 * An Ethereum Wallet holds a specific amount of ETHER or TOKENs
 */
typedef struct BREthereumHoldingRecord {
    BREthereumWalletHoldingType type;
    union {
        struct {
            BREthereumEther amount;
        } ether;
        struct {
            UInt256 scale;
            UInt256 amount;
        } token;
    } holding;
} BREthereumHolding;

extern BREthereumHolding
createHolding(BREthereumWalletHoldingType type);

extern BREthereumHolding
holdingCreateEther (BREthereumEther ether);

extern BREthereumHolding
holdingCreateToken (UInt256 scale, UInt256 amount);

extern BREthereumWalletHoldingType
holdingGetType (BREthereumHolding holding);

extern BRRlpItem
holdingRlpEncode(BREthereumHolding holding, BRRlpCoder coder);

/**
 * A Ethereum Token defines an ERC20 Token
 */
typedef struct BREthereumTokenRecord {
    /**
     * An Ethereum '0x' address for the token's contract.
     */
    char *address;

    /**
     * The (exchange) symbol - "BRD"
     */
    char *symbol;

    /**
     * The name - "Bread Token"
     */
    char *name;

    /**
     * The description - "The Bread Token ..."
     */
    char *description;

    /**
     * The (default) Gas Limit for exchanges of this token.
     */
    BREthereumGas gasLimit;

    /**
     * The (default) Gas Price for exchanges of this token.
     */
    BREthereumGasPrice gasPrice;
    
} BREthereumToken;

extern BREthereumToken
tokenCreate(char *address,
            char *symbol,
            char *name,
            char *description,
            BREthereumGas gasLimit,
            BREthereumGasPrice gasPrice);

extern BREthereumToken
tokenCreateNone (void);

extern BREthereumGas
tokenGetGasLimit (BREthereumToken token);

extern BREthereumGasPrice
tokenGetGasPrice (BREthereumToken token);

//extern BREthereumToken tokenBRD;

#ifdef __cplusplus
}
#endif

#endif //BR_Ethereum_Holding_H
