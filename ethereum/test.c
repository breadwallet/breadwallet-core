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
#include <BRCrypto.h>
#include <BRInt.h>

#include "BREthereum.h"
#include "../BRBIP39WordsEn.h"

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

int equalBytes (uint8_t *a, size_t aLen, uint8_t *b, size_t bLen) {
    if (aLen != bLen) return 0;
    for (int i = 0; i < aLen; i++)
        if (a[i] != b[i]) return 0;
    return 1;
}

void rlpCheck (BRRlpCoder coder, uint8_t *result, size_t resultSize) {
    BRRlpData data = rlpGetData(coder);
    assert (equalBytes(data.bytes, data.bytesCount, result, resultSize));
    printf (" => "); showHex (data.bytes, data.bytesCount);
    rlpCoderRelease(coder);
}

void rlpCheckString (const char *string, uint8_t *result, size_t resultSize) {
    BRRlpCoder coder = createRlpCoder();
    printf ("  \"%s\"", string);
    rlpEncodeItemString(coder, string);
    rlpCheck(coder, result, resultSize);
}

void rlpCheckInt (uint64_t value, uint8_t *result, size_t resultSize) {
    BRRlpCoder coder = createRlpCoder();
    printf ("  %d", value);
    rlpEncodeItemUInt64(coder, value);
    rlpCheck(coder, result, resultSize);
}

void runRlpTest () {
    printf ("==== RLP\n");

    uint8_t s1r[] = RLP_S1_RES;
    rlpCheckString(RLP_S1, s1r, sizeof(s1r));

    uint8_t s2r[] = RLP_S2_RES;
    rlpCheckString(RLP_S2, s2r, sizeof(s2r));

    uint8_t s3r[] = RLP_S3_RES;
    rlpCheckString(RLP_S3, s3r, sizeof(s3r));

    uint8_t t3r[] = RLP_V1_RES;
    rlpCheckInt(RLP_V1, t3r, sizeof(t3r));

    uint8_t t4r[] = RLP_V2_RES;
    rlpCheckInt(RLP_V2, t4r, sizeof(t4r));

    uint8_t t5r[] = RLP_V3_RES;
    rlpCheckInt(RLP_V3,t5r, sizeof(t5r));

    printf ("\n\n");
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

    printf ("\n== Address\n");
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


// https://github.com/ethereum/EIPs/issues/155
//
// Consider a transaction with nonce = 9, gasprice = 20 * 10**9, startgas = 21000,
// to = 0x3535353535353535353535353535353535353535, value = 10**18, data='' (empty).
//
// The "signing data" becomes:
//
// 0xec098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080018080
// The "signing hash" becomes:
//
// 0x2691916f9e6e5b304f135496c08f632040f02d78e36ae5bbbb38f919730c8fa0
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

#define SIGNATURE_SIGNING_DATA "0xec098504a817c800825208943535353535353535353535353535353535353535880de0b6b3a764000080018080"
#define SIGNATURE_SIGNING_HASH "0x2691916f9e6e5b304f135496c08f632040f02d78e36ae5bbbb38f919730c8fa0"
#define SIGNATURE_PRIVATE_KEY  "4646464646464646464646464646464646464646464646464646464646464646"

#define SIGNATURE_V "37"
#define SIGNATURE_R "11298168949998536842419725113857172427648002808790045841403298480749678639159"
#define SIGNATURE_S "26113561835810707062310182368620287328545641189938585203131842552044123671646"

#define SIGNING_DATA_2 "0xf86c258502540be40083035b609482e041e84074fc5f5947d4d27e3c44f824b7a1a187b1a2bc2ec500008078a04a7db627266fa9a4116e3f6b33f5d245db40983234eb356261f36808909d2848a0166fa098a2ce3bda87af6000ed0083e3bf7cc31c6686b670bd85cbc6da2d6e85"
#define SIGNING_HASH_2 "0x58e5a0fc7fbc849eddc100d44e86276168a8c7baaa5604e44ba6f5eb8ba1b7eb"

void runSignatureTests (BREthereumAccount account) {
    printf ("\n== Signature\n");

    size_t signingDataLen = strlen(SIGNATURE_SIGNING_DATA)/2;
    uint8_t signingData[signingDataLen];
    decodeHex(signingData, signingDataLen, SIGNATURE_SIGNING_DATA, sizeof(SIGNATURE_SIGNING_DATA));

    printf ("%s\n", SIGNATURE_SIGNING_DATA);
    showHex(signingData, signingDataLen);
    printf ("\n");

    char result[65];

    UInt256 digest;
    BRSHA256(&digest, signingData, signingDataLen);
    printf ("\nSHA 1: "); showHex(digest.u8, sizeof(digest));
    encodeHex(result, 65, digest.u8, sizeof(digest));
    printf ("\n     : 0x%s\n", result);

    BRSHA256_2(&digest, signingData, signingDataLen);
    printf ("\nSHA 2: "); showHex(digest.u8, sizeof(digest));
    encodeHex(result, 65, digest.u8, sizeof(digest));
    printf ("\n     : 0x%s\n", result);

    BRKeccak256(&digest, signingData, signingDataLen);
    printf ("\nKECCK: "); showHex(digest.u8, sizeof(digest));
    encodeHex(result, 65, digest.u8, sizeof(digest));
    printf ("\n     : 0x%s\n", result);

    printf ("\n HASH: %s", SIGNATURE_SIGNING_HASH);

    printf ("\n\n\n");

    // SIGNING_DATA -> HEX
    size_t signingData2Len = strlen(SIGNING_DATA_2)/2;
    uint8_t signingData2[signingData2Len];
    decodeHex(signingData2, signingData2Len, SIGNING_DATA_2, sizeof(SIGNING_DATA_2));
    // HASH and display
    BRKeccak256(&digest, signingData, signingDataLen);
    printf ("\nKECCK: "); showHex(digest.u8, sizeof(digest));
    encodeHex(result, 65, digest.u8, sizeof(digest));
    printf ("\n     : 0x%s\n", result);

    // Just has the freaking string
    BRKeccak256(&digest, SIGNING_DATA_2, sizeof(SIGNING_DATA_2));
    printf ("\nKECCK raw: "); showHex(digest.u8, sizeof(digest));
    encodeHex(result, 65, digest.u8, sizeof(digest));
    printf ("\n         : 0x%s", result);
    printf ("\n     HASH: %s", SIGNING_HASH_2);


//    > msgSha = web3.sha3('Now it the time')
//    "0x8b3942af68acfd875239181babe9ce093c420ca78d15b178fb63cf839dcf0971"


}

// Consider a transaction with nonce = 9, gasprice = 20 * 10**9, startgas = 21000,
// to = 0x3535353535353535353535353535353535353535, value = 10**18, data='' (empty).
//
//  The "signing data" becomes:
//     0xec 09 8504a817c800 825208 943535353535353535353535353535353535353535 880de0b6b3a7640000 80 01 80 80
//          09 8504a817c800 825208 943535353535353535353535353535353535353535 880de0b6b3a7640000 80

//  The "signing hash" becomes:
//     0x2691916f9e6e5b304f135496c08f632040f02d78e36ae5bbbb38f919730c8fa0

#define TEST_TRANS_NONCE 9
#define TEST_TRANS_GAS_PRICE_VALUE 20000000000 // 20 GWEI
#define TEST_TRANS_GAS_PRICE_UNIT  WEI
#define TEST_TRANS_GAS_LIMIT 21000
#define TEST_TRANS_TARGET_ADDRESS "0x3535353535353535353535353535353535353535"
#define TEST_TRANS_ETHER_AMOUNT 1000000000000000000u // 1 ETHER
#define TEST_TRANS_ETHER_AMOUNT_UNIT WEI
#define TEST_TRANS_DATA ""

void runTransactionTests (BREthereumAccount account) {
    printf ("\n== Transaction\n");

    BREthereumWallet  wallet = walletCreate(account);

    BREthereumTransaction transaction = walletCreateTransactionDetailed(
            wallet,
            createAddress(TEST_TRANS_TARGET_ADDRESS),
            holdingCreateEther(etherCreateNumber(TEST_TRANS_ETHER_AMOUNT, TEST_TRANS_ETHER_AMOUNT_UNIT)),
            gasPriceCreate(etherCreateNumber(TEST_TRANS_GAS_PRICE_VALUE, TEST_TRANS_GAS_PRICE_UNIT)),
            gasCreate(TEST_TRANS_GAS_LIMIT),
            TEST_TRANS_NONCE);

    BRRlpData dataUnsignedTransaction = transactionEncodeRLP(transaction, TRANSACTION_RLP_UNSIGNED);

    showHex(dataUnsignedTransaction.bytes, dataUnsignedTransaction.bytesCount);

    char result[2 * dataUnsignedTransaction.bytesCount + 1];
    encodeHex(result, 2 * dataUnsignedTransaction.bytesCount + 1, dataUnsignedTransaction.bytes, dataUnsignedTransaction.bytesCount);
    printf ("Hex: %s", result);

}

// https://etherscan.io/tx/0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c
// Hash: 0xc070b1e539e9a329b14c95ec960779359a65be193137779bf2860dc239248d7c
// From: 0x23c2a202c38331b91980a8a23d31f4ca3d0ecc2b
//   to: 0x873feb0644a6fbb9532bb31d1c03d4538aadec30
// Amnt: 0.5 Ether ($429.90)
// GasL: 21000
// Nonc: 1
// Data: 0x
//
//  Raw: 0xf86b 01 8477359400 825208 94,873feb0644a6fbb9532bb31d1c03d4538aadec30 8806f05b59d3b20000 80 26a030013044b571726723302bcf8dfad8765cf676db0844277a6f8cf63d04894008a069edd285604fdf010d96b8b7d9c547f9599b8ac51c50b8b97076bb9955c0bdde
//       List  Nonc  GasP      GasL          RecvAddr                               Amount         Data   <signature>

// Account Tests
//
void runAccountTests () {

    BREthereumAccount account = createAccount(TEST_PAPER_KEY);

    printf ("==== Account: %p\n", account);
    runTransactionTests(account);
    runAddressTests(account);
    runSignatureTests(account);

    accountFree (account);
    printf ("\n\n");
}

//
// All Tests
//

void runTests () {
    installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);
    runRlpTest();
    runAccountTests();
}

int main(int argc, const char *argv[]) {
    runTests();
}


