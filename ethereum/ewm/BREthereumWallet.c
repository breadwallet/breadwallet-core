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
#include "BRArray.h"
#include "BREthereumPrivate.h"
#include "BREthereumWallet.h"
#include "BREthereumTransfer.h"

#define DEFAULT_ETHER_GAS_PRICE_NUMBER   500000000 // 0.5 GWEI
#define DEFAULT_ETHER_GAS_PRICE_UNIT     WEI

#define DEFAULT_TRANSFER_CAPACITY     20

/* Forward Declarations */
static BREthereumGasPrice
walletCreateDefaultGasPrice (BREthereumWallet wallet);

static BREthereumGas
walletCreateDefaultGasLimit (BREthereumWallet wallet);

static void
walletInsertTransferSorted (BREthereumWallet wallet,
                               BREthereumTransfer transfer);

static int // -1 if not found
walletLookupTransferIndex (BREthereumWallet wallet,
                              BREthereumTransfer transfer);

/**
 *
 */
struct BREthereumWalletRecord {
    
    /**
     * The wallet's account.  The account is used to sign transfers.
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
     * for a transfer of this wallet's holding type.  This default value can be 'overridden'
     * when creating a specific transfer.
     *
     * The gasPrice determines how 'interested' a miner is in 'blocking' a transfer.  Thus,
     * the gasPrice determines how quickly your transfer will be added to the block chain.
     */
    BREthereumGasPrice defaultGasPrice;
    
    /**
     * The wallet's default gasLimit. gasLimit is the maximum gas you are willing to pay for
     * a transfer of this wallet's holding type.  This default value can be 'overridden'
     * when creating a specific transfer.
     *
     * The gasLimit prevents your transfer's computation from 'spinning out of control' and
     * thus consuming unexpectedly large amounts of Ether.
     */
    BREthereumGas defaultGasLimit;
    
    /**
     * The wallet's balance, either ETHER or a TOKEN.
     */
    BREthereumAmount balance;
    
    /**
     * An optional ERC20 token specification.  Will be NULL (and unused) for holding ETHER.
     */
    BREthereumToken token; // optional
    
    //
    // Transfers - these are sorted from oldest [index 0] to newest.  As transfers are added
    // we'll maintain the ordering using an 'insertion sort' - while starting at the end and
    // working backwards.
    //
    BREthereumTransfer *transfers;
};

//
// Wallet Creation
//
static BREthereumWallet
walletCreateDetailed (BREthereumAccount account,
                      BREthereumAddress address,
                      BREthereumNetwork network,
                      BREthereumAmountType type,
                      BREthereumToken optionalToken) {
    
    assert (NULL != account);
    //    assert (NULL != address);
    assert (AMOUNT_TOKEN != type || NULL != optionalToken);
    
    BREthereumWallet wallet = calloc(1, sizeof(struct BREthereumWalletRecord));
    
    wallet->account = account;
    wallet->address = address;
    wallet->network = network;
    
    wallet->token = optionalToken;
    wallet->balance = (AMOUNT_ETHER == type
                       ? amountCreateEther(etherCreate(UINT256_ZERO))
                       : amountCreateToken(createTokenQuantity (wallet->token, UINT256_ZERO)));
    
    wallet->defaultGasLimit = AMOUNT_ETHER == type
    ? walletCreateDefaultGasLimit(wallet)
    : tokenGetGasLimit (optionalToken);
    
    wallet->defaultGasPrice = AMOUNT_ETHER == type
    ? walletCreateDefaultGasPrice(wallet)
    : tokenGetGasPrice (optionalToken);
    
    array_new(wallet->transfers, DEFAULT_TRANSFER_CAPACITY);
    return wallet;
}

extern BREthereumWallet
walletCreate(BREthereumAccount account,
             BREthereumNetwork network)
{
    return walletCreateDetailed (account,
                                 accountGetPrimaryAddress(account),
                                 network,
                                 AMOUNT_ETHER,
                                 NULL);
}

extern BREthereumWallet
walletCreateHoldingToken(BREthereumAccount account,
                         BREthereumNetwork network,
                         BREthereumToken token) {
    return walletCreateDetailed (account,
                                 accountGetPrimaryAddress(account),
                                 network,
                                 AMOUNT_TOKEN,
                                 token);
}

extern void
walletRelease (BREthereumWallet wallet) {
    // TODO: Release 'transfers' - tell EWM?
    array_free(wallet->transfers);
    free (wallet);
}

//
// Transfer
//
extern BREthereumEther
walletEstimateTransferFee (BREthereumWallet wallet,
                              BREthereumAmount amount,
                              int *overflow) {
    return walletEstimateTransferFeeDetailed(wallet,
                                                amount,
                                                wallet->defaultGasPrice,
                                                amountGetGasEstimate(amount),
                                                overflow);
}

/**
 * Estimate the transfer fee (in Ether) for transferring amount.
 */
extern BREthereumEther
walletEstimateTransferFeeDetailed (BREthereumWallet wallet,
                                      BREthereumAmount amount,
                                      BREthereumGasPrice price,
                                      BREthereumGas gas,
                                      int *overflow) {
    return etherCreate(mulUInt256_Overflow(price.etherPerGas.valueInWEI,
                                           createUInt256(gas.amountOfGas),
                                           overflow));
}

//
// Transfer Creation
//
extern BREthereumTransfer
walletCreateTransfer(BREthereumWallet wallet,
                        BREthereumAddress recvAddress,
                        BREthereumAmount amount) {

    BREthereumFeeBasis feeBasis = {
        FEE_BASIS_GAS,
        { .gas = {
        wallet->defaultGasLimit,
        wallet->defaultGasPrice
        }}
    };

    BREthereumTransfer transfer = transferCreate (wallet->address, recvAddress, amount, feeBasis);
    walletHandleTransfer(wallet, transfer);
    return transfer;
}

#ifdef WALLET_CREATE_TRANSACTION_DIRECTLY
extern BREthereumTransfer
walletCreateTransferDetailed(BREthereumWallet wallet,
                                BREthereumAddress recvAddress,
                                BREthereumEther amount,
                                BREthereumGasPrice gasPrice,
                                BREthereumGas gasLimit,
                                const char *data,
                                uint64_t nonce) {
    assert (walletGetAmountType(wallet) == amountGetType(amount));
    assert (AMOUNT_ETHER == amountGetType(amount)
            || (wallet->token == tokenQuantityGetToken (amountGetTokenQuantity(amount))));
    
    BREthereumTransfer transfer = transferCreate(wallet->address,
                                                          recvAddress,
                                                          amount,
                                                          gasPrice,
                                                          gasLimit,
                                                          data,
                                                          nonce);
    walletHandleTransfer(wallet, transfer);
    return transfer;
}
#endif // WALLET_CREATE_TRANSACTION_DIRECTLY

private_extern void
walletHandleTransfer(BREthereumWallet wallet,
                        BREthereumTransfer transfer) {
    walletInsertTransferSorted(wallet, transfer);
}

private_extern void
walletUnhandleTransfer (BREthereumWallet wallet,
                           BREthereumTransfer transfer) {
    int index = walletLookupTransferIndex(wallet, transfer);
    assert (-1 != index);
    array_rm(wallet->transfers, index);
}

private_extern int
walletHasTransfer (BREthereumWallet wallet,
                      BREthereumTransfer transfer) {
    return -1 != walletLookupTransferIndex(wallet, transfer);
}
//
// Transfer Signing and Encoding
//

/**
 * Sign the transfer.
 *
 * @param wallet
 * @param transfer
 * @param paperKey
 */
extern void
walletSignTransfer(BREthereumWallet wallet,
                      BREthereumTransfer transfer,
                      const char *paperKey) {
    transferSign (transfer,
                  wallet->network,
                  wallet->account,
                  wallet->address,
                  paperKey);
}

// For now.
extern void
walletSignTransferWithPrivateKey(BREthereumWallet wallet,
                                 BREthereumTransfer transfer,
                                 BRKey privateKey) {
    transferSignWithKey(transfer,
                        wallet->network,
                        wallet->account,
                        wallet->address,
                        privateKey);
}

//
// Wallet 'Field' Accessors
//

extern BREthereumAddress
walletGetAddress(BREthereumWallet wallet) {
    return wallet->address;
}

extern BREthereumAmountType
walletGetAmountType (BREthereumWallet wallet) {
    return wallet->balance.type;
}

extern BREthereumToken
walletGetToken (BREthereumWallet wallet) {
    return wallet->token;
}

// Balance

extern BREthereumAmount
walletGetBalance (BREthereumWallet wallet) {
    return wallet->balance;
}

private_extern void
walletSetBalance (BREthereumWallet wallet,
                  BREthereumAmount balance) {
    wallet->balance = balance;
}

// Gas Limit

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
    return amountGetGasEstimate(wallet->balance);
}

// Gas Price

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
    switch (amountGetType(wallet->balance)) {
        case AMOUNT_ETHER:
            return gasPriceCreate(etherCreateNumber
                                  (DEFAULT_ETHER_GAS_PRICE_NUMBER,
                                   DEFAULT_ETHER_GAS_PRICE_UNIT));
        case AMOUNT_TOKEN:
            return tokenGetGasPrice (wallet->token);
    }
}

//
// Transfer 'Observation'
//

extern int
transferPredicateAny (void *ignore,
                         BREthereumTransfer transfer,
                         unsigned int index) {
    return 1;
}

extern int
transferPredicateStatus (BREthereumTransferStatusType type,
                            BREthereumTransfer transfer,
                            unsigned int index) {
    return transferHasStatusType(transfer, type);
}

extern void
walletWalkTransfers (BREthereumWallet wallet,
                        void *context,
                        BREthereumTransferPredicate predicate,
                        BREthereumTransferWalker walker) {
    for (int i = 0; i < array_count(wallet->transfers); i++)
        if (predicate (context, wallet->transfers[i], i))
            walker (context, wallet->transfers[i], i);
}

extern BREthereumTransfer
walletGetTransferByHash (BREthereumWallet wallet,
                            BREthereumHash hash) {
    for (int i = 0; i < array_count(wallet->transfers); i++) {
        BREthereumHash transferHash = transferGetHash(wallet->transfers[i]);
        if (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(hash, transferHash)))
            return wallet->transfers[i];
    }
    return NULL;
}

extern BREthereumTransfer
walletGetTransferByNonce(BREthereumWallet wallet,
                            BREthereumAddress sourceAddress,
                            uint64_t nonce) {
    for (int i = 0; i < array_count(wallet->transfers); i++)
        if (nonce == transferGetNonce (wallet->transfers[i])
            && ETHEREUM_BOOLEAN_IS_TRUE(addressEqual(sourceAddress, transferGetSourceAddress(wallet->transfers[i]))))
            return wallet->transfers [i];
    return NULL;
}

extern BREthereumTransfer
walletGetTransferByIndex(BREthereumWallet wallet,
                            uint64_t index) {
    return (index < array_count(wallet->transfers)
            ? wallet->transfers[index]
            : NULL);
}

static int // -1 if not found
walletLookupTransferIndex (BREthereumWallet wallet,
                              BREthereumTransfer transfer) {
    for (int i = 0; i < array_count(wallet->transfers); i++)
        if (transfer == wallet->transfers[i])
            return i;
    return -1;
}

static void
walletInsertTransferSorted (BREthereumWallet wallet,
                               BREthereumTransfer transfer) {
    size_t index = array_count(wallet->transfers);  // if empty (unsigned int) index == 0
    for (; index > 0; index--)
        // quit if transfer is not-less-than the next in wallet
        if (ETHEREUM_COMPARISON_LT != transferCompare(transfer, wallet->transfers[index - 1]))
            break;
    array_insert(wallet->transfers, index, transfer);
}

static void
walletUpdateTransferSorted (BREthereumWallet wallet,
                               BREthereumTransfer transfer) {
    // transfer might have moved - move it if needed - but for now, remove then insert.
    int index = walletLookupTransferIndex(wallet, transfer);
    assert (-1 != index);
    array_rm(wallet->transfers, index);
    walletInsertTransferSorted(wallet, transfer);
}

extern unsigned long
walletGetTransferCount (BREthereumWallet wallet) {
    return array_count(wallet->transfers);
}

//
// Transfer State
//
#if 0
private_extern void
walletTransferSubmitted (BREthereumWallet wallet,
                            BREthereumTransfer transfer,
                            const BREthereumHash hash) {
    transferSetStatus(transfer, transactionStatusCreate (TRANSFER_STATUS_SUBMITTED));
    // balance updated?
}

private_extern void
walletTransferIncluded(BREthereumWallet wallet,
                          BREthereumTransfer transfer,
                          BREthereumGas gasUsed,
                          BREthereumHash blockHash,
                          uint64_t blockNumber,
                          uint64_t blockTransferIndex) {
    transferSetStatus(transfer,
                         transactionStatusCreateIncluded(gasUsed,
                                                         blockHash,
                                                         blockNumber,
                                                         blockTransferIndex));
    walletUpdateTransferSorted(wallet, transfer);
}

private_extern void
walletTransferErrored (BREthereumWallet wallet,
                          BREthereumTransfer transfer,
                          const char *reason) {
    transferSetStatus(transfer,
                         transactionStatusCreateErrored(reason));
}
#endif // 0
/*
 * https://medium.com/blockchain-musings/how-to-create-raw-transfers-in-ethereum-part-1-1df91abdba7c
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
 
 // Transfer is created
 const tx = new ethTx(txParams);
 const privKey = Buffer.from('05a20149c1c76ae9da8457435bf0224a4f81801da1d8204cb81608abe8c112ca', 'hex');
 
 // Transfer is signed
 tx.sign(privKey);
 
 const serializedTx = tx.serialize();
 const rawTx = '0x' + serializedTx.toString('hex');
 console.log(rawTx)
 
 eth.sendRawTransfer(raxTX)
 
 
 */


/*
 *
 * https://ethereum.stackexchange.com/questions/16472/signing-a-raw-transfer-in-go
 
 signer := types.NewEIP155Signer(nil)
 tx := types.NewTransfer(nonce, to, amount, gas, gasPrice, data)
 signature, _ := crypto.Sign(tx.SigHash(signer).Bytes(), privkey)
 signed_tx, _ := tx.WithSignature(signer, signature)
 
 */

/*
 *
 
 web3.eth.accounts.create();
 > {
 address: "0xb8CE9ab6943e0eCED004cDe8e3bBed6568B2Fa01",
 privateKey: "0x348ce564d427a3311b6536bbcff9390d69395b06ed6c486954e971d960fe8709",
 walletSignTransfer: function(tx){...},
 sign: function(data){...},
 encrypt: function(password){...}
 }
 
 */
