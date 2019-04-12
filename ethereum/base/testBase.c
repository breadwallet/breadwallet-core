//
//  testBase.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//

#include <stdio.h>
#include <assert.h>
#include "support/BRCrypto.h"
#include "BREthereumBase.h"

static void
runEtherParseTests () {
    BRCoreParseStatus status;
    BREthereumEther e;

    e = etherCreateString("1", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));

    e = etherCreateString("100", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(100, WEI)));

    e = etherCreateString("100.0000", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(100, WEI)));

    e = etherCreateString("0.001", WEI+1, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));

    e = etherCreateString("0.00100", WEI+1, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1, WEI)));

    e = etherCreateString("0.001002", ETHER, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(1002, ETHER-2)));

    e = etherCreateString("12.03", ETHER, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ(e, etherCreateNumber(12030, ETHER-1)));

    e = etherCreateString("12.03", WEI, &status);
    //  assert (ETHEREUM_ETHER_PARSE_UNDERFLOW == status);
    assert (CORE_PARSE_OK != status);

    e = etherCreateString("100000000000000000000000000000000000000000000000000000000000000000000000000000000", WEI, &status);
    //  assert (ETHEREUM_ETHER_PARSE_OVERFLOW == status);
    assert (CORE_PARSE_OK != status);

    e = etherCreateString("1000000000000000000000", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ (e, etherCreateNumber(1, KETHER)));

    e = etherCreateString("2000000000000000000000.000000", WEI, &status);
    assert (CORE_PARSE_OK == status
            && ETHEREUM_BOOLEAN_TRUE == etherIsEQ (e, etherCreateNumber(2, KETHER)));

    char *s;
    e = etherCreateString("123", WEI, &status);
    s = etherGetValueString(e, WEI);
    assert (0 == strcmp (s, "123"));
    free (s);

    e = etherCreateString("1234", WEI, &status);
    s = etherGetValueString(e, WEI+1);
    assert (0 == strcmp (s, "1.234"));
    free (s);

    e = etherCreateString("123", WEI, &status);
    s = etherGetValueString(e, WEI+2);
    assert (0 == strcmp (s, "0.000123"));
    free (s);
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
#define SIGNATURE_ADDRESS      "9D8A62F656A8D1615C1294FD71E9CFB3E4855A4F"

#define SIGNATURE_V "1b" // 37
#define SIGNATURE_R "28ef61340bd939bc2195fe537567866003e1a15d3c71ff63e1590620aa636276" // remove "0x"
#define SIGNATURE_S "67cbe9d8997f761aecb703304b3800ccf555c9f3dc64214b297fb1966a3b6d83" // remove "0x"

#define SIGNING_DATA_2 "f86c258502540be40083035b609482e041e84074fc5f5947d4d27e3c44f824b7a1a187b1a2bc2ec500008078a04a7db627266fa9a4116e3f6b33f5d245db40983234eb356261f36808909d2848a0166fa098a2ce3bda87af6000ed0083e3bf7cc31c6686b670bd85cbc6da2d6e85"
#define SIGNING_HASH_2 "58e5a0fc7fbc849eddc100d44e86276168a8c7baaa5604e44ba6f5eb8ba1b7eb"

static void runSignatureTests1 (void) {
    printf ("\n== Signature 1\n");
    UInt256 digest;

    printf ("    Data:\n");
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

    // RAW (using directly BRKeyCompactSign)
    uint8_t signatureBytes[signatureLen];
    signatureLen = BRKeyCompactSign(&privateKeyUncompressed,
                                    signatureBytes, signatureLen,
                                    digest);
    assert (65 == signatureLen);

    char *signatureHex = encodeHexCreate(NULL, signatureBytes, signatureLen);
    printf ("      SigRaw: %s\n", signatureHex);
    assert (130 == strlen(signatureHex));
    assert (0 == strncmp (&signatureHex[ 0], SIGNATURE_V, 2));
    assert (0 == strncmp (&signatureHex[ 2], SIGNATURE_R, 64));
    assert (0 == strncmp (&signatureHex[66], SIGNATURE_S, 64));

    // Setup
    int success;
    size_t sigRDataLen, sigSDataLen;
    uint8_t *sigRData, *sigSData;

    sigRData = decodeHexCreate(&sigRDataLen, SIGNATURE_R, strlen (SIGNATURE_R));
    sigSData = decodeHexCreate(&sigSDataLen, SIGNATURE_S, strlen (SIGNATURE_S));
    assert (32 == sigRDataLen & 32 == sigSDataLen);

    // VRS
    printf ("      SigVRS\n");
    BREthereumSignature sigVRS = signatureCreate (SIGNATURE_TYPE_RECOVERABLE_VRS_EIP,
                                                  signingBytes, signingBytesCount,
                                                  privateKeyUncompressed);

    assert (sigVRS.sig.vrs.v == 0x1b);
    assert (0 == memcmp (sigVRS.sig.vrs.r, sigRData, sigRDataLen));
    assert (0 == memcmp (sigVRS.sig.vrs.s, sigSData, sigSDataLen));

    BREthereumAddress addrVRS = signatureExtractAddress (sigVRS, signingBytes, signingBytesCount, &success);
    assert (1 ==  success);


    // RSV
    printf ("      SigRSV\n");
    BREthereumSignature sigRSV = signatureCreate (SIGNATURE_TYPE_RECOVERABLE_RSV,
                                                  signingBytes, signingBytesCount,
                                                  privateKeyUncompressed);

    assert (sigRSV.sig.rsv.v == 0x00);
    assert (0 == memcmp (sigRSV.sig.rsv.r, sigRData, sigRDataLen));
    assert (0 == memcmp (sigRSV.sig.rsv.s, sigSData, sigSDataLen));

    BREthereumAddress addrRSV = signatureExtractAddress (sigRSV, signingBytes, signingBytesCount, &success);
    assert (1 == success);

    assert (ETHEREUM_BOOLEAN_TRUE == addressEqual (addrVRS, addrRSV));

}

static void runSignatureTests2 (void) {
    printf ("\n== Signature 2\n");
    UInt256 digest;

    printf ("    Data:\n");
    char *signingData = SIGNING_DATA_2;
    char *signingHash = SIGNING_HASH_2;

    size_t   signingBytesCount = 0;
    uint8_t *signingBytes = decodeHexCreate(&signingBytesCount, signingData, strlen (signingData));

    BRKeccak256(&digest, signingBytes, signingBytesCount);

    char *digestString = encodeHexCreate(NULL, (uint8_t *) &digest, sizeof(UInt256));
    printf ("      Hex: %s\n", digestString);
    assert (0 == strcmp (digestString, signingHash));


}

void runSignatureTests (void) {
    runSignatureTests1();
    runSignatureTests2();
}

extern void
runBaseTests () {
    runEtherParseTests();
    runSignatureTests();
}
