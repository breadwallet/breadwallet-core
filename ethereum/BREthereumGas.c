//
//  BREthereumGas
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/24/18.
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

#include "BREthereumGas.h"

//
// Gas
//
extern BREthereumGas
gasCreate(uint64_t amountOfGas) {
    BREthereumGas gas;
    gas.amountOfGas = amountOfGas;
    return gas;
}

extern void
gasRlpEncode (BREthereumGas gas, BRRlpCoder coder) {
    rlpEncodeItemUInt64(coder, gas.amountOfGas);
}

//
// Gas Price
//
extern BREthereumGasPrice
gasPriceCreate(BREthereumEther ether) {
    BREthereumGasPrice gasPrice;
    gasPrice.etherPerGas = ether;
    return gasPrice;
}

extern BREthereumEther
gasPriceGetGasCost(BREthereumGasPrice price, BREthereumGas gas) {
    BREthereumEther etherPerGas = price.etherPerGas;
    uint64_t amountOfGas = gas.amountOfGas;

    return etherCreate(UINT256_ZERO /* etherPerGas * amountOfGas */, WEI);
}

extern void
gasPriceRlpEncode (BREthereumGasPrice price, BRRlpCoder coder) {
    etherRlpEncode(price.etherPerGas, coder);
}
