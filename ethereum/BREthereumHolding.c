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


#include <string.h>
#include "BREthereumHolding.h"
#include "BREthereumEther.h"
#include "BREthereum.h"

//
// Holding
//

extern BREthereumHolding
createHolding(BREthereumWalletHoldingType type) {
    BREthereumHolding holding;

    holding.type = type;
    switch (type) {
        case WALLET_HOLDING_ETHER:
            holding.holding.ether.amount = etherCreateZero();
            break;
        case WALLET_HOLDING_TOKEN: {
            UInt256 scale = {.u64 = {0, 0, 0, 1}};
            holding.holding.token.scale = scale;
            holding.holding.token.amount = UINT256_ZERO;
            break;
        }
    }
    return holding;
}


extern BREthereumHolding
holdingCreateEther (BREthereumEther ether) {
    BREthereumHolding holding = createHolding(WALLET_HOLDING_ETHER);
    holding.holding.ether.amount = ether;
    return holding;
}

extern BREthereumHolding
holdingCreateToken (UInt256 scale, UInt256 amount) {
    BREthereumHolding holding = createHolding(WALLET_HOLDING_TOKEN);
    holding.holding.token.scale = scale;
    holding.holding.token.amount = amount;
    return holding;
}

extern BREthereumWalletHoldingType
holdingGetType (BREthereumHolding holding) {
    return holding.type;
}

extern BRRlpItem
holdingRlpEncode(BREthereumHolding holding, BRRlpCoder coder) {
    switch (holding.type) {
        case WALLET_HOLDING_ETHER:
            return etherRlpEncode(holding.holding.ether.amount, coder);

        case WALLET_HOLDING_TOKEN:
            return rlpEncodeItemUInt64(coder, 0);
    }
}

//
// Parse
//
extern BREthereumHolding
holdingCreateEtherParse (const char *number, BREthereumEtherUnit unit, BREthereumHoldingParseStatus *status) {
  BREthereumHolding holding;
  return holding;
}

extern BREthereumHolding
holdingCreateTokenParse (const char *number, BREthereumHoldingParseStatus *status) {
  BREthereumHolding holding;
  return holding;
}

//
// Token
//
extern BREthereumToken
tokenCreate(char *address,
            char *symbol,
            char *name,
            char *description,
            BREthereumGas gasLimit,
            BREthereumGasPrice gasPrice) {
    BREthereumToken token = tokenCreateNone();

    // TODO: Copy address or what (BREthereumToken is a value type....)
    token.address = address;
    token.symbol  = symbol;
    token.name    = name;
    token.description = description;
    token.gasLimit = gasLimit;
    token.gasPrice = gasPrice;

    return token;
}

extern BREthereumToken
tokenCreateNone (void) {
    BREthereumToken token;
    memset (&token, sizeof (BREthereumToken), 0);
    return token;
}

extern BREthereumGas
tokenGetGasLimit (BREthereumToken token) {
    return token.gasLimit;
}


extern BREthereumGasPrice
tokenGetGasPrice (BREthereumToken token) {
    return token.gasPrice;
}


BREthereumToken tokenBRD = {
  "0x5250776FAD5A73707d222950de7999d3675a2722",
  "BRD",
  "Bread Token",
  "The Bread Token ...",
  10,
  { 50000 },
  { { { .u64 = {0, 0, 0, 0}}}}
};

/*

 const Web3 = require("web3");
 const web3 = new Web3();
 web3.setProvider(new
 web3.providers.HttpProvider("https://ropsten.infura.io/XXXXXX"));
 var abi = [ {} ] // redacted on purpose
 const account1 = "0x9..."
 const account2 = "0x3..."
 var count = web3.eth.getTransactionCount(account1);
 var abiArray = abi;
 var contractAddress = "0x2...";
 var contract =  web3.eth.contract(abiArray).at(contractAddress);


 var data = contract.transfer.getData(account2, 10000, {from: account1});
 var gasPrice = web3.eth.gasPrice;
 var gasLimit = 90000;

 var rawTransaction = {
 "from": account1,
 "nonce": web3.toHex(count),
 "gasPrice": web3.toHex(gasPrice),
 "gasLimit": web3.toHex(gasLimit),
 "to": account2,
 "value": 0,
 "data": data,
 "chainId": 0x03
 };

 var privKey = new Buffer('XXXXXXXXXXXXXX', 'hex');
 var tx = new Tx(rawTransaction);

 tx.sign(privKey);
 var serializedTx = tx.serialize();

 web3.eth.sendRawTransaction('0x' + serializedTx.toString('hex'), function(err, hash) {
 if (!err)
 console.log(hash);
 else
 console.log(err);
 });

 */


/*
 // https://github.com/ethereum/wiki/wiki/Ethereum-Contract-ABI
 Given the contract:

 contract Foo {
 function bar(fixed[2] xy) {}
 function baz(uint32 x, bool y) returns (bool r) { r = x > 32 || y; }
 function sam(bytes name, bool z, uint[] data) {}
 }
 Thus for our Foo example if we wanted to call baz with the parameters 69 and true, we would pass 68 bytes total, which can be broken down into:

 0xcdcd77c0: the Method ID. This is derived as the first 4 bytes of the Keccak hash of the ASCII form of the signature baz(uint32,bool).
 0x0000000000000000000000000000000000000000000000000000000000000045: the first parameter, a uint32 value 69 padded to 32 bytes
 0x0000000000000000000000000000000000000000000000000000000000000001: the second parameter - boolean true, padded to 32 bytes
 In total:

 // commas added
 0xcdcd77c0,0000000000000000000000000000000000000000000000000000000000000045,0000000000000000000000000000000000000000000000000000000000000001
*/
