//
//  BREthereumGas
//  Core Ethereum
//
//  Created by Ed Gamble on 2/24/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <assert.h>
#include "BREthereumGas.h"

//
// Gas
//
extern BREthereumGas
ethGasCreate(uint64_t amountOfGas) {
    BREthereumGas gas;
    gas.amountOfGas = amountOfGas;
    return gas;
}

extern BREthereumComparison
ethGasCompare (BREthereumGas e1, BREthereumGas e2) {
    return (e1.amountOfGas == e2.amountOfGas
            ? ETHEREUM_COMPARISON_EQ
            : (e1.amountOfGas > e2.amountOfGas
               ? ETHEREUM_COMPARISON_GT
               : ETHEREUM_COMPARISON_LT));
}

extern BRRlpItem
ethGasRlpEncode (BREthereumGas gas, BRRlpCoder coder) {
    return rlpEncodeUInt64(coder, gas.amountOfGas, 1);
}

extern BREthereumGas
ethGasRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    return ethGasCreate(rlpDecodeUInt64(coder, item, 1));
}

//
// Gas Price
//
extern BREthereumGasPrice
ethGasPriceCreate(BREthereumEther ether) {
    BREthereumGasPrice gasPrice;
    gasPrice.etherPerGas = ether;
    return gasPrice;
}

extern BREthereumComparison
ethGasPriceCompare (BREthereumGasPrice e1, BREthereumGasPrice e2) {
    return ethEtherCompare(e1.etherPerGas, e2.etherPerGas);
}

extern BREthereumEther
ethGasPriceGetGasCost(BREthereumGasPrice price, BREthereumGas gas, int *overflow) {
    assert (NULL != overflow);
    
    return ethEtherCreate (uint256Mul_Overflow (uint256Create(gas.amountOfGas), // gas
                                             price.etherPerGas.valueInWEI,   // WEI/gas
                                             overflow));
}

extern BRRlpItem
ethGasPriceRlpEncode (BREthereumGasPrice price, BRRlpCoder coder) {
    return ethEtherRlpEncode(price.etherPerGas, coder);
}

extern BREthereumGasPrice
ethGasPriceRlpDecode (BRRlpItem item, BRRlpCoder coder) {
    return ethGasPriceCreate(ethEtherRlpDecode(item, coder));
}
