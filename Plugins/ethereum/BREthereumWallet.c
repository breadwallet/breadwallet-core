//
//  BBREthereumWallet.c
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
#include "BREthereumWallet.h"

struct BREthereumWalletRecord {
    int nonce;
    // account
    // privateKey

};

extern BREthereumWallet
createEthereumWallet ()
{
    BREthereumWallet wallet = calloc (1, sizeof (struct BREthereumWalletRecord));
    // nonce = eth.getTransactionCount(<account>)
    return wallet;
}

// gasPrice is the maximum price of gas you are willing to pay for this transaction.
// gasLimit is the maximum gas you are willing to pay for this transaction.
//
extern BREthereumTransaction
createTransaction(BREthereumWallet wallet,
                  BREthereumAddress recvAddress,
                  BREthereumEther amount) {

    return createTransactionDetailed
            (wallet,
             recvAddress,
             amount,
             getDefaultGasPrice(wallet),
             getDefaultGasLimit(wallet),
             incrementNonce(wallet));
}

extern BREthereumTransaction
createTransactionDetailed(BREthereumWallet wallet,
                          BREthereumAddress recvAddress,
                          BREthereumEther amount,
                          int gasPrice,
                          int gasLimit,
                          int nonce) {
    return NULL;
}

extern void
signTransaction (BREthereumWallet wallet,
                 BREthereumTransaction transaction) {
    return;
}

extern char *
getRawTransaction (BREthereumWallet wallet,
                   BREthereumTransaction transaction) {
    return NULL;
}


extern int
getDefaultGasLimit (BREthereumWallet wallet) {
    return 0;
}

extern int
getDefaultGasPrice (BREthereumWallet wallet) {
    return 0;
}

extern int
getNonce (BREthereumWallet wallet) {
    return wallet->nonce;
}

extern int
incrementNonce (BREthereumWallet wallet) {
    return ++wallet->nonce;
}

/*
 * https://medium.com/blockchain-musings/how-to-create-raw-transactions-in-ethereum-part-1-1df91abdba7c
 *
 *

 // Private key
    const keythereum = require('keythereum');
    const address = '0x9e378d2365b7657ebb0f72ae402bc08812022211';
    const datadir = '/home/administrator/ethereum/data';
    const password = 'password';
    let   privKey; // a 'buffer'

    keythereum.importFromFile(address, datadir,
        function (keyObject) {
            keythereum.recover(password, keyObject,
                function (privateKey) {
                    console.log(privateKey.toString('hex'));
                    privKey = privateKey
                });
        });
    //05a20149c1c76ae9da8457435bf0224a4f81801da1d8204cb81608abe8c112ca

   const ethTx = require('ethereumjs-tx');

   const txParams = {
        nonce: '0x6', // Replace by nonce for your account on geth node
        gasPrice: '0x09184e72a000',
        gasLimit: '0x30000',
        to: '0xfa3caabc8eefec2b5e2895e5afbf79379e7268a7',
        value: '0x00'
    };

    // Transaction is created
    const tx = new ethTx(txParams);
    const privKey = Buffer.from('05a20149c1c76ae9da8457435bf0224a4f81801da1d8204cb81608abe8c112ca', 'hex');

    // Transaction is signed
    tx.sign(privKey);

    const serializedTx = tx.serialize();
    const rawTx = '0x' + serializedTx.toString('hex');
    console.log(rawTx)

    eth.sendRawTransaction(raxTX)


 */


/*
 *
 * https://ethereum.stackexchange.com/questions/16472/signing-a-raw-transaction-in-go

  signer := types.NewEIP155Signer(nil)
  tx := types.NewTransaction(nonce, to, amount, gas, gasPrice, data)
  signature, _ := crypto.Sign(tx.SigHash(signer).Bytes(), privkey)
  signed_tx, _ := tx.WithSignature(signer, signature)

 */

/*
 *

 web3.eth.accounts.create();
 > {
    address: "0xb8CE9ab6943e0eCED004cDe8e3bBed6568B2Fa01",
    privateKey: "0x348ce564d427a3311b6536bbcff9390d69395b06ed6c486954e971d960fe8709",
    signTransaction: function(tx){...},
    sign: function(data){...},
    encrypt: function(password){...}
 }

 */