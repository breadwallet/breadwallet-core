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
#include <assert.h>
#include <regex.h>
#include "BRCrypto.h"
#include "BRInt.h"
#include "BRKey.h"

#include "BREthereum.h"
#include "../BRBIP39WordsEn.h"
#include "BREthereumLightNode.h"

static void
showHex (uint8_t *source, size_t sourceLen) {
    char *prefix = "{";
    for (int i = 0; i < sourceLen; i++) {
        printf("%s%x", prefix, source[i]);
        prefix = ", ";
    }
    printf ("}\n");
}

/*
m/44'/60'/0'/0/0 :: 0x2161DedC3Be05B7Bb5aa16154BcbD254E9e9eb68
                    0x03c026c4b041059c84a187252682b6f80cbbe64eb81497111ab6914b050a8936fd
                    0x73bf21bf06769f98dabcfac16c2f74e852da823effed12794e56876ede02d45d
m/44'/60'/0'/0/1 :: 0x9595F373a4eAe74511561A52998cc6fB4F9C2bdD
*/

//
// RLP Test
//
#define RLP_S1 "dog"
#define RLP_S1_RES { 0x83, 'd', 'o', 'g' };

#define RLP_S2 ""
#define RLP_S2_RES { 0x80 }

#define RLP_S3 "Lorem ipsum dolor sit amet, consectetur adipisicing elit"
#define RLP_S3_RES { 0xb8, 0x38, 'L', 'o', 'r', 'e', 'm', ' ', 'i', 'p', 's', 'u', 'm', ' ', 'd', 'o', 'l', 'o', 'r', \
 ' ', 's', 'i', 't', ' ', 'a', 'm', 'e', 't', ',', ' ', 'c', 'o', 'n', 's', 'e', 'c', 't', 'e', 't', 'u', 'r', \
 ' ', 'a', 'd', 'i', 'p', 'i', 's', 'i', 'c', 'i', 'n', 'g', ' ', 'e', 'l', 'i', 't' };

#define RLP_V1 0
#define RLP_V1_RES { 0x00 }

#define RLP_V2 15
#define RLP_V2_RES { 0x0f }

#define RLP_V3 1024
#define RLP_V3_RES { 0x82, 0x04, 0x00 }

// 'cat', 'dog'
#define RLP_L1_RES { 0xc8, 0x83, 'c', 'a', 't', 0x83, 'd', 'o', 'g' }

int equalBytes (uint8_t *a, size_t aLen, uint8_t *b, size_t bLen) {
    if (aLen != bLen) return 0;
    for (int i = 0; i < aLen; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

void rlpCheck (BRRlpCoder coder, BRRlpItem item, uint8_t *result, size_t resultSize) {
  BRRlpData data;

  rlpGetData(coder, item, &data.bytes, &data.bytesCount);
  assert (equalBytes(data.bytes, data.bytesCount, result, resultSize));
  printf (" => "); showHex (data.bytes, data.bytesCount);

  free (data.bytes);
}

void rlpCheckString (BRRlpCoder coder, const char *string, uint8_t *result, size_t resultSize) {
    printf ("  \"%s\"", string);
    rlpCheck(coder, rlpEncodeItemString(coder, (char*) string), result, resultSize);
}

void rlpCheckInt (BRRlpCoder coder, uint64_t value, uint8_t *result, size_t resultSize) {
    printf ("  %llu", value);
    rlpCheck(coder, rlpEncodeItemUInt64(coder, value), result, resultSize);
}

void runRlpTest () {
    printf ("==== RLP\n");

  BRRlpCoder coder = rlpCoderCreate();

  uint8_t s1r[] = RLP_S1_RES;
  rlpCheckString(coder, RLP_S1, s1r, sizeof(s1r));

  uint8_t s2r[] = RLP_S2_RES;
  rlpCheckString(coder, RLP_S2, s2r, sizeof(s2r));

  uint8_t s3r[] = RLP_S3_RES;
  rlpCheckString(coder, RLP_S3, s3r, sizeof(s3r));

  uint8_t t3r[] = RLP_V1_RES;
  rlpCheckInt(coder, RLP_V1, t3r, sizeof(t3r));

  uint8_t t4r[] = RLP_V2_RES;
  rlpCheckInt(coder, RLP_V2, t4r, sizeof(t4r));

  uint8_t t5r[] = RLP_V3_RES;
  rlpCheckInt(coder, RLP_V3,t5r, sizeof(t5r));

  BRRlpItem listCatDog = rlpEncodeList2(coder,
                                        rlpEncodeItemString(coder, "cat"),
                                        rlpEncodeItemString(coder, "dog"));
  uint8_t resCatDog[] = RLP_L1_RES;
  printf ("  \"%s\"", "[\"cat\" \"dog\"]");
  rlpCheck(coder, listCatDog, resCatDog, 9);

  rlpCoderRelease(coder);
  printf ("\n");
}

//
// Account Test
//
#define TEST_PAPER_KEY    "army van defense carry jealous true garbage claim echo media make crunch"
#define TEST_ETH_ADDR_CHK "0x2161DedC3Be05B7Bb5aa16154BcbD254E9e9eb68"
#define TEST_ETH_ADDR     "0x2161dedc3be05b7bb5aa16154bcbd254e9e9eb68"
#define TEST_ETH_PUBKEY   "0x03c026c4b041059c84a187252682b6f80cbbe64eb81497111ab6914b050a8936fd"
#define TEST_ETH_PRIKEY   "0x73bf21bf06769f98dabcfac16c2f74e852da823effed12794e56876ede02d45d"

void runAddressTests (BREthereumAccount account) {
    BREthereumAddress address = accountGetPrimaryAddress(account);

    printf ("== Address\n");
    printf ("        String: %p\n", address);

    printf ("      PaperKey: %p, %s\n", TEST_PAPER_KEY, TEST_PAPER_KEY);
    printf ("\n");

    const char *publicKeyString = addressPublicKeyAsString (address);
    printf ("    Public Key: %p, %s\n", publicKeyString, publicKeyString);
    assert (0 == strcmp (TEST_ETH_PUBKEY, publicKeyString));

    const char *addressString = addressAsString (address);
    printf ("       Address: %s\n", addressString);
    assert (0 == strcmp (TEST_ETH_ADDR, addressString) ||
	      0 == strcmp (TEST_ETH_ADDR_CHK, addressString));

    free ((void *) addressString);
    free ((void *) publicKeyString);
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

#define TEST_TRANS1_RESULT "e9098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080"  // Leave off: signature { 01 80 80 }, replace 0xec with 0xe9

void runTransactionTests1 (BREthereumAccount account) {
    BREthereumWallet  wallet = walletCreate(account);

    BREthereumTransaction transaction = walletCreateTransactionDetailed(
            wallet,
            createAddress(TEST_TRANS1_TARGET_ADDRESS),
            holdingCreateEther(etherCreateNumber(TEST_TRANS1_ETHER_AMOUNT, TEST_TRANS1_ETHER_AMOUNT_UNIT)),
            gasPriceCreate(etherCreateNumber(TEST_TRANS1_GAS_PRICE_VALUE, TEST_TRANS1_GAS_PRICE_UNIT)),
            gasCreate(TEST_TRANS1_GAS_LIMIT),
            TEST_TRANS1_NONCE);

    BRRlpData dataUnsignedTransaction = transactionEncodeRLP(transaction, TRANSACTION_RLP_UNSIGNED);

    char result[2 * dataUnsignedTransaction.bytesCount + 1];
    encodeHex(result, 2 * dataUnsignedTransaction.bytesCount + 1, dataUnsignedTransaction.bytes, dataUnsignedTransaction.bytesCount);
    printf ("  Tx1 Raw (unsigned): %s\n", result);
    assert (0 == strcmp (result, TEST_TRANS1_RESULT));
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
#define TEST_TRANS2_RESULT_UNSIGNED   "e801847735940082520894873feb0644a6fbb9532bb31d1c03d4538aadec308806f05b59d3b2000080"
//                                     e801847735940082520894873feb0644a6fbb9532bb31d1c03d4538aadec308806f05b59d3b2000080


void runTransactionTests2 (BREthereumAccount account) {

    BREthereumWallet  wallet = walletCreate(account);

    BREthereumTransaction transaction = walletCreateTransactionDetailed(
            wallet,
            createAddress(TEST_TRANS2_TARGET_ADDRESS),
            holdingCreateEther(etherCreateNumber(TEST_TRANS2_ETHER_AMOUNT, TEST_TRANS2_ETHER_AMOUNT_UNIT)),
            gasPriceCreate(etherCreateNumber(TEST_TRANS2_GAS_PRICE_VALUE, TEST_TRANS2_GAS_PRICE_UNIT)),
            gasCreate(TEST_TRANS2_GAS_LIMIT),
            TEST_TRANS2_NONCE);

    BRRlpData dataUnsignedTransaction = transactionEncodeRLP(transaction, TRANSACTION_RLP_UNSIGNED);

    char result[2 * dataUnsignedTransaction.bytesCount + 1];
    encodeHex(result, 2 * dataUnsignedTransaction.bytesCount + 1, dataUnsignedTransaction.bytes, dataUnsignedTransaction.bytesCount);
    printf ("  Tx2 Raw (unsigned): %s\n", result);
    assert (0 == strcmp (result, TEST_TRANS2_RESULT_UNSIGNED));
}

void runTransactionTests (BREthereumAccount account) {
    printf ("\n== Transaction\n");

    runTransactionTests1 (account);
    runTransactionTests2 (account);
}

//
// Account Tests
//
void runAccountTests () {

    BREthereumAccount account = createAccount(TEST_PAPER_KEY);

    printf ("==== Account: %p\n", account);
    runAddressTests(account);
    runSignatureTests(account);
    runTransactionTests(account);

    accountFree (account);
    printf ("\n\n");
}

//
// Light Node Tests
//
#define NODE_PAPER_KEY "ginger settle marine tissue robot crane night number ramp coast roast critic"
#define NODE_NONCE              TEST_TRANS2_NONCE // 1
#define NODE_GAS_PRICE_VALUE    TEST_TRANS2_GAS_PRICE_VALUE // 20 GWEI
#define NODE_GAS_PRICE_UNIT     TEST_TRANS2_GAS_PRICE_UNIT // WEI
#define NODE_GAS_LIMIT          TEST_TRANS2_GAS_LIMIT
#define NODE_RECV_ADDR          TEST_TRANS2_TARGET_ADDRESS
#define NODE_ETHER_AMOUNT_UNIT  TEST_TRANS2_ETHER_AMOUNT_UNIT
#define NODE_ETHER_AMOUNT       TEST_TRANS2_ETHER_AMOUNT

//  Result      f86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 1b,a053ee5877032551f52da516c83620273312c8ab5a773d482dd60a772bb4a39938a07e187ee2335bfcfa3d20119e0e424d9ef5a81452dadee91ef2daf40081fdc454
//  Raw:      0xf86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 26,a030013044b571726723302bcf8dfad8765cf676db0844277a6f8cf63d04894008a069edd285604fdf010d96b8b7d9c547f9599b8ac51c50b8b97076bb9955c0bdde
#define NODE_RESULT "01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 1b,a0594c2fe40942a9dbd75b9cdd09397016592fc98ae24226f41706c5004c6608d0a072861c46ae62f4aae06eba04e5708b9421d2fcf21fa7f02aed1ff04accd405e3"

void runLightNodeTests () {
    printf ("==== Light Node\n");

    BREthereumLightNodeConfiguration configuration;

    BREthereumLightNode node = createLightNode(configuration);
    BREthereumLightNodeAccountId account = lightNodeCreateAccount(node, NODE_PAPER_KEY);

    printf ("  Node + Account\n");

    // A wallet holding Ether
    BREthereumLightNodeWalletId wallet = lightNodeCreateWallet(node, account);

    printf ("  Wallet\n");

  lightNodeSetWalletGasPrice(node, wallet,
                             TEST_TRANS2_GAS_PRICE_UNIT,
                             TEST_TRANS2_GAS_PRICE_VALUE);
  
    BREthereumLightNodeTransactionId tx1 =
            lightNodeWalletCreateTransaction
                    (node,
                     wallet,
                     NODE_RECV_ADDR,
                     NODE_ETHER_AMOUNT_UNIT,
                     NODE_ETHER_AMOUNT);

    printf ("  Transaction\n");

    lightNodeWalletSignTransaction (node, wallet, tx1, NODE_PAPER_KEY);

    printf ("    Signed\n");

    uint8_t *bytes;
    size_t   bytesCount;

    lightNodeFillTransactionRawData(node, wallet, tx1, &bytes, &bytesCount);

    printf ("    Filled Raw\n");

    // USE JSON_RPC to submit {bytes}
  char result[2 * bytesCount + 1];
  encodeHex(result, 2 * bytesCount + 1, bytes, bytesCount);
  printf ("        Bytes: %s\n", result);
}
//
// All Tests
//

extern void
runTests (void) {
    installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);
    runRlpTest();
    runAccountTests();
    runLightNodeTests();
}

int main(int argc, const char *argv[]) {
    runTests();
}


