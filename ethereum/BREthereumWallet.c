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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "BREthereum.h"

#define DEFAULT_ETHER_GAS_LIMIT    21000ull

#define DEFAULT_ETHER_GAS_PRICE_NUMBER   200000000ull  // 2 GWEI
#define DEFAULT_ETHER_GAS_PRICE_UNIT     WEI

/* Forward Declarations */
static BREthereumGasPrice
walletCreateDefaultGasPrice (BREthereumWallet wallet);

static BREthereumGas
walletCreateDefaultGasLimit (BREthereumWallet wallet);

/**
 *
 */
struct BREthereumWalletRecord {

    /**
     * The wallet's account.  The account is used to sign transactions.
     */
    BREthereumAccount account;

    /**
     * The wallet's primary address - perhaps the sole address.  Must be an address
     * from the wallet's account.
     */
    BREthereumAddress address;      // Primary Address

    /**
     * The wallet's network.
     */
    BREthereumNetwork network;

    /**
     * The wallet' default gasPrice. gasPrice is the maximum price of gas you are willing to pay
     * for a transaction of this wallet's holding type.  This default value can be 'overridden'
     * when creating a specific transaction.
     */
    BREthereumGasPrice defaultGasPrice;

    /**
     * The wallet's default gasLimit. gasLimit is the maximum gas you are willing to pay for t
     * a transaction of this wallet's holding type.  This default value can be 'overridden'
     * when creating a specific transaction.
     */
    BREthereumGas defaultGasLimit;

    /**
     * The wallet's balance, either ETHER or a TOKEN.
     */
    BREthereumHolding balance;

    /**
     * An optional ERC20 token specification.  Will be NULL (and unused) for holding ETHER.
     */
    BREthereumToken token; // optional

    /**
     * The number of transactions for this wallet.
     */
    int nonce;
};

static BREthereumWallet
walletCreateDetailed (BREthereumAccount account,
                      BREthereumAddress address,
                      BREthereumNetwork network,
                      BREthereumWalletHoldingType type,
                      BREthereumToken optionalToken) {

    assert (NULL != account);
    assert (NULL != address);

    BREthereumWallet wallet = calloc(1, sizeof(struct BREthereumWalletRecord));

    wallet->account = account;
    wallet->address = address;
    wallet->network = network;

    wallet->token = optionalToken;
    wallet->balance = (WALLET_HOLDING_ETHER == type
                       ? holdingCreateEther(etherCreate(UINT256_ZERO))
                       : holdingCreateToken(wallet->token, UINT256_ZERO));

    wallet->defaultGasLimit = WALLET_HOLDING_ETHER == type
                              ? walletCreateDefaultGasLimit(wallet)
                              : tokenGetGasLimit (optionalToken);

    wallet->defaultGasPrice = WALLET_HOLDING_ETHER == type
                              ? walletCreateDefaultGasPrice(wallet)
                              : tokenGetGasPrice (optionalToken);

    // nonce = eth.getTransactionCount(<account>)
    return wallet;
}

extern BREthereumWallet
walletCreate(BREthereumAccount account,
             BREthereumNetwork network)
{
    return walletCreateWithAddress
            (account,
             accountGetPrimaryAddress(account),
             network);
}

extern BREthereumWallet
walletCreateWithAddress(BREthereumAccount account,
                        BREthereumAddress address,
                        BREthereumNetwork network) {
    return walletCreateDetailed
            (account,
             address,
             network,
             WALLET_HOLDING_ETHER,
             tokenCreateNone());
}

extern BREthereumWallet
walletCreateHoldingToken(BREthereumAccount account,
                         BREthereumNetwork network,
                         BREthereumToken token) {
    return walletCreateDetailed
            (account,
             accountGetPrimaryAddress(account),
             network,
             WALLET_HOLDING_TOKEN,
             token);
}

extern BREthereumTransaction
walletCreateTransaction(BREthereumWallet wallet,
                        BREthereumAddress recvAddress,
                        BREthereumHolding amount) {

    return walletCreateTransactionDetailed
            (wallet,
             recvAddress,
             amount,
             walletGetDefaultGasPrice(wallet),
             walletGetDefaultGasLimit(wallet),
             walletIncrementNonce(wallet));
}

extern BREthereumTransaction
walletCreateTransactionDetailed(BREthereumWallet wallet,
                                BREthereumAddress recvAddress,
                                BREthereumHolding amount,
                                BREthereumGasPrice gasPrice,
                                BREthereumGas gasLimit,
                                int nonce) {
  assert (walletGetHoldingType(wallet) == holdingGetType(amount));
  assert (WALLET_HOLDING_ETHER == holdingGetType(amount)
          || 1 /*(wallet->token == tokenQuantityGetToken (holdingGetTokenQuantity(amount)))*/);

  // TODO: provide 'DataForHolding'
  //  Wallet needs contract+function - expects a 'transfer function'
  //  What are the arguments to walletDataForHolding... needs the transaction?  Needs more?
  //  Must 'rework' transaction (if token, amount = 0, ...)
  //
  // switch (wallet->holding.type) { ... }
    return transactionCreate(
            wallet->address,
            recvAddress,
            amount,
            gasPrice,
            gasLimit,
            nonce);
}

static char *
walletDataForHolding (BREthereumWallet wallet) {
    // TODO: Implement
    switch (wallet->balance.type) {
        case WALLET_HOLDING_ETHER:
            return "";  // empty string - official 'ETHER' data

        case WALLET_HOLDING_TOKEN:
//          contractEncode(contractERC20, functionERC20Transfer, ...)
            return "token";
    }
}

/**
 * Sign the transaction.
 *
 * @param wallet
 * @param transaction
 * @param paperKey
 */
extern void
walletSignTransaction(BREthereumWallet wallet,
                      BREthereumTransaction transaction,
                      const char *paperKey) {

    // TODO: Perhaps this is unneeded, if already provided.
    // Fill in the transaction data appropriate for the holding (ETHER or TOKEN)
    transactionSetData(transaction, walletDataForHolding(wallet));

    // RLP Encode the UNSIGNED transaction
    BRRlpData transactionUnsignedRLP = transactionEncodeRLP
            (transaction, wallet->network, TRANSACTION_RLP_UNSIGNED);

    // Sign the RLP Encoded bytes.
    BREthereumSignature signature = accountSignBytes
            (wallet->account,
             wallet->address,
             SIGNATURE_TYPE_RECOVERABLE,
             transactionUnsignedRLP.bytes,
             transactionUnsignedRLP.bytesCount,
             paperKey);

    // Attach the signature
    transactionSign(transaction, wallet->account, signature);
}

extern BRRlpData
walletGetRawTransaction(BREthereumWallet wallet,
                        BREthereumTransaction transaction) {
    return ETHEREUM_BOOLEAN_TRUE == transactionIsSigned(transaction)
           ? transactionEncodeRLP(transaction, wallet->network, TRANSACTION_RLP_SIGNED)
           : createRlpDataEmpty();
}

extern const char *
walletGetRawTransactionHexEncoded (BREthereumWallet wallet,
                                   BREthereumTransaction transaction,
                                   const char *prefix) {
  BRRlpData data = walletGetRawTransaction (wallet, transaction);
  char *result;

  if (NULL == prefix) prefix = "";

  if (0 == data.bytesCount)
    result = (char *) prefix;
  else {
    size_t resultLen = strlen(prefix) + 2 * data.bytesCount + 1;
    result = malloc (resultLen);
    strcpy (result, prefix);
    encodeHex(&result[strlen(prefix)], 2 * data.bytesCount + 1, data.bytes, data.bytesCount);
  }

  rlpDataRelease(data);
  return result;
}

extern BREthereumAddress
walletGetAddress (BREthereumWallet wallet) {
  return wallet->address;
}

extern BREthereumWalletHoldingType
walletGetHoldingType (BREthereumWallet wallet) {
  return wallet->balance.type;
}

extern BREthereumHolding
walletGetBalance (BREthereumWallet wallet) {
  return wallet->balance;
}

extern BREthereumGas
walletGetDefaultGasLimit(BREthereumWallet wallet) {
    return wallet->defaultGasLimit;
}

extern void
walletSetDefaultGasLimit(BREthereumWallet wallet, BREthereumGas gasLimit) {
    wallet->defaultGasLimit = gasLimit;
}

static BREthereumGas
walletCreateDefaultGasLimit (BREthereumWallet wallet) {
    switch (holdingGetType(wallet->balance)) {
        case WALLET_HOLDING_ETHER:
            return gasCreate (DEFAULT_ETHER_GAS_LIMIT);
        case WALLET_HOLDING_TOKEN:
            return tokenGetGasLimit (wallet->token);
    }
}


//
// Gas Price
//

extern BREthereumGasPrice
walletGetDefaultGasPrice(BREthereumWallet wallet) {
    return wallet->defaultGasPrice;
}

extern void
walletSetDefaultGasPrice(BREthereumWallet wallet, BREthereumGasPrice gasPrice) {
    wallet->defaultGasPrice = gasPrice;
}

static BREthereumGasPrice
walletCreateDefaultGasPrice (BREthereumWallet wallet) {
    switch (holdingGetType(wallet->balance)) {
        case WALLET_HOLDING_ETHER:
            return gasPriceCreate(
                    etherCreateNumber
                            (DEFAULT_ETHER_GAS_PRICE_NUMBER,
                             DEFAULT_ETHER_GAS_PRICE_UNIT));
        case WALLET_HOLDING_TOKEN:
            return tokenGetGasPrice (wallet->token);
    }
}

//
// Nonce
//

extern int
walletGetNonce(BREthereumWallet wallet) {
    return wallet->nonce;
}

extern int
walletIncrementNonce(BREthereumWallet wallet) {
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
    walletSignTransaction: function(tx){...},
    sign: function(data){...},
    encrypt: function(password){...}
 }

 */
