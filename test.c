//
//  test.c
//
//  Created by Aaron Voisine on 8/14/15.
//  Copyright (c) 2015 breadwallet LLC
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

#include "BRHash.h"
#include "BRBloomFilter.h"
#include "BRMerkleBlock.h"
#include "BRWallet.h"
#include "BRAddress.h"
#include "BRBase58.h"
#include "BRBIP39Mnemonic.h"
#include "BRBIP39WordsEn.H"
#include "BRPeer.h"
#include "BRPeerManager.h"
#include "BRPaymentProtocol.h"
#include "BRInt.h"
#include "BRArray.h"
#include "BRList.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#ifdef __ANDROID__
#include <android/log.h>
#define fprintf(...) __android_log_print(ANDROID_LOG_ERROR, "bread", _va_rest(__VA_ARGS__, NULL))
#define printf(...) __android_log_print(ANDROID_LOG_INFO, "bread", __VA_ARGS__)
#define _va_first(first, ...) first
#define _va_rest(first, ...) __VA_ARGS__
#endif

int BRIntsTests()
{
    // test endianess
    
    int r = 1;
    union {
        uint8_t u8[8];
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
    } x = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    
    if (be16(x.u16) != 0x0102) r = 0, fprintf(stderr, "***FAILED*** %s: be16() test\n", __func__);
    if (le16(x.u16) != 0x0201) r = 0, fprintf(stderr, "***FAILED*** %s: le16() test\n", __func__);
    if (be32(x.u32) != 0x01020304) r = 0, fprintf(stderr, "***FAILED*** %s: be32() test\n", __func__);
    if (le32(x.u32) != 0x04030201) r = 0, fprintf(stderr, "***FAILED*** %s: le32() test\n", __func__);
    if (be64(x.u64) != 0x0102030405060708) r = 0, fprintf(stderr, "***FAILED*** %s: be64() test\n", __func__);
    if (le64(x.u64) != 0x0807060504030201) r = 0, fprintf(stderr, "***FAILED*** %s: le64() test\n", __func__);
    
    printf("\n");
#if WORDS_BIGENDIAN
    printf("big endian\n");
#else
    printf("little endian\n");
#endif
    printf("                          ");
    return r;
}

int BRArrayTests()
{
    int r = 1;
    int *a = NULL, b[] = { 1, 2, 3 }, c[] = { 3, 2 };
    
    array_new(a, 0);                // [ ]
    if (array_count(a) != 0) r = 0, fprintf(stderr, "***FAILED*** %s: array_new() test\n", __func__);

    array_add(a, 0);                // [ 0 ]
    if (array_count(a) != 1 || a[0] != 0) r = 0, fprintf(stderr, "***FAILED*** %s: array_add() test\n", __func__);

    array_add_array(a, b, 3);       // [ 0, 1, 2, 3 ]
    if (array_count(a) != 4 || a[3] != 3) r = 0, fprintf(stderr, "***FAILED*** %s: array_add_array() test\n", __func__);

    array_insert(a, 0, 1);          // [ 1, 0, 1, 2, 3 ]
    if (array_count(a) != 5 || a[0] != 1) r = 0, fprintf(stderr, "***FAILED*** %s: array_insert() test\n", __func__);

    array_insert_array(a, 0, c, 2); // [ 3, 2, 1, 0, 1, 2, 3 ]
    if (array_count(a) != 7 || a[0] != 3)
        r = 0, fprintf(stderr, "***FAILED*** %s: array_insert_array() test\n", __func__);

    array_rm_range(a, 0, 4);        // [ 1, 2, 3 ]
    if (array_count(a) != 3 || a[0] != 1) r = 0, fprintf(stderr, "***FAILED*** %s: array_rm_range() test\n", __func__);
    printf("\n");

    for (size_t i = 0; i < array_count(a); i++) {
        printf("%i, ", a[i]);       // 1, 2, 3,
    }
    
    printf("\n");
    array_rm(a, 0);                 // [ 2, 3 ]
    if (array_count(a) != 2 || a[0] != 2) r = 0, fprintf(stderr, "***FAILED*** %s: array_rm() test\n", __func__);

    array_rm_last(a);               // [ 2 ]
    if (array_count(a) != 1 || a[0] != 2) r = 0, fprintf(stderr, "***FAILED*** %s: array_rm_last() test\n", __func__);
    
    array_clear(a);                 // [ ]
    if (array_count(a) != 0) r = 0, fprintf(stderr, "***FAILED*** %s: array_clear() test\n", __func__);
    
    array_free(a);
    printf("                          ");
    return r;
}

inline static int compare_ints(void *info, const void *a, const void *b)
{
    if (*(int *)a < *(int *)b) return -1;
    if (*(int *)a > *(int *)b) return 1;
    return 0;
}

int BRListTests()
{
    // test singly-linked list

    int r = 1;
    int *head = NULL, *item = NULL;
    
    list_new(head, 1);                   // 1
    if (! head || *head != 1) r = 0, fprintf(stderr, "***FAILED*** %s: list_new() test\n", __func__);
    list_insert_head(head, 2);           // 2->1
    if (! head || *head != 2) r = 0, fprintf(stderr, "***FAILED*** %s: list_insert_head() test\n", __func__);
    list_insert_after(head, 3);          // 2->3->1
    item = list_next(head);
    if (! item || *item != 3) r = 0, fprintf(stderr, "***FAILED*** %s: list_insert_after() test\n", __func__);
    list_sort(head, NULL, compare_ints); // 1->2->3
    printf("\n");

    for (item = head; item; item = list_next(item)) {
        printf("%i->", *item);           // "1->2->3->"
    }

    printf("\n");
    if (! head || *head != 1) r = 0, fprintf(stderr, "***FAILED*** %s: list_sort() test\n", __func__);
    item = list_next(head);
    if (! item || *item != 2) r = 0, fprintf(stderr, "***FAILED*** %s: list_sort() test\n", __func__);
    item = list_next(list_next(head));
    if (! item || *item != 3) r = 0, fprintf(stderr, "***FAILED*** %s: list_sort() test\n", __func__);
    item = list_next(head);
    list_rm_after(item);                 // 1->2
    item = list_next(item);
    if (item) r = 0, fprintf(stderr, "***FAILED*** %s: list_rm_after() test\n", __func__);
    list_rm_head(head);                  // 2
    item = list_next(head);
    if (item) r = 0, fprintf(stderr, "***FAILED*** %s: list_rm_head() test\n", __func__);
    list_free(head);
    if (head) r = 0, fprintf(stderr, "***FAILED*** %s: list_free() test\n", __func__);
    
    printf("                          ");
    return r;
}

int BRBase58Tests()
{
    int r = 1;
    char *s;
    
    s = "#&$@*^(*#!^"; // test bad input
    
    uint8_t buf1[BRBase58Decode(NULL, 0, s)];
    size_t len1 = BRBase58Decode(buf1, sizeof(buf1), s);

    if (len1 != 0) r = 0, fprintf(stderr, "***FAILED*** %s: Base58Decode() test 1\n", __func__);

    uint8_t buf2[BRBase58Decode(NULL, 0, "")];
    size_t len2 = BRBase58Decode(buf2, sizeof(buf2), "");
    
    if (len2 != 0) r = 0, fprintf(stderr, "***FAILED*** %s: Base58Decode() test 2\n", __func__);
    
    s = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    
    uint8_t buf3[BRBase58Decode(NULL, 0, s)];
    size_t len3 = BRBase58Decode(buf3, sizeof(buf3), s);
    char str3[BRBase58Encode(NULL, 0, buf3, len3)];
    
    BRBase58Encode(str3, sizeof(str3), buf3, len3);
    if (strcmp(str3, s) != 0) r = 0, fprintf(stderr, "***FAILED*** %s: Base58Decode() test 3\n", __func__);

    s = "1111111111111111111111111111111111111111111111111111111111111111111";

    uint8_t buf4[BRBase58Decode(NULL, 0, s)];
    size_t len4 = BRBase58Decode(buf4, sizeof(buf4), s);
    char str4[BRBase58Encode(NULL, 0, buf4, len4)];
    
    BRBase58Encode(str4, sizeof(str4), buf4, len4);
    if (strcmp(str4, s) != 0) r = 0, fprintf(stderr, "***FAILED*** %s: Base58Decode() test 4\n", __func__);

    s = "111111111111111111111111111111111111111111111111111111111111111111z";

    uint8_t buf5[BRBase58Decode(NULL, 0, s)];
    size_t len5 = BRBase58Decode(buf5, sizeof(buf5), s);
    char str5[BRBase58Encode(NULL, 0, buf5, len5)];
    
    BRBase58Encode(str5, sizeof(str5), buf5, len5);
    if (strcmp(str5, s) != 0) r = 0, fprintf(stderr, "***FAILED*** %s: Base58Decode() test 5\n", __func__);

    s = "z";
    
    uint8_t buf6[BRBase58Decode(NULL, 0, s)];
    size_t len6 = BRBase58Decode(buf6, sizeof(buf6), s);
    char str6[BRBase58Encode(NULL, 0, buf6, len6)];
    
    BRBase58Encode(str6, sizeof(str6), buf6, len6);
    if (strcmp(str6, s) != 0) r = 0, fprintf(stderr, "***FAILED*** %s: Base58Decode() test 6\n", __func__);

    s = NULL;
    
    char s1[BRBase58CheckEncode(NULL, 0, (uint8_t *)s, 0)];
    size_t l1 = BRBase58CheckEncode(s1, sizeof(s1), (uint8_t *)s, 0);
    uint8_t b1[BRBase58CheckDecode(NULL, 0, s1)];
    
    l1 = BRBase58CheckDecode(b1, sizeof(b1), s1);
    if (l1 != 0) r = 0, fprintf(stderr, "***FAILED*** %s: BRBase58CheckDecode() test 1\n", __func__);

    s = "";

    char s2[BRBase58CheckEncode(NULL, 0, (uint8_t *)s, 0)];
    size_t l2 = BRBase58CheckEncode(s2, sizeof(s2), (uint8_t *)s, 0);
    uint8_t b2[BRBase58CheckDecode(NULL, 0, s2)];
    
    l2 = BRBase58CheckDecode(b2, sizeof(b2), s2);
    if (l2 != 0) r = 0, fprintf(stderr, "***FAILED*** %s: BRBase58CheckDecode() test 2\n", __func__);
    
    s = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    
    char s3[BRBase58CheckEncode(NULL, 0, (uint8_t *)s, 21)];
    size_t l3 = BRBase58CheckEncode(s3, sizeof(s3), (uint8_t *)s, 21);
    uint8_t b3[BRBase58CheckDecode(NULL, 0, s3)];
    
    l3 = BRBase58CheckDecode(b3, sizeof(b3), s3);
    if (l3 != 21 || memcmp(s, b3, l3) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBase58CheckDecode() test 3\n", __func__);

    s = "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x01";
    
    char s4[BRBase58CheckEncode(NULL, 0, (uint8_t *)s, 21)];
    size_t l4 = BRBase58CheckEncode(s4, sizeof(s4), (uint8_t *)s, 21);
    uint8_t b4[BRBase58CheckDecode(NULL, 0, s4)];
    
    l4 = BRBase58CheckDecode(b4, sizeof(b4), s4);
    if (l4 != 21 || memcmp(s, b4, l4) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBase58CheckDecode() test 4\n", __func__);

    s = "\x05\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF";
    
    char s5[BRBase58CheckEncode(NULL, 0, (uint8_t *)s, 21)];
    size_t l5 = BRBase58CheckEncode(s5, sizeof(s5), (uint8_t *)s, 21);
    uint8_t b5[BRBase58CheckDecode(NULL, 0, s5)];
    
    l5 = BRBase58CheckDecode(b5, sizeof(b5), s5);
    if (l5 != 21 || memcmp(s, b5, l5) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBase58CheckDecode() test 5\n", __func__);

    return r;
}

int BRHashTests()
{
    // test sha1
    
    int r = 1;
    uint8_t md[64];
    char *s;
    
    s = "Free online SHA1 Calculator, type text here...";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x6f\xc2\xe2\x51\x72\xcb\x15\x19\x3c\xb1\xc6\xd4\x8f\x60\x7d\x42\xc1\xd2\xa2\x15",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA1() test 1\n", __func__);
        
    s = "this is some text to test the sha1 implementation with more than 64bytes of data since it's internal digest "
        "buffer is 64bytes in size";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x08\x51\x94\x65\x8a\x92\x35\xb2\x95\x1a\x83\xd1\xb8\x26\xb9\x87\xe9\x38\x5a\xa3",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA1() test 2\n", __func__);
        
    s = "123456789012345678901234567890123456789012345678901234567890";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x24\x5b\xe3\x00\x91\xfd\x39\x2f\xe1\x91\xf4\xbf\xce\xc2\x2d\xcb\x30\xa0\x3a\xe6",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA1() test 3\n", __func__);
    
    // a message exactly 64bytes long (internal buffer size)
    s = "1234567890123456789012345678901234567890123456789012345678901234";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\xc7\x14\x90\xfc\x24\xaa\x3d\x19\xe1\x12\x82\xda\x77\x03\x2d\xd9\xcd\xb3\x31\x03",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA1() test 4\n", __func__);
    
    s = ""; // empty
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\xda\x39\xa3\xee\x5e\x6b\x4b\x0d\x32\x55\xbf\xef\x95\x60\x18\x90\xaf\xd8\x07\x09",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA1() test 5\n", __func__);
    
    s = "a";
    BRSHA1(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x86\xf7\xe4\x37\xfa\xa5\xa7\xfc\xe1\x5d\x1d\xdc\xb9\xea\xea\xea\x37\x76\x67\xb8",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA1() test 6\n", __func__);

    // test sha256
    
    s = "Free online SHA256 Calculator, type text here...";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\x43\xfd\x9d\xeb\x93\xf6\xe1\x4d\x41\x82\x66\x04\x51\x4e\x3d\x78\x73\xa5\x49\xac"
                    "\x87\xae\xbe\xbf\x3d\x1c\x10\xad\x6e\xb0\x57\xd0", *(UInt256 *)md))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA256() test 1\n", __func__);
        
    s = "this is some text to test the sha256 implementation with more than 64bytes of data since it's internal "
        "digest buffer is 64bytes in size";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\x40\xfd\x09\x33\xdf\x2e\x77\x47\xf1\x9f\x7d\x39\xcd\x30\xe1\xcb\x89\x81\x0a\x7e"
                    "\x47\x06\x38\xa5\xf6\x23\x66\x9f\x3d\xe9\xed\xd4", *(UInt256 *)md))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA256() test 2\n", __func__);
    
    s = "123456789012345678901234567890123456789012345678901234567890";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\xde\xcc\x53\x8c\x07\x77\x86\x96\x6a\xc8\x63\xb5\x53\x2c\x40\x27\xb8\x58\x7f\xf4"
                    "\x0f\x6e\x31\x03\x37\x9a\xf6\x2b\x44\xea\xe4\x4d", *(UInt256 *)md))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA256() test 3\n", __func__);
    
    // a message exactly 64bytes long (internal buffer size)
    s = "1234567890123456789012345678901234567890123456789012345678901234";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\x67\x64\x91\x96\x5e\xd3\xec\x50\xcb\x7a\x63\xee\x96\x31\x54\x80\xa9\x5c\x54\x42"
                    "\x6b\x0b\x72\xbc\xa8\xa0\xd4\xad\x12\x85\xad\x55", *(UInt256 *)md))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA256() test 4\n", __func__);
    
    s = ""; // empty
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\xe3\xb0\xc4\x42\x98\xfc\x1c\x14\x9a\xfb\xf4\xc8\x99\x6f\xb9\x24\x27\xae\x41\xe4"
                    "\x64\x9b\x93\x4c\xa4\x95\x99\x1b\x78\x52\xb8\x55", *(UInt256 *)md))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA256() test 5\n", __func__);
    
    s = "a";
    BRSHA256(md, s, strlen(s));
    if (! UInt256Eq(*(UInt256 *)"\xca\x97\x81\x12\xca\x1b\xbd\xca\xfa\xc2\x31\xb3\x9a\x23\xdc\x4d\xa7\x86\xef\xf8"
                    "\x14\x7c\x4e\x72\xb9\x80\x77\x85\xaf\xee\x48\xbb", *(UInt256 *)md))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA256() test 6\n", __func__);

    // test sha512
    
    s = "Free online SHA512 Calculator, type text here...";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x04\xf1\x15\x41\x35\xee\xcb\xe4\x2e\x9a\xdc\x8e\x1d\x53\x2f\x9c\x60\x7a\x84\x47"
                    "\xb7\x86\x37\x7d\xb8\x44\x7d\x11\xa5\xb2\x23\x2c\xdd\x41\x9b\x86\x39\x22\x4f\x78\x7a\x51"
                    "\xd1\x10\xf7\x25\x91\xf9\x64\x51\xa1\xbb\x51\x1c\x4a\x82\x9e\xd0\xa2\xec\x89\x13\x21\xf3",
                    *(UInt512 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA512() test 1\n", __func__);
    
    s = "this is some text to test the sha512 implementation with more than 128bytes of data since it's internal "
        "digest buffer is 128bytes in size";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x9b\xd2\xdc\x7b\x05\xfb\xbe\x99\x34\xcb\x32\x89\xb6\xe0\x6b\x8c\xa9\xfd\x7a\x55"
                    "\xe6\xde\x5d\xb7\xe1\xe4\xee\xdd\xc6\x62\x9b\x57\x53\x07\x36\x7c\xd0\x18\x3a\x44\x61\xd7"
                    "\xeb\x2d\xfc\x6a\x27\xe4\x1e\x8b\x70\xf6\x59\x8e\xbc\xc7\x71\x09\x11\xd4\xfb\x16\xa3\x90",
                    *(UInt512 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA512() test 2\n", __func__);
    
    s = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567"
        "8901234567890";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x0d\x9a\x7d\xf5\xb6\xa6\xad\x20\xda\x51\x9e\xff\xda\x88\x8a\x73\x44\xb6\xc0\xc7"
                    "\xad\xcc\x8e\x2d\x50\x4b\x4a\xf2\x7a\xaa\xac\xd4\xe7\x11\x1c\x71\x3f\x71\x76\x95\x39\x62"
                    "\x94\x63\xcb\x58\xc8\x61\x36\xc5\x21\xb0\x41\x4a\x3c\x0e\xdf\x7d\xc6\x34\x9c\x6e\xda\xf3",
                    *(UInt512 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA512() test 3\n", __func__);
    
    //exactly 128bytes (internal buf size)
    s = "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567"
        "890123456789012345678";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x22\x2b\x2f\x64\xc2\x85\xe6\x69\x96\x76\x9b\x5a\x03\xef\x86\x3c\xfd\x3b\x63\xdd"
                    "\xb0\x72\x77\x88\x29\x16\x95\xe8\xfb\x84\x57\x2e\x4b\xfe\x5a\x80\x67\x4a\x41\xfd\x72\xee"
                    "\xb4\x85\x92\xc9\xc7\x9f\x44\xae\x99\x2c\x76\xed\x1b\x0d\x55\xa6\x70\xa8\x3f\xc9\x9e\xc6",
                    *(UInt512 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA512() test 4\n", __func__);
    
    s = ""; // empty
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\xcf\x83\xe1\x35\x7e\xef\xb8\xbd\xf1\x54\x28\x50\xd6\x6d\x80\x07\xd6\x20\xe4\x05"
                    "\x0b\x57\x15\xdc\x83\xf4\xa9\x21\xd3\x6c\xe9\xce\x47\xd0\xd1\x3c\x5d\x85\xf2\xb0\xff\x83"
                    "\x18\xd2\x87\x7e\xec\x2f\x63\xb9\x31\xbd\x47\x41\x7a\x81\xa5\x38\x32\x7a\xf9\x27\xda\x3e",
                    *(UInt512 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA512() test 5\n", __func__);
    
    s = "a";
    BRSHA512(md, s, strlen(s));
    if (! UInt512Eq(*(UInt512 *)"\x1f\x40\xfc\x92\xda\x24\x16\x94\x75\x09\x79\xee\x6c\xf5\x82\xf2\xd5\xd7\xd2\x8e"
                    "\x18\x33\x5d\xe0\x5a\xbc\x54\xd0\x56\x0e\x0f\x53\x02\x86\x0c\x65\x2b\xf0\x8d\x56\x02\x52"
                    "\xaa\x5e\x74\x21\x05\x46\xf3\x69\xfb\xbb\xce\x8c\x12\xcf\xc7\x95\x7b\x26\x52\xfe\x9a\x75",
                    *(UInt512 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRSHA512() test 6\n", __func__);
    
    // test ripemd160
    
    s = "Free online RIPEMD160 Calculator, type text here...";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x95\x01\xa5\x6f\xb8\x29\x13\x2b\x87\x48\xf0\xcc\xc4\x91\xf0\xec\xbc\x7f\x94\x5b",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRRMD160() test 1\n", __func__);
    
    s = "this is some text to test the ripemd160 implementation with more than 64bytes of data since it's internal "
        "digest buffer is 64bytes in size";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x44\x02\xef\xf4\x21\x57\x10\x6a\x5d\x92\xe4\xd9\x46\x18\x58\x56\xfb\xc5\x0e\x09",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRRMD160() test 2\n", __func__);
    
    s = "123456789012345678901234567890123456789012345678901234567890";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x00\x26\x3b\x99\x97\x14\xe7\x56\xfa\x5d\x02\x81\x4b\x84\x2a\x26\x34\xdd\x31\xac",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRRMD160() test 3\n", __func__);
    
    // a message exactly 64bytes long (internal buffer size)
    s = "1234567890123456789012345678901234567890123456789012345678901234";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\xfa\x8c\x1a\x78\xeb\x76\x3b\xb9\x7d\x5e\xa1\x4c\xe9\x30\x3d\x1c\xe2\xf3\x34\x54",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRRMD160() test 4\n", __func__);
    
    s = ""; // empty
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x9c\x11\x85\xa5\xc5\xe9\xfc\x54\x61\x28\x08\x97\x7e\xe8\xf5\x48\xb2\x25\x8d\x31",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRRMD160() test 5\n", __func__);
    
    s = "a";
    BRRMD160(md, s, strlen(s));
    if (! UInt160Eq(*(UInt160 *)"\x0b\xdc\x9d\x2d\x25\x6b\x3e\xe9\xda\xae\x34\x7b\xe6\xf4\xdc\x83\x5a\x46\x7f\xfe",
                    *(UInt160 *)md)) r = 0, fprintf(stderr, "***FAILED*** %s: BRRMD160() test 6\n", __func__);

    return r;
}

int BRKeyTests()
{
    int r = 1;
    BRKey key;
    BRAddress addr;
    char *msg;
    UInt256 md;
    uint8_t sig[72], pubKey[65];
    size_t sigLen, pkLen;

    if (BRPrivKeyIsValid("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRz"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPrivKeyIsValid() test 0\n", __func__);

    // mini private key format
    if (! BRPrivKeyIsValid("S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPrivKeyIsValid() test 1\n", __func__);

    printf("\n");
    BRKeySetPrivKey(&key, "S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy");
    BRKeyAddress(&key, addr.s, sizeof(addr));
    printf("privKey:S6c56bnXQiBjk9mqSYE7ykVQ7NzrRy = %s\n", addr.s);
#if BITCOIN_TESTNET
    if (! BRAddressEq(&addr, "ms8fwvXzrCoyatnGFRaLbepSqwGRxVJQF1"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySetPrivKey() test 1\n", __func__);
#else
    if (! BRAddressEq(&addr, "1CciesT23BNionJeXrbxmjc7ywfiyM4oLW"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySetPrivKey() test 1\n", __func__);
#endif

    // old mini private key format
    if (! BRPrivKeyIsValid("SzavMBLoXU6kDrqtUVmffv"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPrivKeyIsValid() test 2\n", __func__);

    BRKeySetPrivKey(&key, "SzavMBLoXU6kDrqtUVmffv");
    BRKeyAddress(&key, addr.s, sizeof(addr));
    printf("privKey:SzavMBLoXU6kDrqtUVmffv = %s\n", addr.s);
#if BITCOIN_TESTNET
    if (! BRAddressEq(&addr, "mrhzp5mstA4Midx85EeCjuaUAAGANMFmRP"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySetPrivKey() test 2\n", __func__);
#else
    if (! BRAddressEq(&addr, "1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySetPrivKey() test 2\n", __func__);
#endif

#if ! BITCOIN_TESTNET
    // uncompressed private key
    if (! BRPrivKeyIsValid("5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPrivKeyIsValid() test 3\n", __func__);
    
    BRKeySetPrivKey(&key, "5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF");
    BRKeyAddress(&key, addr.s, sizeof(addr));
    printf("privKey:5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF = %s\n", addr.s);
    if (! BRAddressEq(&addr, "1CC3X2gu58d6wXUWMffpuzN9JAfTUWu4Kj"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySetPrivKey() test 3\n", __func__);

    // uncompressed private key export
    char privKey1[BRKeyPrivKey(&key, NULL, 0)];
    
    BRKeyPrivKey(&key, privKey1, sizeof(privKey1));
    printf("privKey = %s\n", privKey1);
    if (strcmp(privKey1, "5Kb8kLf9zgWQnogidDA76MzPL6TsZZY36hWXMssSzNydYXYB9KF") != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyPrivKey() test 1\n", __func__);
    
    // compressed private key
    if (! BRPrivKeyIsValid("KyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPrivKeyIsValid() test 4\n", __func__);
    
    BRKeySetPrivKey(&key, "KyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL");
    BRKeyAddress(&key, addr.s, sizeof(addr));
    printf("privKey:KyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL = %s\n", addr.s);
    if (! BRAddressEq(&addr, "1JMsC6fCtYWkTjPPdDrYX3we2aBrewuEM3"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySetPrivKey() test 4\n", __func__);
    
    // compressed private key export
    char privKey2[BRKeyPrivKey(&key, NULL, 0)];
    
    BRKeyPrivKey(&key, privKey2, sizeof(privKey2));
    printf("privKey = %s\n", privKey2);
    if (strcmp(privKey2, "KyvGbxRUoofdw3TNydWn2Z78dBHSy2odn1d3wXWN2o3SAtccFNJL") != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyPrivKey() test 2\n", __func__);
#endif
    
    // signing
    BRKeySetSecret(&key, &uint256_hex_decode("0000000000000000000000000000000000000000000000000000000000000001"), 1);
    msg = "Everything should be made as simple as possible, but not simpler.";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeySign(&key, sig, sizeof(sig), md);
    
    char sig1[] = "\x30\x44\x02\x20\x33\xa6\x9c\xd2\x06\x54\x32\xa3\x0f\x3d\x1c\xe4\xeb\x0d\x59\xb8\xab\x58\xc7\x4f\x27"
    "\xc4\x1a\x7f\xdb\x56\x96\xad\x4e\x61\x08\xc9\x02\x20\x6f\x80\x79\x82\x86\x6f\x78\x5d\x3f\x64\x18\xd2\x41\x63\xdd"
    "\xae\x11\x7b\x7d\xb4\xd5\xfd\xf0\x07\x1d\xe0\x69\xfa\x54\x34\x22\x62";

    if (sigLen != sizeof(sig1) - 1 || memcmp(sig, sig1, sigLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySign() test 1\n", __func__);

    if (! BRKeyVerify(&key, md, sig, sigLen))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyVerify() test 1\n", __func__);

    BRKeySetSecret(&key, &uint256_hex_decode("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140"), 1);
    msg = "Equations are more important to me, because politics is for the present, but an equation is something for "
    "eternity.";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeySign(&key, sig, sizeof(sig), md);
    
    char sig2[] = "\x30\x44\x02\x20\x54\xc4\xa3\x3c\x64\x23\xd6\x89\x37\x8f\x16\x0a\x7f\xf8\xb6\x13\x30\x44\x4a\xbb\x58"
    "\xfb\x47\x0f\x96\xea\x16\xd9\x9d\x4a\x2f\xed\x02\x20\x07\x08\x23\x04\x41\x0e\xfa\x6b\x29\x43\x11\x1b\x6a\x4e\x0a"
    "\xaa\x7b\x7d\xb5\x5a\x07\xe9\x86\x1d\x1f\xb3\xcb\x1f\x42\x10\x44\xa5";

    if (sigLen != sizeof(sig2) - 1 || memcmp(sig, sig2, sigLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySign() test 2\n", __func__);
    
    if (! BRKeyVerify(&key, md, sig, sigLen))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyVerify() test 2\n", __func__);

    BRKeySetSecret(&key, &uint256_hex_decode("fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364140"), 1);
    msg = "Not only is the Universe stranger than we think, it is stranger than we can think.";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeySign(&key, sig, sizeof(sig), md);
    
    char sig3[] = "\x30\x45\x02\x21\x00\xff\x46\x6a\x9f\x1b\x7b\x27\x3e\x2f\x4c\x3f\xfe\x03\x2e\xb2\xe8\x14\x12\x1e\xd1"
    "\x8e\xf8\x46\x65\xd0\xf5\x15\x36\x0d\xab\x3d\xd0\x02\x20\x6f\xc9\x5f\x51\x32\xe5\xec\xfd\xc8\xe5\xe6\xe6\x16\xcc"
    "\x77\x15\x14\x55\xd4\x6e\xd4\x8f\x55\x89\xb7\xdb\x77\x71\xa3\x32\xb2\x83";
    
    if (sigLen != sizeof(sig3) - 1 || memcmp(sig, sig3, sigLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySign() test 3\n", __func__);
    
    if (! BRKeyVerify(&key, md, sig, sigLen))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyVerify() test 3\n", __func__);

    BRKeySetSecret(&key, &uint256_hex_decode("0000000000000000000000000000000000000000000000000000000000000001"), 1);
    msg = "How wonderful that we have met with a paradox. Now we have some hope of making progress.";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeySign(&key, sig, sizeof(sig), md);
    
    char sig4[] = "\x30\x45\x02\x21\x00\xc0\xda\xfe\xc8\x25\x1f\x1d\x50\x10\x28\x9d\x21\x02\x32\x22\x0b\x03\x20\x2c\xba"
    "\x34\xec\x11\xfe\xc5\x8b\x3e\x93\xa8\x5b\x91\xd3\x02\x20\x75\xaf\xdc\x06\xb7\xd6\x32\x2a\x59\x09\x55\xbf\x26\x4e"
    "\x7a\xaa\x15\x58\x47\xf6\x14\xd8\x00\x78\xa9\x02\x92\xfe\x20\x50\x64\xd3";
    
    if (sigLen != sizeof(sig4) - 1 || memcmp(sig, sig4, sigLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySign() test 4\n", __func__);
    
    if (! BRKeyVerify(&key, md, sig, sigLen))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyVerify() test 4\n", __func__);

    BRKeySetSecret(&key, &uint256_hex_decode("69ec59eaa1f4f2e36b639716b7c30ca86d9a5375c7b38d8918bd9c0ebc80ba64"), 1);
    msg = "Computer science is no more about computers than astronomy is about telescopes.";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeySign(&key, sig, sizeof(sig), md);
    
    char sig5[] = "\x30\x44\x02\x20\x71\x86\x36\x35\x71\xd6\x5e\x08\x4e\x7f\x02\xb0\xb7\x7c\x3e\xc4\x4f\xb1\xb2\x57\xde"
    "\xe2\x62\x74\xc3\x8c\x92\x89\x86\xfe\xa4\x5d\x02\x20\x0d\xe0\xb3\x8e\x06\x80\x7e\x46\xbd\xa1\xf1\xe2\x93\xf4\xf6"
    "\x32\x3e\x85\x4c\x86\xd5\x8a\xbd\xd0\x0c\x46\xc1\x64\x41\x08\x5d\xf6";
    
    if (sigLen != sizeof(sig5) - 1 || memcmp(sig, sig5, sigLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySign() test 5\n", __func__);
    
    if (! BRKeyVerify(&key, md, sig, sigLen))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyVerify() test 5\n", __func__);

    BRKeySetSecret(&key, &uint256_hex_decode("00000000000000000000000000007246174ab1e92e9149c6e446fe194d072637"), 1);
    msg = "...if you aren't, at any given time, scandalized by code you wrote five or even three years ago, you're not"
    " learning anywhere near enough";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeySign(&key, sig, sizeof(sig), md);
    
    char sig6[] = "\x30\x45\x02\x21\x00\xfb\xfe\x50\x76\xa1\x58\x60\xba\x8e\xd0\x0e\x75\xe9\xbd\x22\xe0\x5d\x23\x0f\x02"
    "\xa9\x36\xb6\x53\xeb\x55\xb6\x1c\x99\xdd\xa4\x87\x02\x20\x0e\x68\x88\x0e\xbb\x00\x50\xfe\x43\x12\xb1\xb1\xeb\x08"
    "\x99\xe1\xb8\x2d\xa8\x9b\xaa\x5b\x89\x5f\x61\x26\x19\xed\xf3\x4c\xbd\x37";
    
    if (sigLen != sizeof(sig6) - 1 || memcmp(sig, sig6, sigLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySign() test 6\n", __func__);
    
    if (! BRKeyVerify(&key, md, sig, sigLen))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyVerify() test 6\n", __func__);

    BRKeySetSecret(&key, &uint256_hex_decode("000000000000000000000000000000000000000000056916d0f9b31dc9b637f3"), 1);
    msg = "The question of whether computers can think is like the question of whether submarines can swim.";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeySign(&key, sig, sizeof(sig), md);
    
    char sig7[] = "\x30\x45\x02\x21\x00\xcd\xe1\x30\x2d\x83\xf8\xdd\x83\x5d\x89\xae\xf8\x03\xc7\x4a\x11\x9f\x56\x1f\xba"
    "\xef\x3e\xb9\x12\x9e\x45\xf3\x0d\xe8\x6a\xbb\xf9\x02\x20\x06\xce\x64\x3f\x50\x49\xee\x1f\x27\x89\x04\x67\xb7\x7a"
    "\x6a\x8e\x11\xec\x46\x61\xcc\x38\xcd\x8b\xad\xf9\x01\x15\xfb\xd0\x3c\xef";
    
    if (sigLen != sizeof(sig7) - 1 || memcmp(sig, sig7, sigLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeySign() test 7\n", __func__);
    
    if (! BRKeyVerify(&key, md, sig, sigLen))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyVerify() test 7\n", __func__);

    // compact signing
    BRKeySetSecret(&key, &uint256_hex_decode("0000000000000000000000000000000000000000000000000000000000000001"), 1);
    msg = "foo";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeyCompactSign(&key, sig, sizeof(sig), md);
    pkLen = BRPubKeyRecover(pubKey, sizeof(pubKey), sig, sigLen, md);
    
    uint8_t pubKey1[BRKeyPubKey(&key, NULL, 0)];
    size_t pkLen1 = BRKeyPubKey(&key, pubKey1, sizeof(pubKey1));
    
    if (pkLen1 != pkLen || memcmp(pubKey, pubKey1, pkLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyCompactSign() test 1\n", __func__);

    BRKeySetSecret(&key, &uint256_hex_decode("0000000000000000000000000000000000000000000000000000000000000001"), 0);
    msg = "foo";
    BRSHA256(&md, msg, strlen(msg));
    sigLen = BRKeyCompactSign(&key, sig, sizeof(sig), md);
    pkLen = BRPubKeyRecover(pubKey, sizeof(pubKey), sig, sigLen, md);
    
    uint8_t pubKey2[BRKeyPubKey(&key, NULL, 0)];
    size_t pkLen2 = BRKeyPubKey(&key, pubKey2, sizeof(pubKey2));
    
    if (pkLen2 != pkLen || memcmp(pubKey, pubKey2, pkLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyCompactSign() test 2\n", __func__);

    // compact pubkey recovery
    pkLen = BRBase58Decode(pubKey, sizeof(pubKey), "26wZYDdvpmCrYZeUcxgqd1KquN4o6wXwLomBW5SjnwUqG");
    msg = "i am a test signed string";
    BRSHA256_2(&md, msg, strlen(msg));
    sigLen = BRBase58Decode(sig, sizeof(sig),
                           "3kq9e842BzkMfbPSbhKVwGZgspDSkz4YfqjdBYQPWDzqd77gPgR1zq4XG7KtAL5DZTcfFFs2iph4urNyXeBkXsEYY");
    
    uint8_t pubKey3[BRPubKeyRecover(NULL, 0, sig, sigLen, md)];
    size_t pkLen3 = BRPubKeyRecover(pubKey3, sizeof(pubKey3), sig, sigLen, md);

    if (pkLen3 != pkLen || memcmp(pubKey, pubKey3, pkLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPubKeyRecover() test 1\n", __func__);

    pkLen = BRBase58Decode(pubKey, sizeof(pubKey), "26wZYDdvpmCrYZeUcxgqd1KquN4o6wXwLomBW5SjnwUqG");
    msg = "i am a test signed string do de dah";
    BRSHA256_2(&md, msg, strlen(msg));
    sigLen = BRBase58Decode(sig, sizeof(sig),
                           "3qECEYmb6x4X22sH98Aer68SdfrLwtqvb5Ncv7EqKmzbxeYYJ1hU9irP6R5PeCctCPYo5KQiWFgoJ3H5MkuX18gHu");
    
    uint8_t pubKey4[BRPubKeyRecover(NULL, 0, sig, sigLen, md)];
    size_t pkLen4 = BRPubKeyRecover(pubKey4, sizeof(pubKey4), sig, sigLen, md);
    
    if (pkLen4 != pkLen || memcmp(pubKey, pubKey4, pkLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPubKeyRecover() test 2\n", __func__);

    pkLen = BRBase58Decode(pubKey, sizeof(pubKey), "gpRv1sNA3XURB6QEtGrx6Q18DZ5cSgUSDQKX4yYypxpW");
    msg = "i am a test signed string";
    BRSHA256_2(&md, msg, strlen(msg));
    sigLen = BRBase58Decode(sig, sizeof(sig),
                           "3oHQhxq5eW8dnp7DquTCbA5tECoNx7ubyiubw4kiFm7wXJF916SZVykFzb8rB1K6dEu7mLspBWbBEJyYk79jAosVR");
    
    uint8_t pubKey5[BRPubKeyRecover(NULL, 0, sig, sigLen, md)];
    size_t pkLen5 = BRPubKeyRecover(pubKey5, sizeof(pubKey5), sig, sigLen, md);
    
    if (pkLen5 != pkLen || memcmp(pubKey, pubKey5, pkLen) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPubKeyRecover() test 3\n", __func__);

    printf("                          ");
    return r;
}

int BRAddressTests()
{
    int r = 1;
    UInt256 secret = uint256_hex_decode("0000000000000000000000000000000000000000000000000000000000000001");
    BRKey k;
    BRAddress addr, addr2;
    
    BRKeySetSecret(&k, &secret, 1);
    if (! BRKeyAddress(&k, addr.s, sizeof(addr)))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRKeyAddress()\n", __func__);

    uint8_t script[BRAddressScriptPubKey(NULL, 0, addr.s)];
    size_t scriptLen = BRAddressScriptPubKey(script, sizeof(script), addr.s);
    
    BRAddressFromScriptPubKey(addr2.s, sizeof(addr2), script, scriptLen);
    if (! BRAddressEq(&addr, &addr2))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRAddressFromScriptPubKey()\n", __func__);
    
    // TODO: test BRAddressFromScriptSig()
    
    return r;
}

int BRBIP39MnemonicTests()
{
    int r = 1;
    
    const char *s = "bless cloud wheel regular tiny venue bird web grief security dignity zoo";

    // test correct handling of bad checksum
    if (BRBIP39PhraseIsValid(BRBIP39WordsEn, s))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39PhraseIsValid() test\n", __func__);

    UInt512 key = UINT512_ZERO;

    BRBIP39DeriveKey(key.u8, NULL, NULL); // test invalid key
    if (! UInt512IsZero(key)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 0\n", __func__);

    UInt128 entropy = UINT128_ZERO;
    char phrase[BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy.u8, sizeof(entropy))];
    size_t len = BRBIP39Encode(phrase, sizeof(phrase), BRBIP39WordsEn, entropy.u8, sizeof(entropy));
    
    if (strncmp(phrase, "abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about",
                len)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Encode() test 1\n", __func__);
    
    BRBIP39Decode(entropy.u8, sizeof(entropy), BRBIP39WordsEn, phrase);
    if (! UInt128IsZero(entropy)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Decode() test 1\n", __func__);
    
    BRBIP39DeriveKey(key.u8, phrase, "TREZOR");
    if (! UInt512Eq(key, *(UInt512 *)"\xc5\x52\x57\xc3\x60\xc0\x7c\x72\x02\x9a\xeb\xc1\xb5\x3c\x05\xed\x03\x62\xad\xa3"
                    "\x8e\xad\x3e\x3e\x9e\xfa\x37\x08\xe5\x34\x95\x53\x1f\x09\xa6\x98\x75\x99\xd1\x82\x64\xc1\xe1\xc9"
                    "\x2f\x2c\xf1\x41\x63\x0c\x7a\x3c\x4a\xb7\xc8\x1b\x2f\x00\x16\x98\xe7\x46\x3b\x04"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 1\n", __func__);

    UInt128 entropy2 = *(UInt128 *)"\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f\x7f";
    char phrase2[BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy2.u8, sizeof(entropy2))];
    size_t len2 = BRBIP39Encode(phrase2, sizeof(phrase2), BRBIP39WordsEn, entropy2.u8, sizeof(entropy2));
    
    if (strncmp(phrase2, "legal winner thank year wave sausage worth useful legal winner thank yellow", len2))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Encode() test 2\n", __func__);

    BRBIP39Decode(entropy.u8, sizeof(entropy), BRBIP39WordsEn, phrase2);
    if (! UInt128Eq(entropy2, entropy)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Decode() test 2\n", __func__);
    
    BRBIP39DeriveKey(key.u8, phrase2, "TREZOR");
    if (! UInt512Eq(key, *(UInt512 *)"\x2e\x89\x05\x81\x9b\x87\x23\xfe\x2c\x1d\x16\x18\x60\xe5\xee\x18\x30\x31\x8d\xbf"
                    "\x49\xa8\x3b\xd4\x51\xcf\xb8\x44\x0c\x28\xbd\x6f\xa4\x57\xfe\x12\x96\x10\x65\x59\xa3\xc8\x09\x37"
                    "\xa1\xc1\x06\x9b\xe3\xa3\xa5\xbd\x38\x1e\xe6\x26\x0e\x8d\x97\x39\xfc\xe1\xf6\x07"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 2\n", __func__);

    UInt128 entropy3 = *(UInt128 *)"\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80";
    char phrase3[BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy3.u8, sizeof(entropy3))];
    size_t len3 = BRBIP39Encode(phrase3, sizeof(phrase3), BRBIP39WordsEn, entropy3.u8, sizeof(entropy3));
    
    if (strncmp(phrase3, "letter advice cage absurd amount doctor acoustic avoid letter advice cage above",
                len3)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Encode() test 3\n", __func__);
    
    BRBIP39Decode(entropy.u8, sizeof(entropy), BRBIP39WordsEn, phrase3);
    if (! UInt128Eq(entropy3, entropy)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Decode() test 3\n", __func__);
    
    BRBIP39DeriveKey(key.u8, phrase3, "TREZOR");
    if (! UInt512Eq(key, *(UInt512 *)"\xd7\x1d\xe8\x56\xf8\x1a\x8a\xcc\x65\xe6\xfc\x85\x1a\x38\xd4\xd7\xec\x21\x6f\xd0"
                    "\x79\x6d\x0a\x68\x27\xa3\xad\x6e\xd5\x51\x1a\x30\xfa\x28\x0f\x12\xeb\x2e\x47\xed\x2a\xc0\x3b\x5c"
                    "\x46\x2a\x03\x58\xd1\x8d\x69\xfe\x4f\x98\x5e\xc8\x17\x78\xc1\xb3\x70\xb6\x52\xa8"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 3\n", __func__);

    UInt128 entropy4 = *(UInt128 *)"\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff";
    char phrase4[BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy4.u8, sizeof(entropy4))];
    size_t len4 = BRBIP39Encode(phrase4, sizeof(phrase4), BRBIP39WordsEn, entropy4.u8, sizeof(entropy4));
    
    if (strncmp(phrase4, "zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo zoo wrong", len4))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Encode() test 4\n", __func__);
    
    BRBIP39Decode(entropy.u8, sizeof(entropy), BRBIP39WordsEn, phrase4);
    if (! UInt128Eq(entropy4, entropy)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Decode() test 4\n", __func__);
    
    BRBIP39DeriveKey(key.u8, phrase4, "TREZOR");
    if (! UInt512Eq(key, *(UInt512 *)"\xac\x27\x49\x54\x80\x22\x52\x22\x07\x9d\x7b\xe1\x81\x58\x37\x51\xe8\x6f\x57\x10"
                    "\x27\xb0\x49\x7b\x5b\x5d\x11\x21\x8e\x0a\x8a\x13\x33\x25\x72\x91\x7f\x0f\x8e\x5a\x58\x96\x20\xc6"
                    "\xf1\x5b\x11\xc6\x1d\xee\x32\x76\x51\xa1\x4c\x34\xe1\x82\x31\x05\x2e\x48\xc0\x69"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 4\n", __func__);

    UInt128 entropy5 = *(UInt128 *)"\x77\xc2\xb0\x07\x16\xce\xc7\x21\x38\x39\x15\x9e\x40\x4d\xb5\x0d";
    char phrase5[BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy5.u8, sizeof(entropy5))];
    size_t len5 = BRBIP39Encode(phrase5, sizeof(phrase5), BRBIP39WordsEn, entropy5.u8, sizeof(entropy5));
    
    if (strncmp(phrase5, "jelly better achieve collect unaware mountain thought cargo oxygen act hood bridge",
                len5)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Encode() test 5\n", __func__);
    
    BRBIP39Decode(entropy.u8, sizeof(entropy), BRBIP39WordsEn, phrase5);
    if (! UInt128Eq(entropy5, entropy)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Decode() test 5\n", __func__);
    
    BRBIP39DeriveKey(key.u8, phrase5, "TREZOR");
    if (! UInt512Eq(key, *(UInt512 *)"\xb5\xb6\xd0\x12\x7d\xb1\xa9\xd2\x22\x6a\xf0\xc3\x34\x60\x31\xd7\x7a\xf3\x1e\x91"
                    "\x8d\xba\x64\x28\x7a\x1b\x44\xb8\xeb\xf6\x3c\xdd\x52\x67\x6f\x67\x2a\x29\x0a\xae\x50\x24\x72\xcf"
                    "\x2d\x60\x2c\x05\x1f\x3e\x6f\x18\x05\x5e\x84\xe4\xc4\x38\x97\xfc\x4e\x51\xa6\xff"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 5\n", __func__);

    UInt128 entropy6 = *(UInt128 *)"\x04\x60\xef\x47\x58\x56\x04\xc5\x66\x06\x18\xdb\x2e\x6a\x7e\x7f";
    char phrase6[BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy6.u8, sizeof(entropy6))];
    size_t len6 = BRBIP39Encode(phrase6, sizeof(phrase6), BRBIP39WordsEn, entropy6.u8, sizeof(entropy6));
    
    if (strncmp(phrase6, "afford alter spike radar gate glance object seek swamp infant panel yellow", len6))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Encode() test 6\n", __func__);
    
    BRBIP39Decode(entropy.u8, sizeof(entropy), BRBIP39WordsEn, phrase6);
    if (! UInt128Eq(entropy6, entropy)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Decode() test 6\n", __func__);
    
    BRBIP39DeriveKey(key.u8, phrase6, "TREZOR");
    if (! UInt512Eq(key, *(UInt512 *)"\x65\xf9\x3a\x9f\x36\xb6\xc8\x5c\xbe\x63\x4f\xfc\x1f\x99\xf2\xb8\x2c\xbb\x10\xb3"
                    "\x1e\xdc\x7f\x08\x7b\x4f\x6c\xb9\xe9\x76\xe9\xfa\xf7\x6f\xf4\x1f\x8f\x27\xc9\x9a\xfd\xf3\x8f\x7a"
                    "\x30\x3b\xa1\x13\x6e\xe4\x8a\x4c\x1e\x7f\xcd\x3d\xba\x7a\xa8\x76\x11\x3a\x36\xe4"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 6\n", __func__);

    UInt128 entropy7 = *(UInt128 *)"\xea\xeb\xab\xb2\x38\x33\x51\xfd\x31\xd7\x03\x84\x0b\x32\xe9\xe2";
    char phrase7[BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy7.u8, sizeof(entropy7))];
    size_t len7 = BRBIP39Encode(phrase7, sizeof(phrase7), BRBIP39WordsEn, entropy7.u8, sizeof(entropy7));
    
    if (strncmp(phrase7, "turtle front uncle idea crush write shrug there lottery flower risk shell", len7))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Encode() test 7\n", __func__);
    
    BRBIP39Decode(entropy.u8, sizeof(entropy), BRBIP39WordsEn, phrase7);
    if (! UInt128Eq(entropy7, entropy)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Decode() test 7\n", __func__);
    
    BRBIP39DeriveKey(key.u8, phrase7, "TREZOR");
    if (! UInt512Eq(key, *(UInt512 *)"\xbd\xfb\x76\xa0\x75\x9f\x30\x1b\x0b\x89\x9a\x1e\x39\x85\x22\x7e\x53\xb3\xf5\x1e"
                    "\x67\xe3\xf2\xa6\x53\x63\xca\xed\xf3\xe3\x2f\xde\x42\xa6\x6c\x40\x4f\x18\xd7\xb0\x58\x18\xc9\x5e"
                    "\xf3\xca\x1e\x51\x46\x64\x68\x56\xc4\x61\xc0\x73\x16\x94\x67\x51\x16\x80\x87\x6c"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 7\n", __func__);

    UInt128 entropy8 = *(UInt128 *)"\x18\xab\x19\xa9\xf5\x4a\x92\x74\xf0\x3e\x52\x09\xa2\xac\x8a\x91";
    char phrase8[BRBIP39Encode(NULL, 0, BRBIP39WordsEn, entropy8.u8, sizeof(entropy8))];
    size_t len8 = BRBIP39Encode(phrase8, sizeof(phrase8), BRBIP39WordsEn, entropy8.u8, sizeof(entropy8));
    
    if (strncmp(phrase8, "board flee heavy tunnel powder denial science ski answer betray cargo cat", len8))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Encode() test 8\n", __func__);
    
    BRBIP39Decode(entropy.u8, sizeof(entropy), BRBIP39WordsEn, phrase8);
    if (! UInt128Eq(entropy8, entropy)) r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39Decode() test 8\n", __func__);
    
    BRBIP39DeriveKey(key.u8, phrase8, "TREZOR");
    if (! UInt512Eq(key, *(UInt512 *)"\x6e\xff\x1b\xb2\x15\x62\x91\x85\x09\xc7\x3c\xb9\x90\x26\x0d\xb0\x7c\x0c\xe3\x4f"
                    "\xf0\xe3\xcc\x4a\x8c\xb3\x27\x61\x29\xfb\xcb\x30\x0b\xdd\xfe\x00\x58\x31\x35\x0e\xfd\x63\x39\x09"
                    "\xf4\x76\xc4\x5c\x88\x25\x32\x76\xd9\xfd\x0d\xf6\xef\x48\x60\x9e\x8b\xb7\xdc\xa8"))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP39DeriveKey() test 8\n", __func__);

    return r;
}

int BRBIP32SequenceTests()
{
    int r = 1;

    UInt128 seed = *(UInt128 *)"\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F";
    BRKey key;

    printf("\n");

    BRBIP32PrivKey(&key, &seed, sizeof(seed), 1, 2 | 0x80000000);
    printf("000102030405060708090a0b0c0d0e0f/0H/1/2H prv = %s\n", uint256_hex_encode(key.secret));
    if (! UInt256Eq(key.secret, uint256_hex_decode("cbce0d719ecf7431d88e6a89fa1483e02e35092af60c042b1df2ff59fa424dca")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP32PrivKey() test 1\n", __func__);
    
    // test for correct zero padding of private keys
    BRBIP32PrivKey(&key, &seed, sizeof(seed), 0, 97);
    printf("000102030405060708090a0b0c0d0e0f/0H/0/97 prv = %s\n", uint256_hex_encode(key.secret));
    if (! UInt256Eq(key.secret, uint256_hex_decode("00136c1ad038f9a00871895322a487ed14f1cdc4d22ad351cfa1a0d235975dd7")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP32PrivKey() test 2\n", __func__);
    
    BRMasterPubKey mpk = BRBIP32MasterPubKey(&seed, sizeof(seed));
    
    printf("000102030405060708090a0b0c0d0e0f/0H fp:%08x chain:%s pubkey:%02x%s\n", be32(mpk.fingerPrint),
           uint256_hex_encode(mpk.chainCode), mpk.pubKey[0], uint256_hex_encode(*(UInt256 *)&mpk.pubKey[1]));
    if (be32(mpk.fingerPrint) != 0x3442193e ||
        ! UInt256Eq(mpk.chainCode,
                    uint256_hex_decode("47fdacbd0f1097043b78c63c20c34ef4ed9a111d980047ad16282c7ae6236141")) ||
        mpk.pubKey[0] != 0x03 ||
        ! UInt256Eq(*(UInt256 *)&mpk.pubKey[1],
                    uint256_hex_decode("5a784662a4a20a65bf6aab9ae98a6c068a81c52e4b032c0fb5400c706cfccc56")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP32MasterPubKey() test\n", __func__);

    uint8_t pubKey[33];

    BRBIP32PubKey(pubKey, sizeof(pubKey), mpk, 0, 0);
    printf("000102030405060708090a0b0c0d0e0f/0H/0/0 pub = %02x%s\n", pubKey[0],
           uint256_hex_encode(*(UInt256 *)&pubKey[1]));
    if (pubKey[0] != 0x02 ||
        ! UInt256Eq(*(UInt256 *)&pubKey[1],
                    uint256_hex_decode("7b6a7dd645507d775215a9035be06700e1ed8c541da9351b4bd14bd50ab61428")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBIP32PubKey() test\n", __func__);

    // TODO: XXX test BRBIP32SerializeMasterPrivKey()
    // TODO: XXX test BRBIP32SerializeMasterPubKey()

    printf("                          ");
    return r;
}

int BRTransactionTests()
{
    int r = 1;
    UInt256 secret = uint256_hex_decode("0000000000000000000000000000000000000000000000000000000000000001");
    BRKey k[2];
    BRAddress address;
    
    memset(&k[0], 0, sizeof(k[0])); // test with array of keys where first key is empty/invalid
    BRKeySetSecret(&k[1], &secret, 1);
    BRKeyAddress(&k[1], address.s, sizeof(address));

    uint8_t script[BRAddressScriptPubKey(NULL, 0, address.s)];
    size_t scriptLen = BRAddressScriptPubKey(script, sizeof(script), address.s);
    BRTransaction *tx = BRTransactionNew();
    
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddOutput(tx, 100000000, script, scriptLen);
    BRTransactionAddOutput(tx, 4900000000, script, scriptLen);
    BRTransactionSign(tx, k, 2);
    if (! BRTransactionIsSigned(tx))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRTransactionSign() test 1\n", __func__);

    uint8_t buf[BRTransactionSerialize(tx, NULL, 0)];
    size_t len = BRTransactionSerialize(tx, buf, sizeof(buf));

    if (tx) BRTransactionFree(tx);
    tx = BRTransactionParse(buf, len);

    if (! BRTransactionIsSigned(tx))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRTransactionParse() test 1\n", __func__);

    uint8_t buf2[BRTransactionSerialize(tx, NULL, 0)];
    size_t len2 = BRTransactionSerialize(tx, buf2, sizeof(buf2));
    
    if (len != len2 || memcmp(buf, buf2, len) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRTransactionSerialize() test 1\n", __func__);
    if (tx) BRTransactionFree(tx);
    
    tx = BRTransactionNew();
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddInput(tx, UINT256_ZERO, 0, script, scriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionAddOutput(tx, 1000000, script, scriptLen);
    BRTransactionSign(tx, k, 2);
    if (! BRTransactionIsSigned(tx))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRTransactionSign() test 2\n", __func__);

    uint8_t buf3[BRTransactionSerialize(tx, NULL, 0)];
    size_t len3 = BRTransactionSerialize(tx, buf3, sizeof(buf3));
    
    if (tx) BRTransactionFree(tx);
    tx = BRTransactionParse(buf3, len3);
    if (! BRTransactionIsSigned(tx))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRTransactionParse() test 2\n", __func__);

    uint8_t buf4[BRTransactionSerialize(tx, NULL, 0)];
    size_t len4 = BRTransactionSerialize(tx, buf4, sizeof(buf4));
    
    if (len3 != len4 || memcmp(buf3, buf4, len3) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRTransactionSerialize() test 2\n", __func__);
    if (tx) BRTransactionFree(tx);
    
    return r;
}

static void walletBalanceChanged(void *info, uint64_t balance)
{
    printf("balance changed %"PRIu64"\n", balance);
}

static void walletTxAdded(void *info, BRTransaction *tx)
{
    printf("tx added: %s\n", uint256_hex_encode(tx->txHash));
}

static void walletTxUpdated(void *info, const UInt256 txHash[], size_t count, uint32_t blockHeight, uint32_t timestamp)
{
    for (size_t i = 0; i < count; i++) printf("tx updated: %s\n", uint256_hex_encode(txHash[i]));
}

static void walletTxDeleted(void *info, UInt256 txHash)
{
    printf("tx deleted: %s\n", uint256_hex_encode(txHash));
}

// TODO: test standard free transaction no change
// TODO: test free transaction who's inputs are too new to hit min free priority
// TODO: test transaction with change below min allowable output
// TODO: test gap limit with gaps in address chain less than the limit
// TODO: test removing a transaction that other transansactions depend on
// TODO: test tx ordering for multiple tx with same block height
// TODO: port all applicable tests from bitcoinj and bitcoincore

int BRWalletTests()
{
    int r = 1;
    BRMasterPubKey mpk = BRBIP32MasterPubKey("", 0);
    BRWallet *w = BRWalletNew(NULL, 0, mpk);
    const UInt256 secret = uint256_hex_decode("0000000000000000000000000000000000000000000000000000000000000001");
    BRKey k;
    BRAddress addr, recvAddr = BRWalletReceiveAddress(w);
    BRTransaction *tx;
    
    printf("\n");
    
    BRWalletSetCallbacks(w, w, walletBalanceChanged, walletTxAdded, walletTxUpdated, walletTxDeleted);
    BRKeySetSecret(&k, &secret, 1);
    BRKeyAddress(&k, addr.s, sizeof(addr));
    
    tx = BRWalletCreateTransaction(w, 0, addr.s);
    if (tx) r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletCreateTransaction() test 0\n", __func__);
    
    tx = BRWalletCreateTransaction(w, SATOSHIS, addr.s);
    if (tx) r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletCreateTransaction() test 1\n", __func__);
    
    uint8_t inScript[BRAddressScriptPubKey(NULL, 0, addr.s)];
    size_t inScriptLen = BRAddressScriptPubKey(inScript, sizeof(inScript), addr.s);
    uint8_t outScript[BRAddressScriptPubKey(NULL, 0, recvAddr.s)];
    size_t outScriptLen = BRAddressScriptPubKey(outScript, sizeof(outScript), recvAddr.s);
    
    tx = BRTransactionNew();
    BRTransactionAddInput(tx, UINT256_ZERO, 0, inScript, inScriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddOutput(tx, SATOSHIS, outScript, outScriptLen);
    BRWalletRegisterTransaction(w, tx); // test adding unsigned tx
    if (BRWalletBalance(w) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletRegisterTransaction() test 1\n", __func__);

    if (BRWalletTransactions(w, NULL, 0) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletTransactions() test 1\n", __func__);

    BRTransactionSign(tx, &k, 1);
    BRWalletRegisterTransaction(w, tx);
    if (BRWalletBalance(w) != SATOSHIS)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletRegisterTransaction() test 2\n", __func__);

    if (BRWalletTransactions(w, NULL, 0) != 1)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletTransactions() test 2\n", __func__);

    BRWalletRegisterTransaction(w, tx); // test adding same tx twice
    if (BRWalletBalance(w) != SATOSHIS)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletRegisterTransaction() test 3\n", __func__);

    BRWalletFree(w);
    tx = BRTransactionNew();
    BRTransactionAddInput(tx, UINT256_ZERO, 0, inScript, inScriptLen, NULL, 0, TXIN_SEQUENCE);
    BRTransactionAddOutput(tx, SATOSHIS, outScript, outScriptLen);
    BRTransactionSign(tx, &k, 1);
    tx->timestamp = 1;
    w = BRWalletNew(&tx, 1, mpk);
    if (BRWalletBalance(w) != SATOSHIS)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletNew() test\n", __func__);

    UInt256 hash = tx->txHash;

    tx = BRWalletCreateTransaction(w, SATOSHIS*2, addr.s);
    if (tx) r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletCreateTransaction() test 3\n", __func__);
    
    tx = BRWalletCreateTransaction(w, SATOSHIS/2, addr.s);
    if (! tx) r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletCreateTransaction() test 4\n", __func__);

    if (tx) BRWalletSignTransaction(w, tx, "", 0);
    if (tx && ! BRTransactionIsSigned(tx))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletSignTransaction() test\n", __func__);
    
    if (tx) tx->timestamp = 1, BRWalletRegisterTransaction(w, tx);
    if (tx && BRWalletBalance(w) + BRWalletFeeForTx(w, tx) != SATOSHIS/2)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletRegisterTransaction() test 4\n", __func__);
    
    if (BRWalletTransactions(w, NULL, 0) != 2)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletTransactions() test 3\n", __func__);
    
    if (tx && BRWalletTransactionForHash(w, tx->txHash) != tx)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletTransactionForHash() test\n", __func__);

    if (tx && ! BRWalletTransactionIsValid(w, tx))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletTransactionIsValid() test\n", __func__);

    if (tx && ! BRWalletTransactionIsVerified(w, tx))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletTransactionIsVerified() test\n", __func__);

    if (tx && BRWalletTransactionIsPostdated(w, tx, 0))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletTransactionIsPostdated() test\n", __func__);
    
    BRWalletRemoveTransaction(w, hash); // removing first tx should recursively remove second, leaving none
    if (BRWalletTransactions(w, NULL, 0) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletRemoveTransaction() test\n", __func__);

    if (! BRAddressEq(BRWalletReceiveAddress(w).s, recvAddr.s)) // verify used addresses are correctly tracked
        r = 0, fprintf(stderr, "***FAILED*** %s: BRWalletReceiveAddress() test\n", __func__);
    
    printf("                          ");
    BRWalletFree(w);
    
    int64_t amt;
    
    amt = BRBitcoinAmount(50000, 50000);
    if (amt != SATOSHIS) r = 0, fprintf(stderr, "***FAILED*** %s: BRBitcoinAmount() test 1\n", __func__);

    amt = BRBitcoinAmount(-50000, 50000);
    if (amt != -SATOSHIS) r = 0, fprintf(stderr, "***FAILED*** %s: BRBitcoinAmount() test 2\n", __func__);
    
    amt = BRLocalAmount(SATOSHIS, 50000);
    if (amt != 50000) r = 0, fprintf(stderr, "***FAILED*** %s: BRLocalAmount() test 1\n", __func__);

    amt = BRLocalAmount(-SATOSHIS, 50000);
    if (amt != -50000) r = 0, fprintf(stderr, "***FAILED*** %s: BRLocalAmount() test 2\n", __func__);
    
    return r;
}

int BRBloomFilterTests()
{
    int r = 1;
    BRBloomFilter *f = BRBloomFilterNew(0.01, 3, 0, BLOOM_UPDATE_ALL);
    char data1[] = "\x99\x10\x8a\xd8\xed\x9b\xb6\x27\x4d\x39\x80\xba\xb5\xa8\x5c\x04\x8f\x09\x50\xc8";

    BRBloomFilterInsertData(f, (uint8_t *)data1, sizeof(data1) - 1);
    if (! BRBloomFilterContainsData(f, (uint8_t *)data1, sizeof(data1) - 1))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterContainsData() test 1\n", __func__);

    // one bit difference
    char data2[] = "\x19\x10\x8a\xd8\xed\x9b\xb6\x27\x4d\x39\x80\xba\xb5\xa8\x5c\x04\x8f\x09\x50\xc8";
    
    if (BRBloomFilterContainsData(f, (uint8_t *)data2, sizeof(data2) - 1))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterContainsData() test 2\n", __func__);
    
    char data3[] = "\xb5\xa2\xc7\x86\xd9\xef\x46\x58\x28\x7c\xed\x59\x14\xb3\x7a\x1b\x4a\xa3\x2e\xee";

    BRBloomFilterInsertData(f, (uint8_t *)data3, sizeof(data3) - 1);
    if (! BRBloomFilterContainsData(f, (uint8_t *)data3, sizeof(data3) - 1))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterContainsData() test 3\n", __func__);

    char data4[] = "\xb9\x30\x06\x70\xb4\xc5\x36\x6e\x95\xb2\x69\x9e\x8b\x18\xbc\x75\xe5\xf7\x29\xc5";
    
    BRBloomFilterInsertData(f, (uint8_t *)data4, sizeof(data4) - 1);
    if (! BRBloomFilterContainsData(f, (uint8_t *)data4, sizeof(data4) - 1))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterContainsData() test 4\n", __func__);

    // check against satoshi client output
    uint8_t buf1[BRBloomFilterSerialize(f, NULL, 0)];
    size_t len1 = BRBloomFilterSerialize(f, buf1, sizeof(buf1));
    char d1[] = "\x03\x61\x4e\x9b\x05\x00\x00\x00\x00\x00\x00\x00\x01";
    
    if (len1 != sizeof(d1) - 1 || memcmp(buf1, d1, len1) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterSerialize() test 1\n", __func__);
    
    BRBloomFilterFree(f);
    f = BRBloomFilterNew(0.01, 3, 2147483649, BLOOM_UPDATE_P2PUBKEY_ONLY);

    char data5[] = "\x99\x10\x8a\xd8\xed\x9b\xb6\x27\x4d\x39\x80\xba\xb5\xa8\x5c\x04\x8f\x09\x50\xc8";
    
    BRBloomFilterInsertData(f, (uint8_t *)data5, sizeof(data5) - 1);
    if (! BRBloomFilterContainsData(f, (uint8_t *)data5, sizeof(data5) - 1))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterContainsData() test 5\n", __func__);

    // one bit difference
    char data6[] = "\x19\x10\x8a\xd8\xed\x9b\xb6\x27\x4d\x39\x80\xba\xb5\xa8\x5c\x04\x8f\x09\x50\xc8";
    
    if (BRBloomFilterContainsData(f, (uint8_t *)data6, sizeof(data6) - 1))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterContainsData() test 6\n", __func__);

    char data7[] = "\xb5\xa2\xc7\x86\xd9\xef\x46\x58\x28\x7c\xed\x59\x14\xb3\x7a\x1b\x4a\xa3\x2e\xee";
    
    BRBloomFilterInsertData(f, (uint8_t *)data7, sizeof(data7) - 1);
    if (! BRBloomFilterContainsData(f, (uint8_t *)data7, sizeof(data7) - 1))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterContainsData() test 7\n", __func__);

    char data8[] = "\xb9\x30\x06\x70\xb4\xc5\x36\x6e\x95\xb2\x69\x9e\x8b\x18\xbc\x75\xe5\xf7\x29\xc5";
    
    BRBloomFilterInsertData(f, (uint8_t *)data8, sizeof(data8) - 1);
    if (! BRBloomFilterContainsData(f, (uint8_t *)data8, sizeof(data8) - 1))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterContainsData() test 8\n", __func__);

    // check against satoshi client output
    uint8_t buf2[BRBloomFilterSerialize(f, NULL, 0)];
    size_t len2 = BRBloomFilterSerialize(f, buf2, sizeof(buf2));
    char d2[] = "\x03\xce\x42\x99\x05\x00\x00\x00\x01\x00\x00\x80\x02";
    
    if (len2 != sizeof(d2) - 1 || memcmp(buf2, d2, len2) != 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRBloomFilterSerialize() test 2\n", __func__);
    
    BRBloomFilterFree(f);    
    return r;
}

int BRMerkleBlockTests()
{
    int r = 1;
    char block[] = // block 10001 filtered to include only transactions 0, 1, 2, and 6
    "\x01\x00\x00\x00\x06\xe5\x33\xfd\x1a\xda\x86\x39\x1f\x3f\x6c\x34\x32\x04\xb0\xd2\x78\xd4\xaa\xec\x1c"
    "\x0b\x20\xaa\x27\xba\x03\x00\x00\x00\x00\x00\x6a\xbb\xb3\xeb\x3d\x73\x3a\x9f\xe1\x89\x67\xfd\x7d\x4c\x11\x7e\x4c"
    "\xcb\xba\xc5\xbe\xc4\xd9\x10\xd9\x00\xb3\xae\x07\x93\xe7\x7f\x54\x24\x1b\x4d\x4c\x86\x04\x1b\x40\x89\xcc\x9b\x0c"
    "\x00\x00\x00\x08\x4c\x30\xb6\x3c\xfc\xdc\x2d\x35\xe3\x32\x94\x21\xb9\x80\x5e\xf0\xc6\x56\x5d\x35\x38\x1c\xa8\x57"
    "\x76\x2e\xa0\xb3\xa5\xa1\x28\xbb\xca\x50\x65\xff\x96\x17\xcb\xcb\xa4\x5e\xb2\x37\x26\xdf\x64\x98\xa9\xb9\xca\xfe"
    "\xd4\xf5\x4c\xba\xb9\xd2\x27\xb0\x03\x5d\xde\xfb\xbb\x15\xac\x1d\x57\xd0\x18\x2a\xae\xe6\x1c\x74\x74\x3a\x9c\x4f"
    "\x78\x58\x95\xe5\x63\x90\x9b\xaf\xec\x45\xc9\xa2\xb0\xff\x31\x81\xd7\x77\x06\xbe\x8b\x1d\xcc\x91\x11\x2e\xad\xa8"
    "\x6d\x42\x4e\x2d\x0a\x89\x07\xc3\x48\x8b\x6e\x44\xfd\xa5\xa7\x4a\x25\xcb\xc7\xd6\xbb\x4f\xa0\x42\x45\xf4\xac\x8a"
    "\x1a\x57\x1d\x55\x37\xea\xc2\x4a\xdc\xa1\x45\x4d\x65\xed\xa4\x46\x05\x54\x79\xaf\x6c\x6d\x4d\xd3\xc9\xab\x65\x84"
    "\x48\xc1\x0b\x69\x21\xb7\xa4\xce\x30\x21\xeb\x22\xed\x6b\xb6\xa7\xfd\xe1\xe5\xbc\xc4\xb1\xdb\x66\x15\xc6\xab\xc5"
    "\xca\x04\x21\x27\xbf\xaf\x9f\x44\xeb\xce\x29\xcb\x29\xc6\xdf\x9d\x05\xb4\x7f\x35\xb2\xed\xff\x4f\x00\x64\xb5\x78"
    "\xab\x74\x1f\xa7\x82\x76\x22\x26\x51\x20\x9f\xe1\xa2\xc4\xc0\xfa\x1c\x58\x51\x0a\xec\x8b\x09\x0d\xd1\xeb\x1f\x82"
    "\xf9\xd2\x61\xb8\x27\x3b\x52\x5b\x02\xff\x1a";
    BRMerkleBlock *b;
    
    b = BRMerkleBlockParse((uint8_t *)block, sizeof(block) - 1);
    
    if (! UInt256Eq(b->blockHash,
        UInt256Reverse(uint256_hex_decode("00000000000080b66c911bd5ba14a74260057311eaeb1982802f7010f1a9f090")))) r = 0;

    if (! BRMerkleBlockIsValid(b, (uint32_t)time(NULL)))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRMerkleBlockParse() test\n", __func__);
    
    if (! BRMerkleBlockContainsTxHash(b,
        uint256_hex_decode("4c30b63cfcdc2d35e3329421b9805ef0c6565d35381ca857762ea0b3a5a128bb")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRMerkleBlockContainsTxHash() test\n", __func__);
    
    if (BRMerkleBlockTxHashes(b, NULL, 0) != 4)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRMerkleBlockTxHashes() test 0\n", __func__);
    
    UInt256 txHashes[BRMerkleBlockTxHashes(b, NULL, 0)];
    
    BRMerkleBlockTxHashes(b, txHashes, 4);
    
    if (! UInt256Eq(txHashes[0],
                    uint256_hex_decode("4c30b63cfcdc2d35e3329421b9805ef0c6565d35381ca857762ea0b3a5a128bb")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRMerkleBlockTxHashes() test 1\n", __func__);
    
    if (! UInt256Eq(txHashes[1],
                    uint256_hex_decode("ca5065ff9617cbcba45eb23726df6498a9b9cafed4f54cbab9d227b0035ddefb")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRMerkleBlockTxHashes() test 2\n", __func__);
    
    if (! UInt256Eq(txHashes[2],
                    uint256_hex_decode("bb15ac1d57d0182aaee61c74743a9c4f785895e563909bafec45c9a2b0ff3181")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRMerkleBlockTxHashes() test 3\n", __func__);
    
    if (! UInt256Eq(txHashes[3],
                    uint256_hex_decode("c9ab658448c10b6921b7a4ce3021eb22ed6bb6a7fde1e5bcc4b1db6615c6abc5")))
        r = 0, fprintf(stderr, "***FAILED*** %s: BRMerkleBlockTxHashes() test 4\n", __func__);
    
    // TODO: test a block with an odd number of tree rows both at the tx level and merkle node level

    // TODO: XXX test BRMerkleBlockVerifyDifficulty()
    
    if (b) BRMerkleBlockFree(b);
    return r;
}

int BRPaymentProtocolTests()
{
    int r = 1;
    const char buf1[] = "\x08\x01\x12\x0b\x78\x35\x30\x39\x2b\x73\x68\x61\x32\x35\x36\x1a\xb8\x1d\x0a\xc9\x0b\x30\x82"
    "\x05\xc5\x30\x82\x04\xad\xa0\x03\x02\x01\x02\x02\x07\x2b\x85\x8c\x53\xee\xed\x2f\x30\x0d\x06\x09\x2a\x86\x48\x86"
    "\xf7\x0d\x01\x01\x05\x05\x00\x30\x81\xca\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x10\x30\x0e\x06"
    "\x03\x55\x04\x08\x13\x07\x41\x72\x69\x7a\x6f\x6e\x61\x31\x13\x30\x11\x06\x03\x55\x04\x07\x13\x0a\x53\x63\x6f\x74"
    "\x74\x73\x64\x61\x6c\x65\x31\x1a\x30\x18\x06\x03\x55\x04\x0a\x13\x11\x47\x6f\x44\x61\x64\x64\x79\x2e\x63\x6f\x6d"
    "\x2c\x20\x49\x6e\x63\x2e\x31\x33\x30\x31\x06\x03\x55\x04\x0b\x13\x2a\x68\x74\x74\x70\x3a\x2f\x2f\x63\x65\x72\x74"
    "\x69\x66\x69\x63\x61\x74\x65\x73\x2e\x67\x6f\x64\x61\x64\x64\x79\x2e\x63\x6f\x6d\x2f\x72\x65\x70\x6f\x73\x69\x74"
    "\x6f\x72\x79\x31\x30\x30\x2e\x06\x03\x55\x04\x03\x13\x27\x47\x6f\x20\x44\x61\x64\x64\x79\x20\x53\x65\x63\x75\x72"
    "\x65\x20\x43\x65\x72\x74\x69\x66\x69\x63\x61\x74\x69\x6f\x6e\x20\x41\x75\x74\x68\x6f\x72\x69\x74\x79\x31\x11\x30"
    "\x0f\x06\x03\x55\x04\x05\x13\x08\x30\x37\x39\x36\x39\x32\x38\x37\x30\x1e\x17\x0d\x31\x33\x30\x34\x32\x35\x31\x39"
    "\x31\x31\x30\x30\x5a\x17\x0d\x31\x35\x30\x34\x32\x35\x31\x39\x31\x31\x30\x30\x5a\x30\x81\xbe\x31\x13\x30\x11\x06"
    "\x0b\x2b\x06\x01\x04\x01\x82\x37\x3c\x02\x01\x03\x13\x02\x55\x53\x31\x19\x30\x17\x06\x0b\x2b\x06\x01\x04\x01\x82"
    "\x37\x3c\x02\x01\x02\x13\x08\x44\x65\x6c\x61\x77\x61\x72\x65\x31\x1d\x30\x1b\x06\x03\x55\x04\x0f\x13\x14\x50\x72"
    "\x69\x76\x61\x74\x65\x20\x4f\x72\x67\x61\x6e\x69\x7a\x61\x74\x69\x6f\x6e\x31\x10\x30\x0e\x06\x03\x55\x04\x05\x13"
    "\x07\x35\x31\x36\x33\x39\x36\x36\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x10\x30\x0e\x06\x03\x55"
    "\x04\x08\x13\x07\x47\x65\x6f\x72\x67\x69\x61\x31\x10\x30\x0e\x06\x03\x55\x04\x07\x13\x07\x41\x74\x6c\x61\x6e\x74"
    "\x61\x31\x15\x30\x13\x06\x03\x55\x04\x0a\x13\x0c\x42\x69\x74\x50\x61\x79\x2c\x20\x49\x6e\x63\x2e\x31\x13\x30\x11"
    "\x06\x03\x55\x04\x03\x13\x0a\x62\x69\x74\x70\x61\x79\x2e\x63\x6f\x6d\x30\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48"
    "\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01\x00\xc4\x6e\xef\xc2\x8b\x15"
    "\x7d\x03\x71\x7f\x0c\x00\xa1\xd6\x7b\xa7\x61\x2c\x1f\x2b\x56\x21\x82\xce\x99\x60\x2c\x47\x68\xff\x8f\xbd\x10\x66"
    "\x85\xd9\x39\x26\x32\x66\xbb\x9e\x10\x7d\x05\x7d\xb8\x44\x50\x2d\x8e\xc6\x1e\x88\x7e\xa5\x5b\x55\xc2\xc1\x71\x21"
    "\x89\x64\x54\xa3\x19\xf6\x5b\x3d\xb3\x4c\x86\x29\xa7\x5b\x3e\x12\x3f\xe2\x07\x6d\x85\xcf\x4f\x64\x4a\xe3\xf6\xfb"
    "\x84\x29\xc5\xa7\x83\x0d\xf4\x65\x85\x9c\x4d\x6c\x0b\xcd\xbc\x12\x86\x5f\xab\x22\x18\xbd\x65\xf2\xb2\x53\x00\x12"
    "\xce\x49\x96\x98\xcc\xae\x02\x59\xac\x0b\x34\x70\xa8\x56\x6b\x70\x5e\x1a\x66\x1a\xd8\x28\x64\x29\xac\xf0\xb3\x13"
    "\x6e\x4c\xdf\x4d\x91\x19\x08\x4a\x5b\x6e\xcf\x19\x76\x94\xc2\xb5\x57\x82\x70\x12\x11\xca\x28\xda\xfa\x6d\x96\xac"
    "\xec\xc2\x23\x2a\xc5\xe9\xa8\x61\x81\xd4\xf7\x41\x7f\xd8\xd9\x38\x50\x7f\x6d\x0c\x62\x52\x94\x02\x16\x30\x09\x46"
    "\xf7\x62\x70\x13\xd7\x49\x98\xe0\x92\x2d\x4b\x9c\x97\xa7\x77\x9b\x1d\x56\xf3\x0c\x07\xd0\x26\x9b\x15\x89\xbd\x60"
    "\x4d\x38\x4a\x52\x37\x21\x3c\x75\xd0\xc6\xbf\x81\x1b\xce\x8c\xdb\xbb\x06\xc1\xa2\xc6\xe4\x79\xd2\x71\xfd\x02\x03"
    "\x01\x00\x01\xa3\x82\x01\xb8\x30\x82\x01\xb4\x30\x0f\x06\x03\x55\x1d\x13\x01\x01\xff\x04\x05\x30\x03\x01\x01\x00"
    "\x30\x1d\x06\x03\x55\x1d\x25\x04\x16\x30\x14\x06\x08\x2b\x06\x01\x05\x05\x07\x03\x01\x06\x08\x2b\x06\x01\x05\x05"
    "\x07\x03\x02\x30\x0e\x06\x03\x55\x1d\x0f\x01\x01\xff\x04\x04\x03\x02\x05\xa0\x30\x33\x06\x03\x55\x1d\x1f\x04\x2c"
    "\x30\x2a\x30\x28\xa0\x26\xa0\x24\x86\x22\x68\x74\x74\x70\x3a\x2f\x2f\x63\x72\x6c\x2e\x67\x6f\x64\x61\x64\x64\x79"
    "\x2e\x63\x6f\x6d\x2f\x67\x64\x73\x33\x2d\x37\x32\x2e\x63\x72\x6c\x30\x53\x06\x03\x55\x1d\x20\x04\x4c\x30\x4a\x30"
    "\x48\x06\x0b\x60\x86\x48\x01\x86\xfd\x6d\x01\x07\x17\x03\x30\x39\x30\x37\x06\x08\x2b\x06\x01\x05\x05\x07\x02\x01"
    "\x16\x2b\x68\x74\x74\x70\x3a\x2f\x2f\x63\x65\x72\x74\x69\x66\x69\x63\x61\x74\x65\x73\x2e\x67\x6f\x64\x61\x64\x64"
    "\x79\x2e\x63\x6f\x6d\x2f\x72\x65\x70\x6f\x73\x69\x74\x6f\x72\x79\x2f\x30\x81\x80\x06\x08\x2b\x06\x01\x05\x05\x07"
    "\x01\x01\x04\x74\x30\x72\x30\x24\x06\x08\x2b\x06\x01\x05\x05\x07\x30\x01\x86\x18\x68\x74\x74\x70\x3a\x2f\x2f\x6f"
    "\x63\x73\x70\x2e\x67\x6f\x64\x61\x64\x64\x79\x2e\x63\x6f\x6d\x2f\x30\x4a\x06\x08\x2b\x06\x01\x05\x05\x07\x30\x02"
    "\x86\x3e\x68\x74\x74\x70\x3a\x2f\x2f\x63\x65\x72\x74\x69\x66\x69\x63\x61\x74\x65\x73\x2e\x67\x6f\x64\x61\x64\x64"
    "\x79\x2e\x63\x6f\x6d\x2f\x72\x65\x70\x6f\x73\x69\x74\x6f\x72\x79\x2f\x67\x64\x5f\x69\x6e\x74\x65\x72\x6d\x65\x64"
    "\x69\x61\x74\x65\x2e\x63\x72\x74\x30\x1f\x06\x03\x55\x1d\x23\x04\x18\x30\x16\x80\x14\xfd\xac\x61\x32\x93\x6c\x45"
    "\xd6\xe2\xee\x85\x5f\x9a\xba\xe7\x76\x99\x68\xcc\xe7\x30\x25\x06\x03\x55\x1d\x11\x04\x1e\x30\x1c\x82\x0a\x62\x69"
    "\x74\x70\x61\x79\x2e\x63\x6f\x6d\x82\x0e\x77\x77\x77\x2e\x62\x69\x74\x70\x61\x79\x2e\x63\x6f\x6d\x30\x1d\x06\x03"
    "\x55\x1d\x0e\x04\x16\x04\x14\xb9\x41\x17\x56\x7a\xe7\xc3\xef\x50\x72\x82\xac\xc4\xd5\x51\xc6\xbf\x7f\xa4\x4a\x30"
    "\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05\x05\x00\x03\x82\x01\x01\x00\xb8\xd5\xac\xa9\x63\xa6\xf9\xa0\xb5"
    "\xc5\xaf\x03\x4a\xcc\x83\x2a\x13\xf1\xbb\xeb\x93\x2d\x39\x7a\x7d\x4b\xd3\xa4\x5e\x6a\x3d\x6d\xb3\x10\x9a\x23\x54"
    "\xa8\x08\x14\xee\x3e\x6c\x7c\xef\xf5\xd7\xf4\xa9\x83\xdb\xde\x55\xf0\x96\xba\x99\x2d\x0f\xff\x4f\xe1\xa9\x2e\xaa"
    "\xb7\x9b\xd1\x47\xb3\x52\x1e\xe3\x61\x2c\xee\x2c\xf7\x59\x5b\xc6\x35\xa1\xfe\xef\xc6\xdb\x5c\x58\x3a\x59\x23\xc7"
    "\x1c\x86\x4d\xda\xcb\xcf\xf4\x63\xe9\x96\x7f\x4c\x02\xbd\xd7\x72\x71\x63\x55\x75\x96\x7e\xc2\x3e\x8b\x6c\xdb\xda"
    "\xb6\x32\xce\x79\x07\x2f\x47\x70\x4a\x6e\xf1\xf1\x60\x31\x08\x37\xde\x45\x6e\x4a\x01\xa2\x2b\xbf\x89\xd8\xe0\xf5"
    "\x26\x7d\xfb\x71\x99\x8a\xde\x3e\xa2\x60\xdc\x9b\xc6\xcf\xf3\x89\x9a\x88\xca\xf6\xa5\xe0\xea\x74\x97\xff\xbc\x42"
    "\xed\x4f\xa6\x95\x51\xe5\xe0\xb2\x15\x6e\x9e\x2d\x22\x5b\xa7\xa5\xe5\x6d\xe5\xff\x13\x0a\x4c\x6e\x5f\x1a\x99\x68"
    "\x68\x7b\x82\x62\x0f\x86\x17\x02\xd5\x6c\x44\x29\x79\x9f\xff\x9d\xb2\x56\x2b\xc2\xdc\xe9\x7f\xe7\xe3\x4a\x1f\xab"
    "\xb0\x39\xe5\xe7\x8b\xd4\xda\xe6\x0f\x58\x68\xa5\xe8\xa3\xf8\xc3\x30\xe3\x7f\x38\xfb\xfe\x1f\x0a\xe2\x09\x30\x82"
    "\x04\xde\x30\x82\x03\xc6\xa0\x03\x02\x01\x02\x02\x02\x03\x01\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05"
    "\x05\x00\x30\x63\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x21\x30\x1f\x06\x03\x55\x04\x0a\x13\x18"
    "\x54\x68\x65\x20\x47\x6f\x20\x44\x61\x64\x64\x79\x20\x47\x72\x6f\x75\x70\x2c\x20\x49\x6e\x63\x2e\x31\x31\x30\x2f"
    "\x06\x03\x55\x04\x0b\x13\x28\x47\x6f\x20\x44\x61\x64\x64\x79\x20\x43\x6c\x61\x73\x73\x20\x32\x20\x43\x65\x72\x74"
    "\x69\x66\x69\x63\x61\x74\x69\x6f\x6e\x20\x41\x75\x74\x68\x6f\x72\x69\x74\x79\x30\x1e\x17\x0d\x30\x36\x31\x31\x31"
    "\x36\x30\x31\x35\x34\x33\x37\x5a\x17\x0d\x32\x36\x31\x31\x31\x36\x30\x31\x35\x34\x33\x37\x5a\x30\x81\xca\x31\x0b"
    "\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x10\x30\x0e\x06\x03\x55\x04\x08\x13\x07\x41\x72\x69\x7a\x6f\x6e"
    "\x61\x31\x13\x30\x11\x06\x03\x55\x04\x07\x13\x0a\x53\x63\x6f\x74\x74\x73\x64\x61\x6c\x65\x31\x1a\x30\x18\x06\x03"
    "\x55\x04\x0a\x13\x11\x47\x6f\x44\x61\x64\x64\x79\x2e\x63\x6f\x6d\x2c\x20\x49\x6e\x63\x2e\x31\x33\x30\x31\x06\x03"
    "\x55\x04\x0b\x13\x2a\x68\x74\x74\x70\x3a\x2f\x2f\x63\x65\x72\x74\x69\x66\x69\x63\x61\x74\x65\x73\x2e\x67\x6f\x64"
    "\x61\x64\x64\x79\x2e\x63\x6f\x6d\x2f\x72\x65\x70\x6f\x73\x69\x74\x6f\x72\x79\x31\x30\x30\x2e\x06\x03\x55\x04\x03"
    "\x13\x27\x47\x6f\x20\x44\x61\x64\x64\x79\x20\x53\x65\x63\x75\x72\x65\x20\x43\x65\x72\x74\x69\x66\x69\x63\x61\x74"
    "\x69\x6f\x6e\x20\x41\x75\x74\x68\x6f\x72\x69\x74\x79\x31\x11\x30\x0f\x06\x03\x55\x04\x05\x13\x08\x30\x37\x39\x36"
    "\x39\x32\x38\x37\x30\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x01\x0f\x00"
    "\x30\x82\x01\x0a\x02\x82\x01\x01\x00\xc4\x2d\xd5\x15\x8c\x9c\x26\x4c\xec\x32\x35\xeb\x5f\xb8\x59\x01\x5a\xa6\x61"
    "\x81\x59\x3b\x70\x63\xab\xe3\xdc\x3d\xc7\x2a\xb8\xc9\x33\xd3\x79\xe4\x3a\xed\x3c\x30\x23\x84\x8e\xb3\x30\x14\xb6"
    "\xb2\x87\xc3\x3d\x95\x54\x04\x9e\xdf\x99\xdd\x0b\x25\x1e\x21\xde\x65\x29\x7e\x35\xa8\xa9\x54\xeb\xf6\xf7\x32\x39"
    "\xd4\x26\x55\x95\xad\xef\xfb\xfe\x58\x86\xd7\x9e\xf4\x00\x8d\x8c\x2a\x0c\xbd\x42\x04\xce\xa7\x3f\x04\xf6\xee\x80"
    "\xf2\xaa\xef\x52\xa1\x69\x66\xda\xbe\x1a\xad\x5d\xda\x2c\x66\xea\x1a\x6b\xbb\xe5\x1a\x51\x4a\x00\x2f\x48\xc7\x98"
    "\x75\xd8\xb9\x29\xc8\xee\xf8\x66\x6d\x0a\x9c\xb3\xf3\xfc\x78\x7c\xa2\xf8\xa3\xf2\xb5\xc3\xf3\xb9\x7a\x91\xc1\xa7"
    "\xe6\x25\x2e\x9c\xa8\xed\x12\x65\x6e\x6a\xf6\x12\x44\x53\x70\x30\x95\xc3\x9c\x2b\x58\x2b\x3d\x08\x74\x4a\xf2\xbe"
    "\x51\xb0\xbf\x87\xd0\x4c\x27\x58\x6b\xb5\x35\xc5\x9d\xaf\x17\x31\xf8\x0b\x8f\xee\xad\x81\x36\x05\x89\x08\x98\xcf"
    "\x3a\xaf\x25\x87\xc0\x49\xea\xa7\xfd\x67\xf7\x45\x8e\x97\xcc\x14\x39\xe2\x36\x85\xb5\x7e\x1a\x37\xfd\x16\xf6\x71"
    "\x11\x9a\x74\x30\x16\xfe\x13\x94\xa3\x3f\x84\x0d\x4f\x02\x03\x01\x00\x01\xa3\x82\x01\x32\x30\x82\x01\x2e\x30\x1d"
    "\x06\x03\x55\x1d\x0e\x04\x16\x04\x14\xfd\xac\x61\x32\x93\x6c\x45\xd6\xe2\xee\x85\x5f\x9a\xba\xe7\x76\x99\x68\xcc"
    "\xe7\x30\x1f\x06\x03\x55\x1d\x23\x04\x18\x30\x16\x80\x14\xd2\xc4\xb0\xd2\x91\xd4\x4c\x11\x71\xb3\x61\xcb\x3d\xa1"
    "\xfe\xdd\xa8\x6a\xd4\xe3\x30\x12\x06\x03\x55\x1d\x13\x01\x01\xff\x04\x08\x30\x06\x01\x01\xff\x02\x01\x00\x30\x33"
    "\x06\x08\x2b\x06\x01\x05\x05\x07\x01\x01\x04\x27\x30\x25\x30\x23\x06\x08\x2b\x06\x01\x05\x05\x07\x30\x01\x86\x17"
    "\x68\x74\x74\x70\x3a\x2f\x2f\x6f\x63\x73\x70\x2e\x67\x6f\x64\x61\x64\x64\x79\x2e\x63\x6f\x6d\x30\x46\x06\x03\x55"
    "\x1d\x1f\x04\x3f\x30\x3d\x30\x3b\xa0\x39\xa0\x37\x86\x35\x68\x74\x74\x70\x3a\x2f\x2f\x63\x65\x72\x74\x69\x66\x69"
    "\x63\x61\x74\x65\x73\x2e\x67\x6f\x64\x61\x64\x64\x79\x2e\x63\x6f\x6d\x2f\x72\x65\x70\x6f\x73\x69\x74\x6f\x72\x79"
    "\x2f\x67\x64\x72\x6f\x6f\x74\x2e\x63\x72\x6c\x30\x4b\x06\x03\x55\x1d\x20\x04\x44\x30\x42\x30\x40\x06\x04\x55\x1d"
    "\x20\x00\x30\x38\x30\x36\x06\x08\x2b\x06\x01\x05\x05\x07\x02\x01\x16\x2a\x68\x74\x74\x70\x3a\x2f\x2f\x63\x65\x72"
    "\x74\x69\x66\x69\x63\x61\x74\x65\x73\x2e\x67\x6f\x64\x61\x64\x64\x79\x2e\x63\x6f\x6d\x2f\x72\x65\x70\x6f\x73\x69"
    "\x74\x6f\x72\x79\x30\x0e\x06\x03\x55\x1d\x0f\x01\x01\xff\x04\x04\x03\x02\x01\x06\x30\x0d\x06\x09\x2a\x86\x48\x86"
    "\xf7\x0d\x01\x01\x05\x05\x00\x03\x82\x01\x01\x00\xd2\x86\xc0\xec\xbd\xf9\xa1\xb6\x67\xee\x66\x0b\xa2\x06\x3a\x04"
    "\x50\x8e\x15\x72\xac\x4a\x74\x95\x53\xcb\x37\xcb\x44\x49\xef\x07\x90\x6b\x33\xd9\x96\xf0\x94\x56\xa5\x13\x30\x05"
    "\x3c\x85\x32\x21\x7b\xc9\xc7\x0a\xa8\x24\xa4\x90\xde\x46\xd3\x25\x23\x14\x03\x67\xc2\x10\xd6\x6f\x0f\x5d\x7b\x7a"
    "\xcc\x9f\xc5\x58\x2a\xc1\xc4\x9e\x21\xa8\x5a\xf3\xac\xa4\x46\xf3\x9e\xe4\x63\xcb\x2f\x90\xa4\x29\x29\x01\xd9\x72"
    "\x2c\x29\xdf\x37\x01\x27\xbc\x4f\xee\x68\xd3\x21\x8f\xc0\xb3\xe4\xf5\x09\xed\xd2\x10\xaa\x53\xb4\xbe\xf0\xcc\x59"
    "\x0b\xd6\x3b\x96\x1c\x95\x24\x49\xdf\xce\xec\xfd\xa7\x48\x91\x14\x45\x0e\x3a\x36\x6f\xda\x45\xb3\x45\xa2\x41\xc9"
    "\xd4\xd7\x44\x4e\x3e\xb9\x74\x76\xd5\xa2\x13\x55\x2c\xc6\x87\xa3\xb5\x99\xac\x06\x84\x87\x7f\x75\x06\xfc\xbf\x14"
    "\x4c\x0e\xcc\x6e\xc4\xdf\x3d\xb7\x12\x71\xf4\xe8\xf1\x51\x40\x22\x28\x49\xe0\x1d\x4b\x87\xa8\x34\xcc\x06\xa2\xdd"
    "\x12\x5a\xd1\x86\x36\x64\x03\x35\x6f\x6f\x77\x6e\xeb\xf2\x85\x50\x98\x5e\xab\x03\x53\xad\x91\x23\x63\x1f\x16\x9c"
    "\xcd\xb9\xb2\x05\x63\x3a\xe1\xf4\x68\x1b\x17\x05\x35\x95\x53\xee\x0a\x84\x08\x30\x82\x04\x00\x30\x82\x02\xe8\xa0"
    "\x03\x02\x01\x02\x02\x01\x00\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05\x05\x00\x30\x63\x31\x0b\x30\x09"
    "\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x21\x30\x1f\x06\x03\x55\x04\x0a\x13\x18\x54\x68\x65\x20\x47\x6f\x20\x44"
    "\x61\x64\x64\x79\x20\x47\x72\x6f\x75\x70\x2c\x20\x49\x6e\x63\x2e\x31\x31\x30\x2f\x06\x03\x55\x04\x0b\x13\x28\x47"
    "\x6f\x20\x44\x61\x64\x64\x79\x20\x43\x6c\x61\x73\x73\x20\x32\x20\x43\x65\x72\x74\x69\x66\x69\x63\x61\x74\x69\x6f"
    "\x6e\x20\x41\x75\x74\x68\x6f\x72\x69\x74\x79\x30\x1e\x17\x0d\x30\x34\x30\x36\x32\x39\x31\x37\x30\x36\x32\x30\x5a"
    "\x17\x0d\x33\x34\x30\x36\x32\x39\x31\x37\x30\x36\x32\x30\x5a\x30\x63\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02"
    "\x55\x53\x31\x21\x30\x1f\x06\x03\x55\x04\x0a\x13\x18\x54\x68\x65\x20\x47\x6f\x20\x44\x61\x64\x64\x79\x20\x47\x72"
    "\x6f\x75\x70\x2c\x20\x49\x6e\x63\x2e\x31\x31\x30\x2f\x06\x03\x55\x04\x0b\x13\x28\x47\x6f\x20\x44\x61\x64\x64\x79"
    "\x20\x43\x6c\x61\x73\x73\x20\x32\x20\x43\x65\x72\x74\x69\x66\x69\x63\x61\x74\x69\x6f\x6e\x20\x41\x75\x74\x68\x6f"
    "\x72\x69\x74\x79\x30\x82\x01\x20\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x01\x0d\x00"
    "\x30\x82\x01\x08\x02\x82\x01\x01\x00\xde\x9d\xd7\xea\x57\x18\x49\xa1\x5b\xeb\xd7\x5f\x48\x86\xea\xbe\xdd\xff\xe4"
    "\xef\x67\x1c\xf4\x65\x68\xb3\x57\x71\xa0\x5e\x77\xbb\xed\x9b\x49\xe9\x70\x80\x3d\x56\x18\x63\x08\x6f\xda\xf2\xcc"
    "\xd0\x3f\x7f\x02\x54\x22\x54\x10\xd8\xb2\x81\xd4\xc0\x75\x3d\x4b\x7f\xc7\x77\xc3\x3e\x78\xab\x1a\x03\xb5\x20\x6b"
    "\x2f\x6a\x2b\xb1\xc5\x88\x7e\xc4\xbb\x1e\xb0\xc1\xd8\x45\x27\x6f\xaa\x37\x58\xf7\x87\x26\xd7\xd8\x2d\xf6\xa9\x17"
    "\xb7\x1f\x72\x36\x4e\xa6\x17\x3f\x65\x98\x92\xdb\x2a\x6e\x5d\xa2\xfe\x88\xe0\x0b\xde\x7f\xe5\x8d\x15\xe1\xeb\xcb"
    "\x3a\xd5\xe2\x12\xa2\x13\x2d\xd8\x8e\xaf\x5f\x12\x3d\xa0\x08\x05\x08\xb6\x5c\xa5\x65\x38\x04\x45\x99\x1e\xa3\x60"
    "\x60\x74\xc5\x41\xa5\x72\x62\x1b\x62\xc5\x1f\x6f\x5f\x1a\x42\xbe\x02\x51\x65\xa8\xae\x23\x18\x6a\xfc\x78\x03\xa9"
    "\x4d\x7f\x80\xc3\xfa\xab\x5a\xfc\xa1\x40\xa4\xca\x19\x16\xfe\xb2\xc8\xef\x5e\x73\x0d\xee\x77\xbd\x9a\xf6\x79\x98"
    "\xbc\xb1\x07\x67\xa2\x15\x0d\xdd\xa0\x58\xc6\x44\x7b\x0a\x3e\x62\x28\x5f\xba\x41\x07\x53\x58\xcf\x11\x7e\x38\x74"
    "\xc5\xf8\xff\xb5\x69\x90\x8f\x84\x74\xea\x97\x1b\xaf\x02\x01\x03\xa3\x81\xc0\x30\x81\xbd\x30\x1d\x06\x03\x55\x1d"
    "\x0e\x04\x16\x04\x14\xd2\xc4\xb0\xd2\x91\xd4\x4c\x11\x71\xb3\x61\xcb\x3d\xa1\xfe\xdd\xa8\x6a\xd4\xe3\x30\x81\x8d"
    "\x06\x03\x55\x1d\x23\x04\x81\x85\x30\x81\x82\x80\x14\xd2\xc4\xb0\xd2\x91\xd4\x4c\x11\x71\xb3\x61\xcb\x3d\xa1\xfe"
    "\xdd\xa8\x6a\xd4\xe3\xa1\x67\xa4\x65\x30\x63\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x21\x30\x1f"
    "\x06\x03\x55\x04\x0a\x13\x18\x54\x68\x65\x20\x47\x6f\x20\x44\x61\x64\x64\x79\x20\x47\x72\x6f\x75\x70\x2c\x20\x49"
    "\x6e\x63\x2e\x31\x31\x30\x2f\x06\x03\x55\x04\x0b\x13\x28\x47\x6f\x20\x44\x61\x64\x64\x79\x20\x43\x6c\x61\x73\x73"
    "\x20\x32\x20\x43\x65\x72\x74\x69\x66\x69\x63\x61\x74\x69\x6f\x6e\x20\x41\x75\x74\x68\x6f\x72\x69\x74\x79\x82\x01"
    "\x00\x30\x0c\x06\x03\x55\x1d\x13\x04\x05\x30\x03\x01\x01\xff\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x05"
    "\x05\x00\x03\x82\x01\x01\x00\x32\x4b\xf3\xb2\xca\x3e\x91\xfc\x12\xc6\xa1\x07\x8c\x8e\x77\xa0\x33\x06\x14\x5c\x90"
    "\x1e\x18\xf7\x08\xa6\x3d\x0a\x19\xf9\x87\x80\x11\x6e\x69\xe4\x96\x17\x30\xff\x34\x91\x63\x72\x38\xee\xcc\x1c\x01"
    "\xa3\x1d\x94\x28\xa4\x31\xf6\x7a\xc4\x54\xd7\xf6\xe5\x31\x58\x03\xa2\xcc\xce\x62\xdb\x94\x45\x73\xb5\xbf\x45\xc9"
    "\x24\xb5\xd5\x82\x02\xad\x23\x79\x69\x8d\xb8\xb6\x4d\xce\xcf\x4c\xca\x33\x23\xe8\x1c\x88\xaa\x9d\x8b\x41\x6e\x16"
    "\xc9\x20\xe5\x89\x9e\xcd\x3b\xda\x70\xf7\x7e\x99\x26\x20\x14\x54\x25\xab\x6e\x73\x85\xe6\x9b\x21\x9d\x0a\x6c\x82"
    "\x0e\xa8\xf8\xc2\x0c\xfa\x10\x1e\x6c\x96\xef\x87\x0d\xc4\x0f\x61\x8b\xad\xee\x83\x2b\x95\xf8\x8e\x92\x84\x72\x39"
    "\xeb\x20\xea\x83\xed\x83\xcd\x97\x6e\x08\xbc\xeb\x4e\x26\xb6\x73\x2b\xe4\xd3\xf6\x4c\xfe\x26\x71\xe2\x61\x11\x74"
    "\x4a\xff\x57\x1a\x87\x0f\x75\x48\x2e\xcf\x51\x69\x17\xa0\x02\x12\x61\x95\xd5\xd1\x40\xb2\x10\x4c\xee\xc4\xac\x10"
    "\x43\xa6\xa5\x9e\x0a\xd5\x95\x62\x9a\x0d\xcf\x88\x82\xc5\x32\x0c\xe4\x2b\x9f\x45\xe6\x0d\x9f\x28\x9c\xb1\xb9\x2a"
    "\x5a\x57\xad\x37\x0f\xaf\x1d\x7f\xdb\xbd\x9f\x22\x9b\x01\x0a\x04\x6d\x61\x69\x6e\x12\x1f\x08\xe0\xb6\x0d\x12\x19"
    "\x76\xa9\x14\xa5\x33\xd4\xfa\x07\x66\x34\xaf\xef\x47\x45\x1f\x6a\xec\x8c\xdc\x1e\x49\xda\xf0\x88\xac\x18\xee\xe1"
    "\x80\x9b\x05\x20\xf2\xe8\x80\x9b\x05\x2a\x39\x50\x61\x79\x6d\x65\x6e\x74\x20\x72\x65\x71\x75\x65\x73\x74\x20\x66"
    "\x6f\x72\x20\x42\x69\x74\x50\x61\x79\x20\x69\x6e\x76\x6f\x69\x63\x65\x20\x38\x63\x58\x35\x52\x62\x4e\x38\x61\x6f"
    "\x66\x63\x35\x33\x61\x57\x41\x6d\x35\x58\x46\x44\x32\x2b\x68\x74\x74\x70\x73\x3a\x2f\x2f\x62\x69\x74\x70\x61\x79"
    "\x2e\x63\x6f\x6d\x2f\x69\x2f\x38\x63\x58\x35\x52\x62\x4e\x38\x61\x6f\x66\x63\x35\x33\x61\x57\x41\x6d\x35\x58\x46"
    "\x44\x2a\x80\x02\x5e\xf8\x8b\xec\x4e\x09\xbe\x97\x9b\x07\x06\x64\x76\x4a\xfa\xe4\xfa\x3b\x1e\xca\x95\x47\x44\xa7"
    "\x66\x99\xb1\x85\x30\x18\x3e\x6f\x46\x7e\xc5\x92\x39\x13\x66\x8c\x5a\xbe\x38\x2c\xb7\xef\x6a\x88\x58\xfa\xe6\x18"
    "\x0c\x47\x8e\x81\x17\x9d\x39\x35\xcd\x53\x23\xf0\xc5\xcc\x2e\xea\x0f\x1e\x29\xb5\xa6\xb2\x65\x4b\x4c\xbd\xa3\x89"
    "\xea\xee\x32\x21\x5c\x87\x77\xaf\xbb\xe0\x7d\x60\xa4\xf9\xfa\x07\xab\x6e\x9a\x6d\x3a\xd2\xa9\xef\xb5\x25\x22\x16"
    "\x31\xc8\x04\x4e\xc7\x59\xd9\xc1\xfc\xcc\x39\xbb\x3e\xe4\xf4\x4e\xbc\x7c\x1c\xc8\x24\x83\x41\x44\x27\x22\xac\x88"
    "\x0d\xa0\xc7\xd5\x9d\x69\x67\x06\xc7\xbc\xf0\x91"; // 4095 character string literal limit in C99
    
    const char buf2[] = "\x01\xb4\x92\x5a\x07\x84\x22\x0a\x93\xc5\xb3\x09\xda\xd8\xe3\x26\x61\xf2\xcc\xab\x4e\xc8\x68"
    "\xb2\xde\x00\x0f\x24\x2d\xb7\x3f\xff\xb2\x69\x37\xcf\x83\xed\x6d\x2e\xfa\xa7\x71\xd2\xd2\xc6\x97\x84\x4b\x83\x94"
    "\x8c\x98\x25\x2b\x5f\x35\x2e\xdd\x4f\xe9\x6b\x29\xcb\xe0\xc9\xca\x3d\x10\x7a\x3e\xb7\x90\xda\xb5\xdd\xd7\x3d\xe6"
    "\xc7\x48\xf2\x04\x7d\xb4\x25\xc8\x0c\x39\x13\x54\x73\xca\xca\xd3\x61\x9b\xaa\xf2\x8e\x39\x1d\xa4\xa6\xc7\xb8\x2b"
    "\x74";
    
    uint8_t buf3[(sizeof(buf1) - 1) + (sizeof(buf2) - 1)];
    
    memcpy(buf3, buf1, sizeof(buf1) - 1);
    memcpy(buf3 + (sizeof(buf1) - 1), buf2, sizeof(buf2) - 1);

    BRPaymentProtocolRequest *req = BRPaymentProtocolRequestParse(buf3, sizeof(buf3));
    uint8_t buf4[BRPaymentProtocolRequestSerialize(req, NULL, 0)];
    size_t len = BRPaymentProtocolRequestSerialize(req, buf4, sizeof(buf4));
    int i = 0;

    if (len != sizeof(buf3) || memcmp(buf3, buf4, len) != 0) // check if parse/serialize produces same result
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolRequestParse/Serialize() test 1\n", __func__);

    do {
        uint8_t buf5[BRPaymentProtocolRequestCert(req, NULL, 0, i)];
    
        len = BRPaymentProtocolRequestCert(req, buf5, sizeof(buf5), i);
        if (len > 0) i++;
    } while (len > 0);

    // check for a chain of 3 certificates
    if (i != 3) r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolRequestCert() test 1\n", __func__);
    
    if (req->details->expires == 0 || req->details->expires >= time(NULL)) // check that request is expired
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolRequest->details->expires test 1\n", __func__);
    
    if (req) BRPaymentProtocolRequestFree(req);

    const char buf5[] = "\x0a\x00\x12\x5f\x54\x72\x61\x6e\x73\x61\x63\x74\x69\x6f\x6e\x20\x72\x65\x63\x65\x69\x76\x65"
    "\x64\x20\x62\x79\x20\x42\x69\x74\x50\x61\x79\x2e\x20\x49\x6e\x76\x6f\x69\x63\x65\x20\x77\x69\x6c\x6c\x20\x62\x65"
    "\x20\x6d\x61\x72\x6b\x65\x64\x20\x61\x73\x20\x70\x61\x69\x64\x20\x69\x66\x20\x74\x68\x65\x20\x74\x72\x61\x6e\x73"
    "\x61\x63\x74\x69\x6f\x6e\x20\x69\x73\x20\x63\x6f\x6e\x66\x69\x72\x6d\x65\x64\x2e";
    BRPaymentProtocolACK *ack = BRPaymentProtocolACKParse((const uint8_t *)buf5, sizeof(buf5) - 1);
    uint8_t buf6[BRPaymentProtocolACKSerialize(ack, NULL, 0)];

    len = BRPaymentProtocolACKSerialize(ack, buf6, sizeof(buf6));
    if (len != sizeof(buf5) - 1 || memcmp(buf5, buf6, len) != 0) // check if parse/serialize produces same result
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolACKParse/Serialize() test\n", __func__);
    
    printf("\n");
    if (ack->memo) printf("%s\n", ack->memo);
    // check that memo is not NULL
    if (! ack->memo) r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolACK->memo test\n", __func__);

    const char buf7[] = "\x12\x0b\x78\x35\x30\x39\x2b\x73\x68\x61\x32\x35\x36\x1a\xbe\x15\x0a\xfe\x0b\x30\x82\x05\xfa"
    "\x30\x82\x04\xe2\xa0\x03\x02\x01\x02\x02\x10\x09\x0b\x35\xca\x5c\x5b\xf1\xb9\x8b\x3d\x8f\x9f\x4a\x77\x55\xd6\x30"
    "\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x0b\x05\x00\x30\x75\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55"
    "\x53\x31\x15\x30\x13\x06\x03\x55\x04\x0a\x13\x0c\x44\x69\x67\x69\x43\x65\x72\x74\x20\x49\x6e\x63\x31\x19\x30\x17"
    "\x06\x03\x55\x04\x0b\x13\x10\x77\x77\x77\x2e\x64\x69\x67\x69\x63\x65\x72\x74\x2e\x63\x6f\x6d\x31\x34\x30\x32\x06"
    "\x03\x55\x04\x03\x13\x2b\x44\x69\x67\x69\x43\x65\x72\x74\x20\x53\x48\x41\x32\x20\x45\x78\x74\x65\x6e\x64\x65\x64"
    "\x20\x56\x61\x6c\x69\x64\x61\x74\x69\x6f\x6e\x20\x53\x65\x72\x76\x65\x72\x20\x43\x41\x30\x1e\x17\x0d\x31\x34\x30"
    "\x35\x30\x39\x30\x30\x30\x30\x30\x30\x5a\x17\x0d\x31\x36\x30\x35\x31\x33\x31\x32\x30\x30\x30\x30\x5a\x30\x82\x01"
    "\x05\x31\x1d\x30\x1b\x06\x03\x55\x04\x0f\x0c\x14\x50\x72\x69\x76\x61\x74\x65\x20\x4f\x72\x67\x61\x6e\x69\x7a\x61"
    "\x74\x69\x6f\x6e\x31\x13\x30\x11\x06\x0b\x2b\x06\x01\x04\x01\x82\x37\x3c\x02\x01\x03\x13\x02\x55\x53\x31\x19\x30"
    "\x17\x06\x0b\x2b\x06\x01\x04\x01\x82\x37\x3c\x02\x01\x02\x13\x08\x44\x65\x6c\x61\x77\x61\x72\x65\x31\x10\x30\x0e"
    "\x06\x03\x55\x04\x05\x13\x07\x35\x31\x35\x34\x33\x31\x37\x31\x0f\x30\x0d\x06\x03\x55\x04\x09\x0c\x06\x23\x32\x33"
    "\x30\x30\x38\x31\x17\x30\x15\x06\x03\x55\x04\x09\x13\x0e\x35\x34\x38\x20\x4d\x61\x72\x6b\x65\x74\x20\x53\x74\x2e"
    "\x31\x0e\x30\x0c\x06\x03\x55\x04\x11\x13\x05\x39\x34\x31\x30\x34\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55"
    "\x53\x31\x13\x30\x11\x06\x03\x55\x04\x08\x13\x0a\x43\x61\x6c\x69\x66\x6f\x72\x6e\x69\x61\x31\x16\x30\x14\x06\x03"
    "\x55\x04\x07\x13\x0d\x53\x61\x6e\x20\x46\x72\x61\x6e\x63\x69\x73\x63\x6f\x31\x17\x30\x15\x06\x03\x55\x04\x0a\x13"
    "\x0e\x43\x6f\x69\x6e\x62\x61\x73\x65\x2c\x20\x49\x6e\x63\x2e\x31\x15\x30\x13\x06\x03\x55\x04\x03\x13\x0c\x63\x6f"
    "\x69\x6e\x62\x61\x73\x65\x2e\x63\x6f\x6d\x30\x82\x01\x22\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05"
    "\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01\x00\xb4\x5e\x3f\xf3\x80\x66\x7a\xa1\x4d\x5a\x12\xfc\x2f"
    "\xc9\x83\xfc\x66\x18\xb5\x54\x99\x93\x3c\x3b\xde\x15\xc0\x1d\x83\x88\x46\xb4\xca\xf9\x84\x8e\x7c\x40\xe5\xfa\x7c"
    "\x67\xef\x9b\x5b\x1e\xfe\x26\xee\x55\x71\xc5\xfa\x2e\xff\x75\x90\x52\x45\x47\x01\xad\x89\x31\x55\x7d\x69\x7b\x13"
    "\x9e\x5d\x19\xab\xb3\xe4\x39\x67\x5f\x31\xdb\x7f\x2e\xf1\xa5\xd9\x7d\xb0\x7c\x1f\x69\x66\x26\x63\x80\xeb\x4f\xcf"
    "\xa8\xe1\x47\x1a\x6e\xcc\x2f\xbe\xbf\x3e\x67\xb3\xea\xa8\x4d\x0f\xbe\x06\x3e\x60\x38\x0d\xcd\xb7\xa2\x02\x03\xd2"
    "\x9a\x94\x05\x9e\xf7\xf2\x0d\x47\x2c\xc2\x57\x83\xab\x2a\x1d\xb6\xa3\x94\xec\xc0\x7b\x40\x24\x97\x41\x00\xbc\xfd"
    "\x47\x0f\x59\xef\x3b\x57\x23\x65\x21\x32\x09\x60\x9f\xad\x22\x99\x94\xb4\x92\x3c\x1d\xf3\xa1\x8c\x41\xe3\xe7\xbc"
    "\x1f\x19\x2b\xa6\xe7\xe5\xc3\x2a\xe1\x55\x10\x7e\x21\x90\x3e\xff\x7b\xce\x9f\xc5\x94\xb4\x9d\x9f\x6a\xe7\x90\x1f"
    "\xa1\x91\xfc\xba\xe8\xa2\xcf\x09\xc3\xbf\xc2\x43\x77\xd7\x17\xb6\x01\x00\x80\xc5\x68\x1a\x7d\xbc\x6e\x1d\x52\x98"
    "\x7b\x7e\xbb\xe9\x5e\x7a\xf4\x20\x2d\xa4\x36\xe6\x7a\x88\x47\x2a\xac\xed\xc9\x02\x03\x01\x00\x01\xa3\x82\x01\xf2"
    "\x30\x82\x01\xee\x30\x1f\x06\x03\x55\x1d\x23\x04\x18\x30\x16\x80\x14\x3d\xd3\x50\xa5\xd6\xa0\xad\xee\xf3\x4a\x60"
    "\x0a\x65\xd3\x21\xd4\xf8\xf8\xd6\x0f\x30\x1d\x06\x03\x55\x1d\x0e\x04\x16\x04\x14\x6d\x33\xb9\x74\x3a\x61\xb7\x49"
    "\x94\x23\xd1\xa8\x9d\x08\x5d\x01\x48\x68\x0b\xba\x30\x29\x06\x03\x55\x1d\x11\x04\x22\x30\x20\x82\x0c\x63\x6f\x69"
    "\x6e\x62\x61\x73\x65\x2e\x63\x6f\x6d\x82\x10\x77\x77\x77\x2e\x63\x6f\x69\x6e\x62\x61\x73\x65\x2e\x63\x6f\x6d\x30"
    "\x0e\x06\x03\x55\x1d\x0f\x01\x01\xff\x04\x04\x03\x02\x05\xa0\x30\x1d\x06\x03\x55\x1d\x25\x04\x16\x30\x14\x06\x08"
    "\x2b\x06\x01\x05\x05\x07\x03\x01\x06\x08\x2b\x06\x01\x05\x05\x07\x03\x02\x30\x75\x06\x03\x55\x1d\x1f\x04\x6e\x30"
    "\x6c\x30\x34\xa0\x32\xa0\x30\x86\x2e\x68\x74\x74\x70\x3a\x2f\x2f\x63\x72\x6c\x33\x2e\x64\x69\x67\x69\x63\x65\x72"
    "\x74\x2e\x63\x6f\x6d\x2f\x73\x68\x61\x32\x2d\x65\x76\x2d\x73\x65\x72\x76\x65\x72\x2d\x67\x31\x2e\x63\x72\x6c\x30"
    "\x34\xa0\x32\xa0\x30\x86\x2e\x68\x74\x74\x70\x3a\x2f\x2f\x63\x72\x6c\x34\x2e\x64\x69\x67\x69\x63\x65\x72\x74\x2e"
    "\x63\x6f\x6d\x2f\x73\x68\x61\x32\x2d\x65\x76\x2d\x73\x65\x72\x76\x65\x72\x2d\x67\x31\x2e\x63\x72\x6c\x30\x42\x06"
    "\x03\x55\x1d\x20\x04\x3b\x30\x39\x30\x37\x06\x09\x60\x86\x48\x01\x86\xfd\x6c\x02\x01\x30\x2a\x30\x28\x06\x08\x2b"
    "\x06\x01\x05\x05\x07\x02\x01\x16\x1c\x68\x74\x74\x70\x73\x3a\x2f\x2f\x77\x77\x77\x2e\x64\x69\x67\x69\x63\x65\x72"
    "\x74\x2e\x63\x6f\x6d\x2f\x43\x50\x53\x30\x81\x88\x06\x08\x2b\x06\x01\x05\x05\x07\x01\x01\x04\x7c\x30\x7a\x30\x24"
    "\x06\x08\x2b\x06\x01\x05\x05\x07\x30\x01\x86\x18\x68\x74\x74\x70\x3a\x2f\x2f\x6f\x63\x73\x70\x2e\x64\x69\x67\x69"
    "\x63\x65\x72\x74\x2e\x63\x6f\x6d\x30\x52\x06\x08\x2b\x06\x01\x05\x05\x07\x30\x02\x86\x46\x68\x74\x74\x70\x3a\x2f"
    "\x2f\x63\x61\x63\x65\x72\x74\x73\x2e\x64\x69\x67\x69\x63\x65\x72\x74\x2e\x63\x6f\x6d\x2f\x44\x69\x67\x69\x43\x65"
    "\x72\x74\x53\x48\x41\x32\x45\x78\x74\x65\x6e\x64\x65\x64\x56\x61\x6c\x69\x64\x61\x74\x69\x6f\x6e\x53\x65\x72\x76"
    "\x65\x72\x43\x41\x2e\x63\x72\x74\x30\x0c\x06\x03\x55\x1d\x13\x01\x01\xff\x04\x02\x30\x00\x30\x0d\x06\x09\x2a\x86"
    "\x48\x86\xf7\x0d\x01\x01\x0b\x05\x00\x03\x82\x01\x01\x00\xaa\xdf\xcf\x94\x05\x0e\xd9\x38\xe3\x11\x4a\x64\x0a\xf3"
    "\xd9\xb0\x42\x76\xda\x00\xf5\x21\x5d\x71\x48\xf9\xf1\x6d\x4c\xac\x0c\x77\xbd\x53\x49\xec\x2f\x47\x29\x9d\x03\xc9"
    "\x00\xf7\x01\x46\x75\x2d\xa7\x28\x29\x29\x0a\xc5\x0a\x77\x99\x2f\x01\x53\x7a\xb2\x68\x93\x92\xce\x0b\xfe\xb7\xef"
    "\xa4\x9f\x4c\x4f\xe4\xe1\xe4\x3c\xa1\xfc\xfb\x16\x26\xce\x55\x4d\xa4\xf6\xe7\xfa\x34\xa5\x97\xe4\x01\xf2\x15\xc4"
    "\x3a\xfd\x0b\xa7\x77\xad\x58\x7e\xb0\xaf\xac\xd7\x1f\x7a\x6a\xf7\x75\x28\x14\xf7\xab\x4c\x20\x2e\xd7\x6d\x33\xde"
    "\xfd\x12\x89\xd5\x41\x80\x3f\xed\x01\xac\x80\xa3\xca\xcf\xda\xae\x29\x27\x9e\x5d\xe1\x4d\x46\x04\x75\xf4\xba\xf2"
    "\x7e\xab\x69\x33\x79\xd3\x91\x20\xe7\x47\x7b\xf3\xec\x71\x96\x64\xc7\xb6\xcb\x5e\x55\x75\x56\xe5\xbb\xdd\xd9\xc9"
    "\xd1\xeb\xc9\xf8\x35\xe9\xda\x5b\x3d\xbb\x72\xfe\x8d\x94\xac\x05\xea\xb3\xc4\x79\x98\x75\x20\xad\xe3\xa1\xd2\x75"
    "\xe1\xe2\xfe\x72\x56\x98\xd2\xf7\xcb\x13\x90\xa9\xd4\x0e\xa6\xcb\xf2\x1a\x73\xbd\xdc\xcd\x1a\xd6\x1a\xa2\x49\xce"
    "\x8e\x28\x85\xa3\x73\x0b\x7d\x53\xbd\x07\x5f\x55\x09\x9d\x29\x60\xf3\xcc\x0a\xba\x09\x30\x82\x04\xb6\x30\x82\x03"
    "\x9e\xa0\x03\x02\x01\x02\x02\x10\x0c\x79\xa9\x44\xb0\x8c\x11\x95\x20\x92\x61\x5f\xe2\x6b\x1d\x83\x30\x0d\x06\x09"
    "\x2a\x86\x48\x86\xf7\x0d\x01\x01\x0b\x05\x00\x30\x6c\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53\x31\x15"
    "\x30\x13\x06\x03\x55\x04\x0a\x13\x0c\x44\x69\x67\x69\x43\x65\x72\x74\x20\x49\x6e\x63\x31\x19\x30\x17\x06\x03\x55"
    "\x04\x0b\x13\x10\x77\x77\x77\x2e\x64\x69\x67\x69\x63\x65\x72\x74\x2e\x63\x6f\x6d\x31\x2b\x30\x29\x06\x03\x55\x04"
    "\x03\x13\x22\x44\x69\x67\x69\x43\x65\x72\x74\x20\x48\x69\x67\x68\x20\x41\x73\x73\x75\x72\x61\x6e\x63\x65\x20\x45"
    "\x56\x20\x52\x6f\x6f\x74\x20\x43\x41\x30\x1e\x17\x0d\x31\x33\x31\x30\x32\x32\x31\x32\x30\x30\x30\x30\x5a\x17\x0d"
    "\x32\x38\x31\x30\x32\x32\x31\x32\x30\x30\x30\x30\x5a\x30\x75\x31\x0b\x30\x09\x06\x03\x55\x04\x06\x13\x02\x55\x53"
    "\x31\x15\x30\x13\x06\x03\x55\x04\x0a\x13\x0c\x44\x69\x67\x69\x43\x65\x72\x74\x20\x49\x6e\x63\x31\x19\x30\x17\x06"
    "\x03\x55\x04\x0b\x13\x10\x77\x77\x77\x2e\x64\x69\x67\x69\x63\x65\x72\x74\x2e\x63\x6f\x6d\x31\x34\x30\x32\x06\x03"
    "\x55\x04\x03\x13\x2b\x44\x69\x67\x69\x43\x65\x72\x74\x20\x53\x48\x41\x32\x20\x45\x78\x74\x65\x6e\x64\x65\x64\x20"
    "\x56\x61\x6c\x69\x64\x61\x74\x69\x6f\x6e\x20\x53\x65\x72\x76\x65\x72\x20\x43\x41\x30\x82\x01\x22\x30\x0d\x06\x09"
    "\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01\x05\x00\x03\x82\x01\x0f\x00\x30\x82\x01\x0a\x02\x82\x01\x01\x00\xd7\x53\xa4"
    "\x04\x51\xf8\x99\xa6\x16\x48\x4b\x67\x27\xaa\x93\x49\xd0\x39\xed\x0c\xb0\xb0\x00\x87\xf1\x67\x28\x86\x85\x8c\x8e"
    "\x63\xda\xbc\xb1\x40\x38\xe2\xd3\xf5\xec\xa5\x05\x18\xb8\x3d\x3e\xc5\x99\x17\x32\xec\x18\x8c\xfa\xf1\x0c\xa6\x64"
    "\x21\x85\xcb\x07\x10\x34\xb0\x52\x88\x2b\x1f\x68\x9b\xd2\xb1\x8f\x12\xb0\xb3\xd2\xe7\x88\x1f\x1f\xef\x38\x77\x54"
    "\x53\x5f\x80\x79\x3f\x2e\x1a\xaa\xa8\x1e\x4b\x2b\x0d\xab\xb7\x63\xb9\x35\xb7\x7d\x14\xbc\x59\x4b\xdf\x51\x4a\xd2"
    "\xa1\xe2\x0c\xe2\x90\x82\x87\x6a\xae\xea\xd7\x64\xd6\x98\x55\xe8\xfd\xaf\x1a\x50\x6c\x54\xbc\x11\xf2\xfd\x4a\xf2"
    "\x9d\xbb\x7f\x0e\xf4\xd5\xbe\x8e\x16\x89\x12\x55\xd8\xc0\x71\x34\xee\xf6\xdc\x2d\xec\xc4\x87\x25\x86\x8d\xd8\x21"
    "\xe4\xb0\x4d\x0c\x89\xdc\x39\x26\x17\xdd\xf6\xd7\x94\x85\xd8\x04\x21\x70\x9d\x6f\x6f\xff\x5c\xba\x19\xe1\x45\xcb"
    "\x56\x57\x28\x7e\x1c\x0d\x41\x57\xaa\xb7\xb8\x27\xbb\xb1\xe4\xfa\x2a\xef\x21\x23\x75\x1a\xad\x2d\x9b\x86\x35\x8c"
    "\x9c\x77\xb5\x73\xad\xd8\x94\x2d\xe4\xf3\x0c\x9d\xee\xc1\x4e\x62\x7e\x17\xc0\x71\x9e\x2c\xde\xf1\xf9\x10\x28\x19"
    "\x33\x02\x03\x01\x00\x01\xa3\x82\x01\x49\x30\x82\x01\x45\x30\x12\x06\x03\x55\x1d\x13\x01\x01\xff\x04\x08\x30\x06"
    "\x01\x01\xff\x02\x01\x00\x30\x0e\x06\x03\x55\x1d\x0f\x01\x01\xff\x04\x04\x03\x02\x01\x86\x30\x1d\x06\x03\x55\x1d"
    "\x25\x04\x16\x30\x14\x06\x08\x2b\x06\x01\x05\x05\x07\x03\x01\x06\x08\x2b\x06\x01\x05\x05\x07\x03\x02\x30\x34\x06"
    "\x08\x2b\x06\x01\x05\x05\x07\x01\x01\x04\x28\x30\x26\x30\x24\x06\x08\x2b\x06\x01\x05\x05\x07\x30\x01\x86\x18\x68"
    "\x74\x74\x70\x3a\x2f\x2f\x6f\x63\x73\x70\x2e\x64\x69\x67\x69\x63\x65\x72\x74\x2e\x63\x6f\x6d\x30\x4b\x06\x03\x55"
    "\x1d\x1f\x04\x44\x30\x42\x30\x40\xa0\x3e\xa0\x3c\x86\x3a\x68\x74\x74\x70\x3a\x2f\x2f\x63\x72\x6c\x34\x2e\x64\x69"
    "\x67\x69\x63\x65\x72\x74\x2e\x63\x6f\x6d\x2f\x44\x69\x67\x69\x43\x65\x72\x74\x48\x69\x67\x68\x41\x73\x73\x75\x72"
    "\x61\x6e\x63\x65\x45\x56\x52\x6f\x6f\x74\x43\x41\x2e\x63\x72\x6c\x30\x3d\x06\x03\x55\x1d\x20\x04\x36\x30\x34\x30"
    "\x32\x06\x04\x55\x1d\x20\x00\x30\x2a\x30\x28\x06\x08\x2b\x06\x01\x05\x05\x07\x02\x01\x16\x1c\x68\x74\x74\x70\x73"
    "\x3a\x2f\x2f\x77\x77\x77\x2e\x64\x69\x67\x69\x63\x65\x72\x74\x2e\x63\x6f\x6d\x2f\x43\x50\x53\x30\x1d\x06\x03\x55"
    "\x1d\x0e\x04\x16\x04\x14\x3d\xd3\x50\xa5\xd6\xa0\xad\xee\xf3\x4a\x60\x0a\x65\xd3\x21\xd4\xf8\xf8\xd6\x0f\x30\x1f"
    "\x06\x03\x55\x1d\x23\x04\x18\x30\x16\x80\x14\xb1\x3e\xc3\x69\x03\xf8\xbf\x47\x01\xd4\x98\x26\x1a\x08\x02\xef\x63"
    "\x64\x2b\xc3\x30\x0d\x06\x09\x2a\x86\x48\x86\xf7\x0d\x01\x01\x0b\x05\x00\x03\x82\x01\x01\x00\x9d\xb6\xd0\x90\x86"
    "\xe1\x86\x02\xed\xc5\xa0\xf0\x34\x1c\x74\xc1\x8d\x76\xcc\x86\x0a\xa8\xf0\x4a\x8a\x42\xd6\x3f\xc8\xa9\x4d\xad\x7c"
    "\x08\xad\xe6\xb6\x50\xb8\xa2\x1a\x4d\x88\x07\xb1\x29\x21\xdc\xe7\xda\xc6\x3c\x21\xe0\xe3\x11\x49\x70\xac\x7a\x1d"
    "\x01\xa4\xca\x11\x3a\x57\xab\x7d\x57\x2a\x40\x74\xfd\xd3\x1d\x85\x18\x50\xdf\x57\x47\x75\xa1\x7d\x55\x20\x2e\x47"
    "\x37\x50\x72\x8c\x7f\x82\x1b\xd2\x62\x8f\x2d\x03\x5a\xda\xc3\xc8\xa1\xce\x2c\x52\xa2\x00\x63\xeb\x73\xba\x71\xc8"
    "\x49\x27\x23\x97\x64\x85\x9e\x38\x0e\xad\x63\x68\x3c\xba\x52\x81\x58\x79\xa3\x2c\x0c\xdf\xde\x6d\xeb\x31\xf2\xba"
    "\xa0\x7c\x6c\xf1\x2c\xd4\xe1\xbd\x77\x84\x37\x03\xce\x32\xb5\xc8\x9a\x81\x1a\x4a\x92\x4e\x3b\x46\x9a\x85\xfe\x83"
    "\xa2\xf9\x9e\x8c\xa3\xcc\x0d\x5e\xb3\x3d\xcf\x04\x78\x8f\x14\x14\x7b\x32\x9c\xc7\x00\xa6\x5c\xc4\xb5\xa1\x55\x8d"
    "\x5a\x56\x68\xa4\x22\x70\xaa\x3c\x81\x71\xd9\x9d\xa8\x45\x3b\xf4\xe5\xf6\xa2\x51\xdd\xc7\x7b\x62\xe8\x6f\x0c\x74"
    "\xeb\xb8\xda\xf8\xbf\x87\x0d\x79\x50\x91\x90\x9b\x18\x3b\x91\x59\x27\xf1\x35\x28\x13\xab\x26\x7e\xd5\xf7\x7a\x22"
    "\xb4\x01\x12\x1f\x08\x98\xb7\x68\x12\x19\x76\xa9\x14\x7d\x53\x25\xa8\x54\xf0\xc9\xa1\xcb\xb6\xcb\xfb\x89\xb2\xa9"
    "\x6d\x83\x7e\xd7\xbf\x88\xac\x18\xac\xb9\xe0\x9e\x05\x20\xd2\xbc\xe0\x9e\x05\x2a\x31\x50\x61\x79\x6d\x65\x6e\x74"
    "\x20\x72\x65\x71\x75\x65\x73\x74\x20\x66\x6f\x72\x20\x43\x6f\x69\x6e\x62\x61\x73\x65\x20\x6f\x72\x64\x65\x72\x20"
    "\x63\x6f\x64\x65\x3a\x20\x51\x43\x4f\x49\x47\x44\x50\x41\x32\x30\x68\x74\x74\x70\x73\x3a\x2f\x2f\x63\x6f\x69\x6e"
    "\x62\x61\x73\x65\x2e\x63\x6f\x6d\x2f\x72\x70\x2f\x35\x33\x64\x38\x31\x62\x66\x61\x35\x64\x36\x62\x31\x64\x64\x61"
    "\x37\x62\x30\x30\x30\x30\x30\x34\x3a\x20\x33\x36\x32\x64\x32\x39\x31\x39\x32\x31\x37\x36\x32\x31\x33\x39\x32\x35"
    "\x38\x37\x36\x63\x65\x32\x63\x62\x34\x30\x30\x34\x31\x62\x2a\x80\x02\x4d\x81\xca\x72\x21\x38\x13\xb2\x58\x5d\x98"
    "\x00\x5b\x23\x8e\x26\x8a\x00\x9e\xc0\x2d\x04\xdd\x7a\x8a\x98\x48\x32\xb9\x90\xd7\x40\xa9\x69\x09\xd6\x2a\x5d\xf9"
    "\xf8\xf8\x5b\x67\x32\x93\x79\xbb\xa0\xa9\xba\x03\xbc\xa3\xd6\x14\x00\xd4\xe4\x77\x98\x4b\x7e\xdc\xf3\x04\x22\x61"
    "\x71\x84\x23\x73\x6c\x44\x1d\x14\x0e\xe8\x9d\x64\x60\x96\x67\xde\x50\xea\xdb\x4c\xab\xbe\xf4\x78\xd3\xa9\xcb\xd4"
    "\xdf\xda\xb9\xa0\xc2\x81\x83\x90\xd2\x0c\x24\x3a\xd0\x2c\xc2\x7a\xbf\x0b\xbb\x2b\xab\x32\x27\xba\xa8\xe5\xd6\x73"
    "\xf8\x49\x91\x41\x22\x53\xbe\x1e\x69\xdf\xa7\x80\xdc\x06\xb6\xf4\x8e\xdf\xa1\x5d\xe6\xd0\xcc\xec\x22\xd9\xfa\xaf"
    "\x67\xb5\x35\xe8\xb2\x77\x8c\xdf\x61\x84\xda\x2f\x2d\x17\x92\xd3\x4c\x64\x40\x98\x83\x27\x32\x9e\x9c\x5a\xe1\x8c"
    "\x34\xdd\xa1\x6d\xcd\xfb\xf4\x19\xf7\xfd\x27\xbf\x57\x5b\x6f\x9c\x95\xb1\xf0\x90\x02\x16\x40\xaf\x5c\x02\xad\x02"
    "\x7b\x5d\x76\x05\x3a\x58\x40\xbc\x4d\x61\x04\xdd\x87\xef\xc3\x1b\xcc\x3a\x8a\xef\xc3\x10\x02\x35\xbe\x61\xc0\x3a"
    "\x50\x55\x66\x77\x71\x85\xdd\x6f\x93\x2b\xae\xb5\xd5\xe2\xd4\x39\x8d\x01\x14\x0d\x48";

    req = BRPaymentProtocolRequestParse((const uint8_t *)buf7, sizeof(buf7) - 1);

    uint8_t buf8[BRPaymentProtocolRequestSerialize(req, NULL, 0)];

    len = BRPaymentProtocolRequestSerialize(req, buf8, sizeof(buf8));
    if (len != sizeof(buf7) - 1 || memcmp(buf7, buf8, len) != 0) // check if parse/serialize produces same result
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolRequestParse/Serialize() test 2\n", __func__);
    i = 0;
    
    do {
        uint8_t buf8[BRPaymentProtocolRequestCert(req, NULL, 0, i)];
        
        len = BRPaymentProtocolRequestCert(req, buf8, sizeof(buf8), i);
        if (len > 0) i++;
    } while (len > 0);
    
    // check for a chain of 2 certificates
    if (i != 2) r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolRequestCert() test 2\n", __func__);
    
    if (req->details->expires == 0 || req->details->expires >= time(NULL)) // check that request is expired
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolRequest->details->expires test 2\n", __func__);
    
    if (req) BRPaymentProtocolRequestFree(req);
    
    // test garbage input
    const char buf9[] = "jfkdlasjfalk;sjfal;jflsadjfla;s";
    
    req = BRPaymentProtocolRequestParse((const uint8_t *)buf9, sizeof(buf9) - 1);
    
    uint8_t buf0[(req) ? BRPaymentProtocolRequestSerialize(req, NULL, 0) : 0];

    len = (req) ? BRPaymentProtocolRequestSerialize(req, buf0, sizeof(buf0)) : 0;
    if (len > 0)
        r = 0, fprintf(stderr, "***FAILED*** %s: BRPaymentProtocolRequestParse/Serialize() test 3\n", __func__);
    
    printf("                          ");
    return r;
}

void BRPeerAcceptMessageTest(BRPeer *peer, const uint8_t *msg, size_t len, const char *type);

int BRPeerTests()
{
    // TODO: XXXX test for stack overflow
    
    int r = 1;
    BRPeer *p = BRPeerNew();
    const char msg[] = "my message";
    
    BRPeerAcceptMessageTest(p, (const uint8_t *)msg, sizeof(msg) - 1, "inv");
    return r;
}

void syncStarted(void *info)
{
    printf("sync started\n");
}

void syncSucceeded(void *info)
{
    printf("sync succeeded\n");
}

void syncFailed(void *info, int code)
{
    printf("sync failed with code: %d\n", code);
}

void txStatusUpdate(void *info)
{
    printf("transaction status updated\n");
}

int BRRunTests()
{
    int fail = 0;
    
    printf("BRIntsTests...            ");
    printf("%s\n", (BRIntsTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRArrayTests...           ");
    printf("%s\n", (BRArrayTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRListTests...            ");
    printf("%s\n", (BRListTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRBase58Tests...          ");
    printf("%s\n", (BRBase58Tests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRHashTests...            ");
    printf("%s\n", (BRHashTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRKeyTests...             ");
    printf("%s\n", (BRKeyTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRAddressTests...         ");
    printf("%s\n", (BRAddressTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRBIP39MnemonicTests...   ");
    printf("%s\n", (BRBIP39MnemonicTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRBIP32SequenceTests...   ");
    printf("%s\n", (BRBIP32SequenceTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRTransactionTests...     ");
    printf("%s\n", (BRTransactionTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRWalletTests...          ");
    printf("%s\n", (BRWalletTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRBloomFilterTests...     ");
    printf("%s\n", (BRBloomFilterTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRMerkleBlockTests...     ");
    printf("%s\n", (BRMerkleBlockTests()) ? "success" : (fail++, "***FAIL***"));
    printf("BRPaymentProtocolTests... ");
    printf("%s\n", (BRPaymentProtocolTests()) ? "success" : (fail++, "***FAIL***"));
    printf("\n");
    
    if (fail > 0) printf("%d TEST FUNCTION(S) ***FAILED***\n", fail);
    else printf("ALL TESTS PASSED\n");
    
    UInt512 seed = UINT512_ZERO;
    BRMasterPubKey mpk = BR_MASTER_PUBKEY_NONE;
    
    BRBIP39DeriveKey(seed.u8, "video tiger report bid suspect taxi mail argue naive layer metal surface", NULL);
    mpk = BRBIP32MasterPubKey(&seed, sizeof(seed));

    BRWallet *wallet = BRWalletNew(NULL, 0, mpk);

    BRWalletSetCallbacks(wallet, wallet, walletBalanceChanged, walletTxAdded, walletTxUpdated, walletTxDeleted);
    printf("wallet created with first receive address: %s\n", BRWalletReceiveAddress(wallet).s);
    
//    BRPeerManager *manager = BRPeerManagerNew(wallet, BIP39_CREATION_TIME, NULL, 0, NULL, 0);
    BRPeerManager *manager = BRPeerManagerNew(wallet, (uint32_t)time(NULL), NULL, 0, NULL, 0);
    int r = 0;

    BRPeerManagerSetCallbacks(manager, manager, syncStarted, syncSucceeded, syncFailed, txStatusUpdate, NULL, NULL,
                              NULL, NULL);
    BRPeerManagerConnect(manager);
    while (r == 0 && BRPeerManagerPeerCount(manager) > 0) r = sleep(1);
    if (r != 0) printf("sleep got a signal\n");
    BRPeerManagerFree(manager);
    BRWalletFree(wallet);

    return (fail == 0);
}

#ifndef BITCOIN_TEST_NO_MAIN
int main(int argc, const char *argv[])
{
    return (BRRunTests()) ? 0 : 1;
}
#endif