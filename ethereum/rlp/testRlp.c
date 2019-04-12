//
//  testRlp.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "ethereum/util/BRUtil.h"
#include "BRRlp.h"

static void
showHex (uint8_t *source, size_t sourceLen) {
    char *prefix = "{";
    for (int i = 0; i < sourceLen; i++) {
        printf("%s%x", prefix, source[i]);
        prefix = ", ";
    }
    printf ("}\n");
}

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
    BRRlpData data = rlpGetData(coder, item);
    assert (equalBytes(data.bytes, data.bytesCount, result, resultSize));
    printf (" => "); showHex (data.bytes, data.bytesCount);

    free (data.bytes);
}

void rlpCheckString (BRRlpCoder coder, const char *string, uint8_t *result, size_t resultSize) {
    printf ("  \"%s\"", string);
    BRRlpItem item = rlpEncodeString(coder, (char*) string);
    rlpCheck(coder, item, result, resultSize);
    rlpReleaseItem(coder, item);
}

void rlpCheckInt (BRRlpCoder coder, uint64_t value, uint8_t *result, size_t resultSize) {
    printf ("  %" PRIu64, value);
    BRRlpItem item = rlpEncodeUInt64(coder, value, 0);
    rlpCheck(coder, item, result, resultSize);
    rlpReleaseItem(coder, item);
}

void runRlpEncodeTest () {
    printf ("         Encode\n");

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
                                          rlpEncodeString(coder, "cat"),
                                          rlpEncodeString(coder, "dog"));
    uint8_t resCatDog[] = RLP_L1_RES;
    printf ("  \"%s\"", "[\"cat\" \"dog\"]");
    rlpCheck(coder, listCatDog, resCatDog, 9);
    rlpReleaseItem(coder, listCatDog);

    BRCoreParseStatus status = CORE_PARSE_OK;
    char *value = "5968770000000000000000";
    UInt256 r = createUInt256Parse(value, 10, &status);
    BRRlpItem item = rlpEncodeUInt256(coder, r, 0);
    BRRlpData data = rlpGetData(coder, item);
    rlpReleaseItem(coder, item);
    printf ("  %s\n    => ", value); showHex (data.bytes, data.bytesCount);
    char *dataHex = encodeHexCreate(NULL, data.bytes, data.bytesCount);
    printf ("    => %s\n", dataHex);
    assert (0 == strcasecmp (dataHex, "8a01439152d319e84d0000"));
    free (dataHex);

    rlpCoderRelease(coder);
    printf ("\n");
}

void runRlpDecodeTest () {
    printf ("         Decode\n");
    BRRlpCoder coder = rlpCoderCreate();
    size_t c;

    // cat & dog
    uint8_t l1b[] = RLP_L1_RES;
    BRRlpData l1d;
    l1d.bytes = l1b;
    l1d.bytesCount = 9;

    BRRlpItem l1i = rlpGetItem(coder, l1d);
    const BRRlpItem *l1is = rlpDecodeList (coder, l1i, &c);
    assert (2 == c);

    char *liCat = rlpDecodeString(coder, l1is[0]);
    char *liDog = rlpDecodeString(coder, l1is[1]);
    assert (0 == strcmp (liCat, "cat"));
    assert (0 == strcmp (liDog, "dog"));
    rlpReleaseItem(coder, l1i);
    free (liCat);
    free (liDog);

    uint8_t s3b[] = RLP_S3_RES;
    BRRlpData s3d;
    s3d.bytes = s3b;
    s3d.bytesCount = 58;

    BRRlpItem s3i = rlpGetItem(coder, s3d);
    char *s3Lorem = rlpDecodeString(coder, s3i);
    assert (0 == strcmp (s3Lorem, RLP_S3));
    rlpReleaseItem(coder, s3i);
   free (s3Lorem);

    //
    uint8_t v3b[] = RLP_V3_RES;
    BRRlpData v3d;
    v3d.bytes = v3b;
    v3d.bytesCount = 3;

    BRRlpItem v3i = rlpGetItem(coder, v3d);
    uint64_t v3v = rlpDecodeUInt64(coder, v3i, 0);
    assert (1024 == v3v);
    rlpReleaseItem(coder, v3i);

    rlpCoderRelease(coder);
}

void runRlpTests (void) {
    printf ("==== RLP\n");
    runRlpEncodeTest ();
    runRlpDecodeTest ();
}
