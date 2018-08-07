
//
//  test
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/27/18.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <pthread.h>

#include "BRCrypto.h"
#include "BRInt.h"
#include "BRArray.h"
#include "BRSet.h"
//#include "BRKey.h"
#include "BRBIP39WordsEn.h"
#include "BREthereum.h"
#include "BREthereumPrivate.h"
#include "BREthereumAccount.h"
#include "blockchain/BREthereumBlockChain.h"
#include "event/BREventAlarm.h"
#include "les/test-les.h"

// Util
extern void runUtilTests (void);

// RLP
extern void runRlpTests (void);

// Event
extern void runEventTests (void);

// Block Chain
extern void runBcTests (void);

// Contract
extern void runContractTests (void);

// LES
extern void runLEStests(void);

// EWM
extern void runEWMTests (void);
extern void runSyncTest (unsigned int durationInSeconds,
                         int restart);


//
// Ether & Token Parse
//
/*
 m/44'/60'/0'/0/0 :: 0x2161DedC3Be05B7Bb5aa16154BcbD254E9e9eb68
 0x03c026c4b041059c84a187252682b6f80cbbe64eb81497111ab6914b050a8936fd
 0x73bf21bf06769f98dabcfac16c2f74e852da823effed12794e56876ede02d45d
 m/44'/60'/0'/0/1 :: 0x9595F373a4eAe74511561A52998cc6fB4F9C2bdD
 */

//
// Account Test
//
#define TEST_PAPER_KEY    "army van defense carry jealous true garbage claim echo media make crunch"
#define TEST_ETH_ADDR_CHK "0x2161DedC3Be05B7Bb5aa16154BcbD254E9e9eb68"
#define TEST_ETH_ADDR     "0x2161dedc3be05b7bb5aa16154bcbd254e9e9eb68"
// This is a compressed public key; we generate uncompress public keys as { 04 x y }
#define TEST_ETH_PUBKEY   "0x03c026c4b041059c84a187252682b6f80cbbe64eb81497111ab6914b050a8936fd"
#define TEST_ETH_PRIKEY   "0x73bf21bf06769f98dabcfac16c2f74e852da823effed12794e56876ede02d45d"

void runAddressTests (BREthereumAccount account) {
    BREthereumAddress address = accountGetPrimaryAddress(account);
    
    printf ("== Address\n");
    printf ("        String: %p\n", &address);
    printf ("      PaperKey: %p, %s\n", TEST_PAPER_KEY, TEST_PAPER_KEY);

#if defined (DEBUG)
    const char *publicKeyString = accountGetPrimaryAddressPublicKeyString(account, 1);
    printf ("    Public Key: %p, %s\n", publicKeyString, publicKeyString);
    assert (0 == strcmp (TEST_ETH_PUBKEY, publicKeyString));
    free ((void *) publicKeyString);
#endif
    
    const char *addressString = addressGetEncodedString(address, 1);
    printf ("       Address: %s\n", addressString);
    assert (0 == strcmp (TEST_ETH_ADDR, addressString) ||
            0 == strcmp (TEST_ETH_ADDR_CHK, addressString));

    assert (0 == accountGetAddressNonce(account, address));
    assert (0 == accountGetThenIncrementAddressNonce(account, address));
    assert (1 == accountGetAddressNonce(account, address));
    accountSetAddressNonce(account, address, 0, ETHEREUM_BOOLEAN_FALSE);
    assert (1 == accountGetAddressNonce(account, address));
    accountSetAddressNonce(account, address, 0, ETHEREUM_BOOLEAN_TRUE);
    assert (0 == accountGetAddressNonce(account, address));
    accountSetAddressNonce(account, address, 2, ETHEREUM_BOOLEAN_FALSE);
    assert (2 == accountGetAddressNonce(account, address));

    free ((void *) addressString);
}

//
// Signature Test
//
// Test signing primitives: BRKeccak256, BRKeyCompactSign. NOTE: there are inconsistencies in the
// Ethereum EIP 155 sources on github.  It does not appear that that 'raw transaction' bytes are
// consistent with the {v, r, s} values 'appended' to the 'unsigned transaction rlp'.  The official
// location is https://github.com/ethereum/EIPs/blob/master/EIPS/eip-155.md - but I've used the
// following (and specifically the 'kvhnuke comment'):
//    https://github.com/ethereum/EIPs/issues/155 (SEE 'kvhnuke commented on Nov 22, 2016')
//
// Consider a transaction with nonce = 9, gasprice = 20 * 10**9, startgas = 21000,
// to = 0x3535353535353535353535353535353535353535, value = 10**18, data='' (empty).
//
// The "signing data" becomes:
//
// 0xec098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080018080
// The "signing hash" becomes:
//
// 0xdaf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53
//
// If the transaction is signed with the private key
// 0x4646464646464646464646464646464646464646464646464646464646464646, then the v,r,s values become:
//
// (37,
//  11298168949998536842419725113857172427648002808790045841403298480749678639159,
//  26113561835810707062310182368620287328545641189938585203131842552044123671646)
//
//Notice the use of 37 instead of 27. The signed tx would become:
//
// 0xf86c098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a76400008025a028ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276a067cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83
//

#define SIGNATURE_SIGNING_DATA "ec098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080018080"  // removed "0x"
#define SIGNATURE_SIGNING_HASH "daf5a779ae972f972197303d7b574746c7ef83eadac0f2791ad23db92e4c8e53" // removed "0x"
#define SIGNATURE_PRIVATE_KEY  "4646464646464646464646464646464646464646464646464646464646464646"

#define SIGNATURE_V "1b" // 37
#define SIGNATURE_R "28ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276" // remove "0x"
#define SIGNATURE_S "67cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83" // remove "0x"

#define SIGNING_DATA_2 "f86c258502540be40083035b609482e041e84074fc5f5947d4d27e3c44f824b7a1a187b1a2bc2ec500008078a04a7db627266fa9a4116e3f6b33f5d245db40983234eb356261f36808909d2848a0166fa098a2ce3bda87af6000ed0083e3bf7cc31c6686b670bd85cbc6da2d6e85"
#define SIGNING_HASH_2 "58e5a0fc7fbc849eddc100d44e86276168a8c7baaa5604e44ba6f5eb8ba1b7eb"

void runSignatureTests (BREthereumAccount account) {
    printf ("\n== Signature\n");
    UInt256 digest;
    
    printf ("    Data 1:\n");
    char *signingData = SIGNATURE_SIGNING_DATA;
    char *signingHash = SIGNATURE_SIGNING_HASH;
    
    size_t   signingBytesCount = 0;
    uint8_t *signingBytes = decodeHexCreate(&signingBytesCount, signingData, strlen (signingData));
    
    BRKeccak256(&digest, signingBytes, signingBytesCount);
    
    char *digestString = encodeHexCreate(NULL, (uint8_t *) &digest, sizeof(UInt256));
    printf ("      Hex: %s\n", digestString);
    assert (0 == strcmp (digestString, signingHash));
    
    BRKey privateKeyUncompressed;
    BRKeySetPrivKey(&privateKeyUncompressed, SIGNATURE_PRIVATE_KEY);
    
    size_t signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                           NULL, 0,
                                           digest);
    
    // Fill the signature
    uint8_t signatureBytes[signatureLen];
    signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                    signatureBytes, signatureLen,
                                    digest);
    assert (65 == signatureLen);
    
    char *signatureHex = encodeHexCreate(NULL, signatureBytes, signatureLen);
    printf ("      Sig: %s\n", signatureHex);
    assert (130 == strlen(signatureHex));
    assert (0 == strncmp (&signatureHex[ 0], SIGNATURE_V, 2));
    assert (0 == strncmp (&signatureHex[ 2], SIGNATURE_R, 64));
    assert (0 == strncmp (&signatureHex[66], SIGNATURE_S, 64));
    
    //
    printf ("    Data 2:");
    signingData = SIGNING_DATA_2;
    signingHash = SIGNING_HASH_2;
    signingBytesCount = 0;
    
    uint8_t *signingBytes2 = decodeHexCreate(&signingBytesCount, signingData, strlen (signingData));
    
    BRKeccak256(&digest, signingBytes2, signingBytesCount);
    
    char *digestString2 = encodeHexCreate(NULL, (uint8_t *) &digest, sizeof(UInt256));
    printf ("\n      Hex: %s\n", digestString2);
    assert (0 == strcmp (digestString2, signingHash));
}
//
// Transaction Tests
//
// Take some transactions from 'etherscan.io'; duplicate their content; ensure we process them
// correctly.  Check the


// Consider a transaction with nonce = 9, gasprice = 20 * 10**9, startgas = 21000,
// to = 0x3535353535353535353535353535353535353535, value = 10**18, data='' (empty).
//
//  The "signing data" becomes:
//     0xec 09 8504a817c800 825208 943535353535353535353535353535353535353535 880de0b6b3a7640000 80 01 80 80
//          09 8504a817c800 825208 943535353535353535353535353535353535353535 880de0b6b3a7640000 80
//          09 8504a817c800 825208 943535353535353535353535353535353535353535 880de0b6b3a7640000 80

//  The "signing hash" becomes:
//     0x2691916f9e6e5b304f135496c08f632040f02d78e36ae5bbbb38f919730c8fa0

#define TEST_TRANS1_NONCE 9
#define TEST_TRANS1_GAS_PRICE_VALUE 20000000000 // 20 GWEI
#define TEST_TRANS1_GAS_PRICE_UNIT  WEI
#define TEST_TRANS1_GAS_LIMIT 21000
#define TEST_TRANS1_TARGET_ADDRESS "0x3535353535353535353535353535353535353535"
#define TEST_TRANS1_ETHER_AMOUNT 1000000000000000000u // 1 ETHER
#define TEST_TRANS1_ETHER_AMOUNT_UNIT WEI
#define TEST_TRANS1_DATA ""

#define TEST_TRANS1_RESULT "ec098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080018080"

void runTransactionTests1 (BREthereumAccount account, BREthereumNetwork network) {
    printf ("     TEST 1\n");
    
    BREthereumWallet  wallet = walletCreate(account, network);
    walletSetDefaultGasLimit(wallet, gasCreate(TEST_TRANS1_GAS_LIMIT));
    walletSetDefaultGasPrice(wallet, gasPriceCreate(etherCreateNumber(TEST_TRANS1_GAS_PRICE_VALUE, TEST_TRANS1_GAS_PRICE_UNIT)));

    BREthereumTransfer transfer = walletCreateTransfer(wallet,
                                                       addressCreate(TEST_TRANS1_TARGET_ADDRESS),
                                                       amountCreateEther(etherCreateNumber(TEST_TRANS1_ETHER_AMOUNT, TEST_TRANS1_ETHER_AMOUNT_UNIT)));
    BREthereumTransaction transaction = transferGetOriginatingTransaction(transfer);
    transactionSetNonce(transaction, TEST_TRANS1_NONCE);

    assert (1 == networkGetChainId(network));
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = transactionRlpEncode(transaction, network, RLP_TYPE_TRANSACTION_UNSIGNED, coder);
    BRRlpData dataUnsignedTransaction;
    rlpDataExtract(coder, item, &dataUnsignedTransaction.bytes, &dataUnsignedTransaction.bytesCount);
    
    char result[2 * dataUnsignedTransaction.bytesCount + 1];
    encodeHex(result, 2 * dataUnsignedTransaction.bytesCount + 1, dataUnsignedTransaction.bytes, dataUnsignedTransaction.bytesCount);
    printf ("       Tx1 Raw (unsigned): %s\n", result);
    assert (0 == strcmp (result, TEST_TRANS1_RESULT));
    rlpDataRelease(dataUnsignedTransaction);

    // Check the gasLimit margin
    assert (21000ull == transactionGetGasLimit(transaction).amountOfGas);
    assert (0ull == transferGetGasEstimate(transfer).amountOfGas);

    // Will update gasLimt with margin
    // TODO: Redo
//    transferSetGasEstimate(transfer, gasCreate(21000ull));
//    assert (((100 + GAS_LIMIT_MARGIN_PERCENT) * 21000ull /100) == transactionGetGasLimit(transaction).amountOfGas);

    walletUnhandleTransfer(wallet, transfer);
    transferRelease(transfer);
    rlpReleaseItem(coder, item);
    rlpCoderRelease(coder);
}

// https://etherscan.io/tx/0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c
// Hash: 0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c
// From: 0x23c2a202c38331b91980a8a23d31f4ca3d0ecc2b
//   to: 0x873feb0644a6fbb9532bb31d1c03d4538aadec30
// Amnt: 0.5 Ether ($429.90)
// GasL: 21000
// GasP: 2 Gwei
// Nonc: 1
// Data: 0x
//
//  Raw: 0xf86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 26a030013044b571726723302bcf8dfad8765cf676db0844277a6f8cf63d04894008a069edd285604fdf010d96b8b7d9c547f9599b8ac51c50b8b97076bb9955c0bdde
//       List  Nonc  GasP      GasL          RecvAddr                               Amount         Data   <signature>
// Rslt:        01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80

#define TEST_TRANS2_NONCE 1
#define TEST_TRANS2_GAS_PRICE_VALUE 2000000000 // 20 GWEI
#define TEST_TRANS2_GAS_PRICE_UNIT  WEI
#define TEST_TRANS2_GAS_LIMIT 21000
#define TEST_TRANS2_TARGET_ADDRESS "0x873feb0644a6fbb9532bb31d1c03d4538aadec30"
#define TEST_TRANS2_ETHER_AMOUNT 500000000000000000u // 0.5 ETHER
#define TEST_TRANS2_ETHER_AMOUNT_UNIT WEI
#define TEST_TRANS2_DATA ""

#define TEST_TRANS2_RESULT_SIGNED   "f86b01847735940082520894873feb0644a6fbb9532bb31d1c03d4538aadec308806f05b59d3b200008026a030013044b571726723302bcf8dfad8765cf676db0844277a6f8cf63d04894008a069edd285604fdf010d96b8b7d9c547f9599b8ac51c50b8b97076bb9955c0bdde"
#define TEST_TRANS2_RESULT_UNSIGNED   "eb01847735940082520894873feb0644a6fbb9532bb31d1c03d4538aadec308806f05b59d3b2000080018080"

void runTransactionTests2 (BREthereumAccount account, BREthereumNetwork network) {
    printf ("     TEST 2\n");
    
    BREthereumWallet  wallet = walletCreate(account, network);
    walletSetDefaultGasLimit(wallet, gasCreate(TEST_TRANS2_GAS_LIMIT));
    walletSetDefaultGasPrice(wallet, gasPriceCreate(etherCreateNumber(TEST_TRANS2_GAS_PRICE_VALUE, TEST_TRANS2_GAS_PRICE_UNIT)));

    BREthereumTransfer transfer = walletCreateTransfer(wallet,
                                                       addressCreate(TEST_TRANS2_TARGET_ADDRESS),
                                                       amountCreateEther(etherCreateNumber(TEST_TRANS2_ETHER_AMOUNT,
                                                                                           TEST_TRANS2_ETHER_AMOUNT_UNIT)));
    BREthereumTransaction transaction = transferGetOriginatingTransaction(transfer);
    transactionSetNonce(transaction, TEST_TRANS2_NONCE);

    assert (1 == networkGetChainId(network));

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = transactionRlpEncode(transaction, network, RLP_TYPE_TRANSACTION_UNSIGNED, coder);
    BRRlpData data;
    rlpDataExtract(coder, item, &data.bytes, &data.bytesCount);
    
    char result[2 * data.bytesCount + 1];
    encodeHex(result, 2 * data.bytesCount + 1, data.bytes, data.bytesCount);
    printf ("       Tx2 Raw (unsigned): %s\n", result);
    assert (0 == strcmp (result, TEST_TRANS2_RESULT_UNSIGNED));
    rlpDataRelease(data);

    data.bytes = decodeHexCreate(&data.bytesCount, TEST_TRANS2_RESULT_SIGNED, strlen (TEST_TRANS2_RESULT_SIGNED));
    rlpShow(data, "Trans2Test");
    rlpDataRelease(data);

    walletUnhandleTransfer(wallet, transfer);
    transferRelease(transfer);
    rlpReleaseItem(coder, item);
    rlpCoderRelease(coder);
}

/*
 From: 0xd551234ae421e3bcba99a0da6d736074f22192ff
 To: Contract 0x558ec3152e2eb2174905cd19aea4e34a23de9ad6 (BreadToken)
 Recv: 0x932a27e1bc84f5b74c29af3d888926b1307f4a5c
 Amnt: 5,968.77
 
 Raw Hash:
 0xf8ad83067642850ba43b74008301246a94558ec3152e2eb2174905cd19aea4e34a23de9ad68
 // RLP Header - 'data'
 0b844
 a9059cbb
 000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c
 0000000000000000000000000000000000000000000001439152d319e84d0000
 // v, r, s signature
 25, a0a352fe7973fe554d3d5d21effb82667b3a17cc7b259eec482baf41a5ac80e155a0772ba32bfe32ccf7c4b764db155cd3e39b66c3b10abaa44ce27bc3013dd9ae7b
 
 Input Data:
 Function: transfer(address _to, uint256 _value) ***
 
 MethodID: 0xa9059cbb
 [0]:  000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c
 [1]:  0000000000000000000000000000000000000000000001439152d319e84d0000
 
 */
#define TEST_TRANS3_TARGET_ADDRESS "0x932a27e1bc84f5b74c29af3d888926b1307f4a5c"
#define TEST_TRANS3_GAS_PRICE_VALUE 50 // 20 GWEI
#define TEST_TRANS3_GAS_PRICE_UNIT  GWEI
#define TEST_TRANS3_GAS_LIMIT 74858
#define TEST_TRANS3_NONCE 423490
#define TEST_TRANS3_DECIMAL_AMOUNT "5968.77"

#define TEST_TRANS3_UNSIGNED_TX "f86d83067642850ba43b74008301246a94558ec3152e2eb2174905cd19aea4e34a23de9ad680b844a9059cbb000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c0000000000000000000000000000000000000000000001439152d319e84d0000018080"
// Add 018080 (v,r,s); adjust header count
// Answer: "0xf8ad 83067642 850ba43b7400 8301246a 94,558ec3152e2eb2174905cd19aea4e34a23de9ad6_80_b844a9059cbb000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c0000000000000000000000000000000000000000000001439152d319e84d0000"
//         "0xf86d 83067642 850ba43b7400 8301246a 94,0000000000000000000000000000000000000000_80_b844a9059cbb000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c0000000000000000000000000000000000000000000001439152d319e84d0000018080"    0x0000000143334f20
// Error :    f86d 83067642 850ba43b7400 8301246a 94,558ec3152e2eb2174905cd19aea4e34a23de9ad6_00_b844a9059cbb000000000000000000000000932a27e1bc84f5b74c29af3d888926b1307f4a5c0000000000000000000000000000000000000000000001439152d319e84d0000018080
//                                                                                            **  => amount = 0 => encoded as empty bytes, not numeric 0

void runTransactionTests3 (BREthereumAccount account, BREthereumNetwork network) {
    printf ("     TEST 3\n");
    
    BRCoreParseStatus status;
    BREthereumToken token = tokenGet(0);
    BREthereumWallet wallet = walletCreateHoldingToken (account, network, token);
    UInt256 value = createUInt256Parse ("5968770000000000000000", 10, &status);
    BREthereumAmount amount = amountCreateToken(createTokenQuantity (token, value));
    
    walletSetDefaultGasLimit(wallet, gasCreate(TEST_TRANS3_GAS_LIMIT));
    walletSetDefaultGasPrice(wallet, gasPriceCreate(etherCreateNumber(TEST_TRANS3_GAS_PRICE_VALUE, TEST_TRANS3_GAS_PRICE_UNIT)));

    BREthereumTransfer transfer = walletCreateTransfer(wallet,
                                                       addressCreate(TEST_TRANS3_TARGET_ADDRESS),
                                                       amount);
    BREthereumTransaction transaction = transferGetOriginatingTransaction(transfer);
    transactionSetNonce(transaction, TEST_TRANS3_NONCE);

    assert (1 == networkGetChainId(network));
    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = transactionRlpEncode(transaction, network, RLP_TYPE_TRANSACTION_UNSIGNED, coder);
    BRRlpData dataUnsignedTransaction = rlpGetData (coder, item);
    rlpDataExtract(coder, item, &dataUnsignedTransaction.bytes, &dataUnsignedTransaction.bytesCount);
    
    char *rawTx = encodeHexCreate(NULL, dataUnsignedTransaction.bytes, dataUnsignedTransaction.bytesCount);
    printf ("       Tx3 Raw (unsigned): %s\n", rawTx);
    assert (0 == strcasecmp(rawTx, TEST_TRANS3_UNSIGNED_TX));
    free (rawTx);
    rlpDataRelease(dataUnsignedTransaction);

    walletUnhandleTransfer(wallet, transfer);
    transferRelease(transfer);
    rlpReleaseItem(coder, item);
    rlpCoderRelease(coder);
}

#define TEST_TRANS4_SIGNED_TX "f8a90184773594008301676094dd974d5c2e2928dea5f71b9825b8b646686bd20080b844a9059cbb00000000000000000000000049f4c50d9bcc7afdbcf77e0d6e364c29d5a660df00000000000000000000000000000000000000000000000002c68af0bb14000025a09d4477bf97f638e1007d897bfd29a2053e2187a6d92c0e186ec98d81d291bf87a07f8c9e24255970b6282d3a21aa146add70b65f74a463eac54b2b11015bc37fbe"
#define TEST_TRANS4_HASH "0xe5a045bdd432a8edc345ff830641d1b75847ab5c9d8380241323fa4c9e6cee1e"

void runTransactionTests4 (BREthereumAccount account, BREthereumNetwork network) {
    printf ("     TEST 4\n");

    BRRlpData data;
    data.bytes = decodeHexCreate(&data.bytesCount, TEST_TRANS4_SIGNED_TX, strlen (TEST_TRANS4_SIGNED_TX));

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = rlpGetItem(coder, data);
    BREthereumTransaction tx = transactionRlpDecode(item, network, RLP_TYPE_TRANSACTION_SIGNED, coder);

    assert (ETHEREUM_BOOLEAN_IS_TRUE (hashEqual(transactionGetHash(tx), hashCreate(TEST_TRANS4_HASH))));
    rlpDataRelease(data);
    transactionRelease(tx);
    rlpReleaseItem(coder, item);
    rlpCoderRelease(coder);
}

void runTransactionTests (BREthereumAccount account, BREthereumNetwork network) {
    printf ("\n== Transaction\n");
    
    runTransactionTests1 (account, network);
    runTransactionTests2 (account, network);
    runTransactionTests3 (account, network);
    runTransactionTests4 (account, network);
}

//
// Account Tests
//
void runAccountTests () {
    
    BREthereumAccount account = createAccount(TEST_PAPER_KEY);
    BREthereumNetwork network = ethereumMainnet;
    
    printf ("==== Account: %p\n", account);
    runAddressTests(account);
    runSignatureTests(account);
    runTransactionTests(account, network);
    
    accountFree (account);
    printf ("\n\n");
}

//
// EWM Tests
//
#define NODE_PAPER_KEY "ginger settle marine tissue robot crane night number ramp coast roast critic"
#define NODE_NONCE              TEST_TRANS2_NONCE // 1
#define NODE_GAS_PRICE_VALUE    TEST_TRANS2_GAS_PRICE_VALUE // 20 GWEI
#define NODE_GAS_PRICE_UNIT     TEST_TRANS2_GAS_PRICE_UNIT // WEI
#define NODE_GAS_LIMIT          TEST_TRANS2_GAS_LIMIT
#define NODE_RECV_ADDR          TEST_TRANS2_TARGET_ADDRESS
#define NODE_ETHER_AMOUNT_UNIT  TEST_TRANS2_ETHER_AMOUNT_UNIT
#define NODE_ETHER_AMOUNT       TEST_TRANS2_ETHER_AMOUNT

#define GAS_PRICE_20_GWEI       2000000000
#define GAS_PRICE_10_GWEI       1000000000
#define GAS_PRICE_5_GWEI         500000000

#define GAS_LIMIT_DEFAULT 21000

//  Result      f86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 1b,a053ee5877032551f52da516c83620273312c8ab5a773d482dd60a772bb4a39938a07e187ee2335bfcfa3d20119e0e424d9ef5a81452dadee91ef2daf40081fdc454
//  Raw:      0xf86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 26,a030013044b571726723302bcf8dfad8765cf676db0844277a6f8cf63d04894008a069edd285604fdf010d96b8b7d9c547f9599b8ac51c50b8b97076bb9955c0bdde
#define NODE_RESULT "01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 1b,a0594c2fe40942a9dbd75b9cdd09397016592fc98ae24226f41706c5004c6608d0a072861c46ae62f4aae06eba04e5708b9421d2fcf21fa7f02aed1ff04accd405e3"

void testTransactionCodingEther () {
    printf ("     Coding Transaction\n");

    BREthereumAccount account = createAccount (NODE_PAPER_KEY);
    BREthereumWallet wallet = walletCreate(account, ethereumMainnet);

    BREthereumAddress txRecvAddr = addressCreate(NODE_RECV_ADDR);
    BREthereumAmount txAmount = amountCreateEther(etherCreate(createUInt256(NODE_ETHER_AMOUNT)));
    BREthereumGasPrice txGasPrice = gasPriceCreate(etherCreate(createUInt256(NODE_GAS_PRICE_VALUE)));
    BREthereumGas txGas = gasCreate(NODE_GAS_LIMIT);

    walletSetDefaultGasPrice(wallet, txGasPrice);
    walletSetDefaultGasLimit(wallet, txGas);
    BREthereumTransfer transfer = walletCreateTransfer(wallet, txRecvAddr, txAmount);
    BREthereumTransaction transaction = transferGetOriginatingTransaction(transfer);
    transactionSetNonce(transaction, NODE_NONCE);

    walletSignTransfer(wallet, transfer, NODE_PAPER_KEY);

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = transactionRlpEncode(transaction,
                                          ethereumMainnet,
                                          RLP_TYPE_TRANSACTION_SIGNED,
                                          coder);
    BRRlpData data = rlpGetData (coder, item);
    char *rawTx = encodeHexCreate(NULL, data.bytes, data.bytesCount);
    printf ("        Raw Transaction: 0x%s\n", rawTx);

    BREthereumTransaction decodedTransaction = transactionRlpDecode(item, ethereumMainnet, RLP_TYPE_TRANSACTION_SIGNED, coder);
    rlpReleaseItem(coder, item);

    assert (transactionGetNonce(transaction) == transactionGetNonce(decodedTransaction));
    assert (ETHEREUM_COMPARISON_EQ == gasPriceCompare(transactionGetGasPrice(transaction),
                                                      transactionGetGasPrice(decodedTransaction)));
    assert (ETHEREUM_COMPARISON_EQ == gasCompare(transactionGetGasLimit(transaction),
                                                 transactionGetGasLimit(decodedTransaction)));
    assert (ETHEREUM_COMPARISON_EQ == etherCompare(transactionGetAmount(transaction),
                                                   transactionGetAmount(decodedTransaction)));

    assert (ETHEREUM_BOOLEAN_TRUE == addressEqual(transactionGetTargetAddress(transaction),
                                                  transactionGetTargetAddress(decodedTransaction)));

    // Signature
    assert (ETHEREUM_BOOLEAN_TRUE == signatureEqual(transactionGetSignature (transaction),
                                                    transactionGetSignature (decodedTransaction)));

    // Address recovery
    BREthereumAddress transactionSourceAddress = transactionGetSourceAddress(transaction);
    BREthereumAddress decodedTransactionSourceAddress = transactionGetSourceAddress(decodedTransaction);
    assert (ETHEREUM_BOOLEAN_IS_TRUE(addressEqual(transactionSourceAddress, decodedTransactionSourceAddress)));

    assert (ETHEREUM_BOOLEAN_IS_TRUE(accountHasAddress(account, transactionSourceAddress)));

    // Archive
    BREthereumHash someBlockHash = HASH_INIT("fc45a8c5ebb5f920931e3d5f48992f3a89b544b4e21dc2c11c5bf8165a7245d6");
    BREthereumTransactionStatus status = transactionStatusCreateIncluded(gasCreate(0),
                                                                         someBlockHash,
                                                                         11592,
                                                                         21);
    transactionSetStatus(transaction, status);
    item = transactionRlpEncode(transaction, ethereumMainnet, RLP_TYPE_ARCHIVE, coder);
    BREthereumTransaction archivedTransaction = transactionRlpDecode(item, ethereumMainnet, RLP_TYPE_ARCHIVE, coder);
    rlpReleaseItem(coder, item);
    BREthereumTransactionStatus archivedStatus = transactionGetStatus(archivedTransaction);
    assert (ETHEREUM_BOOLEAN_IS_TRUE(transactionStatusEqual(status, archivedStatus)));
    assert (ETHEREUM_BOOLEAN_IS_TRUE(addressEqual(transactionGetTargetAddress(transaction),
                                                  transactionGetTargetAddress(archivedTransaction))));
    assert (ETHEREUM_BOOLEAN_IS_TRUE(hashEqual(status.u.included.blockHash, someBlockHash)));

    walletUnhandleTransfer(wallet, transfer);
    transferRelease(transfer);
    transactionRelease(transaction);
    transactionRelease(decodedTransaction);
    rlpCoderRelease(coder);
}

void testTransactionCodingToken () {
    printf ("     Coding Transaction\n");

    BREthereumToken token = tokenGet(0);
    BREthereumAccount account = createAccount (NODE_PAPER_KEY);
    BREthereumWallet wallet = walletCreateHoldingToken(account, ethereumMainnet, token);

    BREthereumAddress txRecvAddr = addressCreate(NODE_RECV_ADDR);
    BREthereumAmount txAmount = amountCreateToken(createTokenQuantity(token, createUInt256(NODE_ETHER_AMOUNT)));
    BREthereumGasPrice txGasPrice = gasPriceCreate(etherCreate(createUInt256(NODE_GAS_PRICE_VALUE)));
    BREthereumGas txGas = gasCreate(NODE_GAS_LIMIT);

    walletSetDefaultGasPrice(wallet, txGasPrice);
    walletSetDefaultGasLimit(wallet, txGas);
    BREthereumTransfer transfer = walletCreateTransfer(wallet, txRecvAddr, txAmount);
    BREthereumTransaction transaction = transferGetOriginatingTransaction(transfer);
    transactionSetNonce(transaction, NODE_NONCE);


    walletSignTransfer(wallet, transfer, NODE_PAPER_KEY);

    BRRlpCoder coder = rlpCoderCreate();
    BRRlpItem item = transactionRlpEncode(transaction,
                                          ethereumMainnet,
                                          RLP_TYPE_TRANSACTION_SIGNED,
                                          coder);
    BRRlpData data = rlpGetData (coder, item);
    char *rawTx = encodeHexCreate(NULL, data.bytes, data.bytesCount);
    printf ("        Raw Transaction: 0x%s\n", rawTx);

    BREthereumTransaction decodedTransaction = transactionRlpDecode(item, ethereumMainnet, RLP_TYPE_TRANSACTION_SIGNED, coder);
    rlpReleaseItem(coder, item);
    
    assert (transactionGetNonce(transaction) == transactionGetNonce(decodedTransaction));
    assert (ETHEREUM_COMPARISON_EQ == gasPriceCompare(transactionGetGasPrice(transaction),
                                                      transactionGetGasPrice(decodedTransaction)));
    assert (ETHEREUM_COMPARISON_EQ == gasCompare(transactionGetGasLimit(transaction),
                                                 transactionGetGasLimit(decodedTransaction)));
//    int typeMismatch = 0;
#if defined (TRANSACTION_ENCODE_TOKEN)
    assert (ETHEREUM_COMPARISON_EQ == amountCompare(transactionGetAmount(transaction),
                                                    transactionGetAmount(decodedTransaction),
                                                    &typeMismatch));
    assert (ETHEREUM_BOOLEAN_TRUE == addressEqual(transactionGetTargetAddress(transaction),
                                                     transactionGetTargetAddress(decodedTransaction)));
#endif
    // Signature
    assert (ETHEREUM_BOOLEAN_TRUE == signatureEqual(transactionGetSignature (transaction),
                                                    transactionGetSignature (decodedTransaction)));

    walletUnhandleTransfer(wallet, transfer);
    transferRelease(transfer);
    transactionRelease(transaction);
    transactionRelease(decodedTransaction);
    rlpCoderRelease(coder);
}



//
// All Tests
//

extern void
runTests (int reallySend) {
    installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);
    // Initialize tokens
//    tokenGet(0);
    runAccountTests();
    testTransactionCodingEther ();
    testTransactionCodingToken ();

//*    if (reallySend) testReallySend();
    printf ("Done\n");
}

#if defined (TEST_ETHEREUM_NEED_MAIN)
int main(int argc, const char *argv[]) {
    runUtilTests();
    runRlpTests();
    runEventTests();
    runBcTests();
    runContractTests();
    runEWMTests();
    runTests(0);
}
#endif

