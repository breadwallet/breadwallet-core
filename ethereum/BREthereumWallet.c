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
#include <assert.h>
#include "BREthereum.h"
#include "BREthereumAccount.h"

#define DEFAULT_ETHER_GAS_LIMIT    21000ull

#define DEFAULT_ETHER_GAS_PRICE_NUMBER   2ull
#define DEFAULT_ETHER_GAS_PRICE_UNIT     GWEI

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
    // BRSet (addresses)

    // gasPrice is the maximum price of gas you are willing to pay for this transaction.
    // gasLimit is the maximum gas you are willing to pay for this transaction.
    //


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
     * The wallet's holding, either ETHER or a TOKEN.
     */
    BREthereumHolding holding;

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
                      BREthereumWalletHoldingType type,
                      BREthereumToken optionalToken) {
    BREthereumWallet wallet = calloc (1, sizeof (struct BREthereumWalletRecord));

    assert (NULL != account);
    wallet->account = account;

    assert (NULL != address);
    wallet->address = address;

    wallet->holding = holdingCreate(type);
    wallet->token   = optionalToken;

    wallet->defaultGasLimit = walletCreateDefaultGasLimit (wallet);
    wallet->defaultGasPrice = walletCreateDefaultGasPrice (wallet);

    // nonce = eth.getTransactionCount(<account>)
    return wallet;
}

extern BREthereumWallet
walletCreate(BREthereumAccount account)
{
    return walletCreateWithAddress
            (account,
             accountCreateAddress(account));
}

extern BREthereumWallet
walletCreateWithAddress(BREthereumAccount account,
                        BREthereumAddress address) {
    return walletCreateDetailed
            (account,
            address,
            WALLET_HOLDING_ETHER,
            tokenCreateNone());
}

extern BREthereumWallet
walletCreateHoldingToken(BREthereumAccount account,
                         BREthereumAddress address,
                         BREthereumToken token) {
    return walletCreateDetailed
            (account,
            address,
            WALLET_HOLDING_TOKEN,
            token);
}

extern BREthereumTransaction
walletCreateTransaction(BREthereumWallet wallet,
                        BREthereumAddress recvAddress,
                        BREthereumEther amount) {

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
                                BREthereumEther amount,
                                BREthereumGasPrice gasPrice,
                                BREthereumGas gasLimit,
                                int nonce) {
    return transactionCreate(
            wallet->address,
            recvAddress,
            amount,
            gasPrice,
            gasLimit,
            nonce);
}


extern void
walletSignTransaction(BREthereumWallet wallet,
                      BREthereumTransaction transaction) {
    transactionSetSigner(transaction, wallet->account);

    // Maybe sign and cache; maybe defer until needed (lazy sign).
}

static char *
walletDataForHolding (BREthereumWallet wallet) {
    // TODO: Implement
    switch (wallet->holding.type) {
        case WALLET_HOLDING_ETHER:
            return "ether";

        case WALLET_HOLDING_TOKEN:
            return "token";
    }
}

extern BRRlpData // TODO: is this the actual result?
walletGetRawTransaction(BREthereumWallet wallet,
                        BREthereumTransaction transaction) {

    // TODO: This is properly done in 'transaction` (if `transaction` sees `account`-ish)

    // Fill in the transaction data appropriate for the holding (ETHER or TOKEN)
    transactionSetData(transaction, walletDataForHolding(wallet));

    // RLP Encode the UNSIGNED transaction
    BRRlpData transactionUnsignedRLP = transactionEncodeRLP
            (transaction, TRANSACTION_RLP_UNSIGNED);

    // Sign the RLP Encoded bytes.
    BREthereumSignature signature = accountSignBytes
            (wallet->account,
             wallet->address,
             SIGNATURE_TYPE_VRS,
             transactionUnsignedRLP.bytes,
             transactionUnsignedRLP.bytesCount);

    // Attach the signature
    transactionSetVRS(transaction,
                      signature.sig.bar.v,
                      signature.sig.bar.r,
                      signature.sig.bar.s);

    // RLP Encode the SIGNED transaction.
    return transactionEncodeRLP(transaction, TRANSACTION_RLP_SIGNED);
}
    /*

    signing_data = RLP.encode(self, sedes: UnsignedTransaction)
    rawhash = Utils.keccak256 signing_data
    key = PrivateKey.new(key).encode(:bin)

    vrs = Secp256k1.recoverable_sign rawhash, key
    self.v = encode_v(vrs[0])
    self.r = vrs[1]
    self.s = vrs[2]

    self.sender = PrivateKey.new(key).to_address

     // SEDES
     Transaction:
     fields = [
        ('nonce', big_endian_int),
        ('gasprice', big_endian_int),
        ('startgas', big_endian_int),
        ('to', utils.address),
        ('value', big_endian_int),
        ('data', binary),
        ('v', big_endian_int),
        ('r', big_endian_int),
        ('s', big_endian_int)]
     UnsignedTransaction:
        Transaction.exclude(['v', 'r', 's'])

     def sign(self, key, network_id=None):
        """Sign this transaction with a private key.
        A potentially already existing signature would be overridden.
        """
        if network_id is None:
            rawhash = utils.sha3(rlp.encode(self, UnsignedTransaction))
        else:
            assert 1 <= network_id < 2**63 - 18
            rlpdata = rlp.encode(rlp.infer_sedes(self).serialize(self)[
                                 :-3] + [network_id, b'', b''])
            rawhash = utils.sha3(rlpdata)

        key = normalize_key(key)

        self.v, self.r, self.s = ecsign(rawhash, key)
        if network_id is not None:
            self.v += 8 + network_id * 2

        self._sender = utils.privtoaddr(key)
        return self

     def ecsign(rawhash, key):
        if coincurve and hasattr(coincurve, 'PrivateKey'):
          pk = coincurve.PrivateKey(key)
         signature = pk.sign_recoverable(rawhash, hasher=None)
         v = safe_ord(signature[64]) + 27
          r = big_endian_to_int(signature[0:32])
          s = big_endian_to_int(signature[32:64])
        else:
          v, r, s = ecdsa_raw_sign(rawhash, key)
     return v, r, s

     def sha3(seed):
         return sha3_256(to_string(seed))

     def sha3_256(x):
        return keccak.new(digest_bits=256, data=x).digest()


     */

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
    switch (holdingGetType(wallet->holding)) {
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
    switch (holdingGetType(wallet->holding)) {
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