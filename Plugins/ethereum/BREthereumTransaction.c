//
//  BBREthereumTransaction.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
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

#include <malloc.h>
#include "BREthereumTransaction.h"

struct BREthereumTransactionRecord {
    BREthereumAddress sourceAddress;
    BREthereumAddress targetAddress;
};

extern BREthereumTransaction
createEthereumTransaction (BREthereumAddress sourceAddress,
                           BREthereumAddress targetAddress) {
    BREthereumTransaction transaction = calloc (1, sizeof (struct BREthereumTransactionRecord));

    transaction->sourceAddress = sourceAddress;
    transaction->targetAddress = targetAddress;

    return transaction;
}

extern BREthereumAddress
getSourceAddress (BREthereumTransaction transaction) {
    return transaction->sourceAddress;
}

extern BREthereumAddress
getTargetAddress (BREthereumTransaction transaction) {
    return transaction->targetAddress;
}

/*
     https://github.com/ethereum/pyethereum/blob/develop/ethereum/transactions.py#L22
     https://github.com/ethereum/pyrlp/blob/develop/rlp/sedes/lists.py#L135

     A transaction is stored as:
    [nonce, gasprice, startgas, to, value, data, v, r, s]
    nonce is the number of transactions already sent by that account, encoded
    in binary form (eg.  0 -> '', 7 -> '\x07', 1000 -> '\x03\xd8').
    (v,r,s) is the raw Electrum-style signature of the transaction without the
    signature made with the private key corresponding to the sending account,
    with 0 <= v <= 3. From an Electrum-style signature (65 bytes) it is
    possible to extract the public key, and thereby the address, directly.
    A valid transaction is one where:
    (i) the signature is well-formed (ie. 0 <= v <= 3, 0 <= r < P, 0 <= s < N,
        0 <= r < P - N if v >= 2), and
    (ii) the sending account has enough funds to pay the fee and the value.
    """

    fields = [
        ('nonce', big_endian_int),
        ('gasprice', big_endian_int),
        ('startgas', big_endian_int),
        ('to', utils.address),
        ('value', big_endian_int),
        ('data', binary),
        ('v', big_endian_int),
        ('r', big_endian_int),
        ('s', big_endian_int),
    ]

 */


/*


 $ curl -X POST -H  "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_getTransactionByHash","params":["0x3104b0ee2aba4197f4da656d6144e5978c0b7bcb08890ed7bd6228bc9dbe745e"],"id":1}' http://localhost:8545
    {"jsonrpc":"2.0","id":1,
    "result":{"blockHash":"0xbf197f8ce876514b8922af10824efba5b4ce3fc7ab9ef97443ef9c56bd0cae32",
        "blockNumber":"0x1b930a",
        "from":"0x888197521cfe05ff89960c50012252008819b2cb",
        "gas":"0x1d8a8",
        "gasPrice":"0x4a817c800",
        "hash":"0x3104b0ee2aba4197f4da656d6144e5978c0b7bcb08890ed7bd6228bc9dbe745e",
        "input":"0x",
        "nonce":"0x0",
        "to":"0xf8e60edd24bc15f32bb4260ec2cea7c54cced121",
        "transactionIndex":"0x3",
        "value":"0xde0b6b3a7640000",
        "v":"0x2b",
        "r":"0xa571650cb08199d808b6646f634a8f7431cfd103a243654263faf2518e3efd40",
        "s":"0x4d2774147ccb90d1e7ad9358eb895c5f5d24db26b9d3e880bcee4fa06e5b3e1b"}}
 */

/*
 Signing


 https://bitcoin.stackexchange.com/questions/38351/ecdsa-v-r-s-what-is-v

 > msgSha = web3.sha3('Now it the time')
"0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971"

 > personal.unlockAccount(eth.accounts[<n>], "password", 3600)

 $ curl -X POST -H  "Content-Type: application/json" --data '{"jsonrpc":"2.0","method":"eth_sign","
    params":["0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", "0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971"],
    "id":1}'
    http://localhost:8545

 {"jsonrpc":"2.0","id":1,
    "result":"0xe79ba93e981e8ee50b8d07b0be7ae4526bc4d9bf7dcffe88dff62c502b2a126d7f772e2374869b41b0f5c0061d6d828348c96a7021f0c3227e73431d8ebbf1331b"}
 */

/*
 * r, s, v

  signature = signature.substr(2); //remove 0x
  const r = '0x' + signature.slice(0, 64)
  const s = '0x' + signature.slice(64, 128)
  const v = '0x' + signature.slice(128, 130)
  const v_decimal = web3.toDecimal(v)


 > web3.eth.sign ("0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", "0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971")
"0xe79ba93e981e8ee50b8d07b0be7ae4526bc4d9bf7dcffe88dff62c502b2a126d7f772e2374869b41b0f5c0061d6d828348c96a7021f0c3227e73431d8ebbf1331b"

 */

/*

 > msg = 'Try again'
 > msgSha = web3.sha3(msg)
 > sig = web3.eth.sign ("0xf8e60edd24bc15f32bb4260ec2cea7c54cced121", msgSha) // account, sha-ed msg
 > personal.ecRecover (msgSha, sig)
 "0xf8e60edd24bc15f32bb4260ec2cea7c54cced121"
 > eth.accounts[1]
 "0xf8e60edd24bc15f32bb4260ec2cea7c54cced121"




 */