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

/* bytes -> chars */
static void
encodeHex (char *target, size_t targetLen, uint8_t *source, size_t sourceLen) {
    assert (targetLen == 2 * sourceLen  + 1);

    int i = 0;
    for (; i < sourceLen && 2 * i < targetLen - 1; i++) {
        target[2*i] = (uint8_t) _hexc (source[i] >> 4);
        target[2*i + 1] = (uint8_t) _hexc (source[i]);
    }
    target[2*i] = '\0';
}

/* chars -> bytes */
static void
decodeHex (uint8_t *target, size_t targetLen, char *source, size_t sourceLen) {
    assert (targetLen == sourceLen / 2);

    for (int i = 0; i < targetLen; i++) {
//        printf ("%c ", source[2*i]);
//        printf ("%c ", source[(2*i)+1]);
        target[i] = (uint8_t) ((_hexu(source[2*i]) << 4) | _hexu(source[(2*i)+1]));
    }
}

static void
showHex (uint8_t *source, size_t sourceLen) {
    char *prefix = "{";
    for (int i = 0; i < sourceLen; i++) {
        printf("%s%x", prefix, source[i]);
        prefix = ", ";
    }
    printf ("}");
}

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
#define TEST_ETH_PUBKEY   "0x03c026c4b041059c84a187252682b6f80cbbe64eb81497111ab6914b050a8936fd"
#define TEST_ETH_PRIKEY   "0x73bf21bf06769f98dabcfac16c2f74e852da823effed12794e56876ede02d45d"

void runAddressTests (BREthereumAccount account) {
    BREthereumAddress address = accountGetPrimaryAddress(account);

    printf ("==== Address\n");
    printf ("       Address: %p\n", address);

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
    printf ("==== Signature\n");

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



}

//
// Account Tests
//
void runAccountTests () {
    BREthereumAccount account = accountCreate(TEST_PAPER_KEY);

    printf ("       Account: %p\n", account);
    runAddressTests(account);
    runSignatureTests(account);

    accountFree (account);
}

//
// All Tests
//

void runTests () {
    installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);
    runAccountTests();
}

int main(int argc, const char *argv[]) {
    runTests();
}


