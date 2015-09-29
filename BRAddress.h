//
//  BRAddress.h
//
//  Created by Aaron Voisine on 9/18/15.
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

#ifndef BRAddress_h
#define BRAddress_h

#include "BRTypes.h"
#include "BRBase58.h"
#include "BRHash.h"
#include <string.h>

#define BITCOIN_PUBKEY_ADDRESS      0
#define BITCOIN_SCRIPT_ADDRESS      5
#define BITCOIN_PUBKEY_ADDRESS_TEST 111
#define BITCOIN_SCRIPT_ADDRESS_TEST 196
#define BITCOIN_PRIVKEY             128
#define BITCOIN_PRIVKEY_TEST        239

// bitcoin script opcodes: https://en.bitcoin.it/wiki/Script#Constants
#define OP_PUSHDATA1   0x4c
#define OP_PUSHDATA2   0x4d
#define OP_PUSHDATA4   0x4e
#define OP_DUP         0x76
#define OP_EQUAL       0x87
#define OP_EQUALVERIFY 0x88
#define OP_HASH160     0xa9
#define OP_CHECKSIG    0xac

uint64_t BRVarInt(const uint8_t *buf, size_t len, size_t *intLen);
size_t BRVarIntSet(uint8_t *buf, size_t len, uint64_t i);
size_t BRVarIntSize(uint64_t i);
size_t BRScriptElements(const uint8_t *elems[], size_t elemsCount, const uint8_t *script, size_t len);
const uint8_t *BRScriptData(const uint8_t *elem, size_t *len);
size_t BRScriptPushData(uint8_t *script, size_t scriptLen, const uint8_t *data, size_t dataLen);

typedef struct {
    char s[36];
} BRAddress;

#define BR_ADDRESS_NONE ((BRAddress) { "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0" })

size_t BRAddressFromScriptPubKey(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen);

size_t BRAddressFromScriptSig(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen);

size_t BRAddressScriptPubKey(uint8_t *script, size_t scriptLen, const char *addr);

inline static UInt160 BRAddressHash160(const char *addr)
{
    uint8_t data[21];
    
    if (BRBase58CheckDecode(data, sizeof(data), addr) != sizeof(data)) return UINT160_ZERO;
    return *(UInt160 *)&data[1];
}

inline static int BRAddressEq(const void *a, const void *b)
{
    return (strncmp(a, b, sizeof(BRAddress)) == 0);
}

inline static size_t BRAddressHash(const void *addr)
{
    return BRMurmur3_32(addr, strlen(addr), 0);
}

#endif // BRAddress_h
