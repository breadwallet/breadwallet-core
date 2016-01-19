//
//  BRAddress.c
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

#include "BRAddress.h"
#include "BRBase58.h"
#include "BRInt.h"

#define VAR_INT16_HEADER 0xfd
#define VAR_INT32_HEADER 0xfe
#define VAR_INT64_HEADER 0xff

uint64_t BRVarInt(const uint8_t *buf, size_t len, size_t *intLen)
{
    uint64_t r = 0;
    uint8_t h = (sizeof(uint8_t) <= len) ? *(const uint8_t *)buf : 0;
    
    switch (h) {
        case VAR_INT16_HEADER:
            if (intLen) *intLen = sizeof(h) + sizeof(uint16_t);
            r = (sizeof(h) + sizeof(uint16_t) <= len) ? le16(*(const uint16_t *)(buf + sizeof(h))) : 0;
            break;
            
        case VAR_INT32_HEADER:
            if (intLen) *intLen = sizeof(h) + sizeof(uint32_t);
            r = (sizeof(h) + sizeof(uint32_t) <= len) ? le32(*(const uint32_t *)(buf + sizeof(h))) : 0;
            break;
            
        case VAR_INT64_HEADER:
            if (intLen) *intLen = sizeof(h) + sizeof(uint64_t);
            r = (sizeof(h) + sizeof(uint64_t) <= len) ? le64(*(const uint64_t *)(buf + sizeof(h))) : 0;
            break;
            
        default:
            if (intLen) *intLen = sizeof(h);
            r = h;
            break;
    }
    
    return r;
}

size_t BRVarIntSet(uint8_t *buf, size_t len, uint64_t i)
{
    size_t r = 0;
    
    if (i < VAR_INT16_HEADER) {
        if (buf && sizeof(uint8_t) <= len) *buf = (uint8_t)i;
        r = (! buf || sizeof(uint8_t) <= len) ? sizeof(uint8_t) : 0;
    }
    else if (i <= UINT16_MAX) {
        if (buf && sizeof(uint8_t) + sizeof(uint16_t) <= len) {
            *buf = VAR_INT16_HEADER;
            *(uint16_t *)(buf + sizeof(uint8_t)) = le16((uint16_t)i);
        }
        
        r = (! buf || sizeof(uint8_t) + sizeof(uint16_t) <= len) ? sizeof(uint8_t) + sizeof(uint16_t) : 0;
    }
    else if (i <= UINT32_MAX) {
        if (buf && sizeof(uint8_t) + sizeof(uint32_t) <= len) {
            *buf = VAR_INT32_HEADER;
            *(uint32_t *)(buf + sizeof(uint8_t)) = le32((uint32_t)i);
        }
        
        r = (! buf || sizeof(uint8_t) + sizeof(uint32_t) <= len) ? sizeof(uint8_t) + sizeof(uint32_t) : 0;
    }
    else {
        if (buf && sizeof(uint8_t) + sizeof(uint64_t) <= len) {
            *buf = VAR_INT64_HEADER;
            *(uint64_t *)(buf + sizeof(uint8_t)) = le64(i);
        }
        
        r = (! buf || sizeof(uint8_t) + sizeof(uint64_t) <= len) ? sizeof(uint8_t) + sizeof(uint64_t) : 0;
    }
    
    return r;
}

size_t BRVarIntSize(uint64_t i)
{
    return BRVarIntSet(NULL, 0, i);
}

size_t BRScriptElements(const uint8_t *elems[], size_t elemsCount, const uint8_t *script, size_t len)
{
    size_t off = 0, i = 0, l = 0;
    
    while (off < len) {
        if (elems && i < elemsCount) elems[i] = &script[off];
        
        switch (script[off]) {
            case OP_PUSHDATA1:
                off++;
                if (off + sizeof(uint8_t) <= len) l = script[off];
                off += sizeof(uint8_t);
                break;
                
            case OP_PUSHDATA2:
                off++;
                if (off + sizeof(uint16_t) <= len) l = le16(*(uint16_t *)&script[off]);
                off += sizeof(uint16_t);
                break;
                
            case OP_PUSHDATA4:
                off++;
                if (off + sizeof(uint32_t) <= len) l = le32(*(uint32_t *)&script[off]);
                off += sizeof(uint32_t);
                break;
                
            default:
                l = (script[off] > OP_PUSHDATA4) ? 0 : script[off];
                off++;
                break;
        }
        
        off += l;
        i++;
    }
        
    return (off == len) ? i : 0;
}

const uint8_t *BRScriptData(const uint8_t *elem, size_t *len)
{
    switch (*elem) {
        case OP_PUSHDATA1:
            elem++;
            *len = *elem;
            elem += sizeof(uint8_t);
            break;
            
        case OP_PUSHDATA2:
            elem++;
            *len = le16(*(uint16_t *)elem);
            elem += sizeof(uint16_t);
            break;
            
        case OP_PUSHDATA4:
            elem++;
            *len = le32(*(uint32_t *)elem);
            elem += sizeof(uint32_t);
            break;
            
        default:
            *len = (*elem > OP_PUSHDATA4) ? 0 : *elem;
            elem++;
            break;
    }
    
    return (*len > 0) ? elem : NULL;
}

size_t BRScriptPushData(uint8_t *script, size_t scriptLen, const uint8_t *data, size_t dataLen)
{
    size_t len;

    if (dataLen == 0) {
        len = dataLen;
    }
    else if (dataLen < OP_PUSHDATA1) {
        len = 1 + dataLen;
        if (script && len <= scriptLen) script[0] = dataLen;
    }
    else if (dataLen < UINT8_MAX) {
        len = 1 + sizeof(uint8_t) + dataLen;
        
        if (script && len <= scriptLen) {
            script[0] = OP_PUSHDATA1;
            script[1] = dataLen;
        }
    }
    else if (dataLen < UINT16_MAX) {
        len = 1 + sizeof(uint16_t) + dataLen;
        
        if (script && len <= scriptLen) {
            script[0] = OP_PUSHDATA2;
            *(uint16_t *)&script[1] = le16((uint16_t)dataLen);
        }
    }
    else {
        len = 1 + sizeof(uint32_t) + dataLen;
        
        if (script && len <= scriptLen) {
            script[0] = OP_PUSHDATA4;
            *(uint32_t *)&script[1] = le32((uint32_t)dataLen);
        }
    }
    
    if (script && len <= scriptLen) memcpy(script + len - dataLen, data, dataLen);
    return (! script || len <= scriptLen) ? len : 0;
}

// NOTE: It's important here to be permissive with scriptSig (spends) and strict with scriptPubKey (receives). If we
// miss a receive transaction, only that transaction's funds are missed, however if we accept a receive transaction that
// we are unable to correctly sign later, then the entire wallet balance after that point would become stuck with the
// current coin selection code

size_t BRAddressFromScriptPubKey(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen)
{
    if (! script || scriptLen == 0) return 0;
    
    uint8_t data[21];
    const uint8_t *elem[BRScriptElements(NULL, 0, script, scriptLen)], *d = NULL;
    size_t len = BRScriptElements(elem, sizeof(elem), script, scriptLen);
    
    data[0] = BITCOIN_PUBKEY_ADDRESS;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PUBKEY_ADDRESS_TEST;
#endif
    
    if (len == 5 && *elem[0] == OP_DUP && *elem[1] == OP_HASH160 && *elem[2] == 20 && *elem[3] == OP_EQUALVERIFY &&
        *elem[4] == OP_CHECKSIG) {
        // pay-to-pubkey-hash scriptPubKey
        d = BRScriptData(elem[2], &len);
        if (len != 20) d = NULL;
        if (d) memcpy(&data[1], d, 20);
    }
    else if (len == 3 && *elem[0] == OP_HASH160 && *elem[1] == 20 && *elem[2] == OP_EQUAL) {
        // pay-to-script-hash scriptPubKey
        data[0] = BITCOIN_SCRIPT_ADDRESS;
#if BITCOIN_TESTNET
        data[0] = BITCOIN_SCRIPT_ADDRESS_TEST;
#endif
        d = BRScriptData(elem[1], &len);
        if (len != 20) d = NULL;
        if (d) memcpy(&data[1], d, 20);
    }
    else if (len == 2 && (*elem[0] == 65 || *elem[0] == 33) && *elem[1] == OP_CHECKSIG) {
        // pay-to-pubkey scriptPubKey
        d = BRScriptData(elem[0], &len);
        if (len != 65 && len != 33) d = NULL;
        if (d) BRHash160(&data[1], d, len);
    }
    
    return (d) ? BRBase58CheckEncode(addr, addrLen, data, sizeof(data)) : 0;
}

size_t BRAddressFromScriptSig(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen)
{
    if (! script || scriptLen == 0) return 0;
    
    uint8_t data[21];
    const uint8_t *elem[BRScriptElements(NULL, 0, script, scriptLen)], *d = NULL;
    size_t l = BRScriptElements(elem, sizeof(elem), script, scriptLen);

    data[0] = BITCOIN_PUBKEY_ADDRESS;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PUBKEY_ADDRESS_TEST;
#endif
    
    if (l >= 2 && *elem[l - 2] <= OP_PUSHDATA4 && *elem[l - 2] > 0 && (*elem[l - 1] == 65 || *elem[l - 1] == 33)) {
        // pay-to-pubkey-hash scriptSig
        d = BRScriptData(elem[l - 1], &l);
        if (l != 65 && l != 33) d = NULL;
        if (d) BRHash160(&data[1], d, l);
    }
    else if (l >= 2 && *elem[l - 2] <= OP_PUSHDATA4 && *elem[l - 2] > 0 && *elem[l - 1] <= OP_PUSHDATA4 &&
             *elem[l - 1] > 0) {
        // pay-to-script-hash scriptSig
        data[0] = BITCOIN_SCRIPT_ADDRESS;
#if BITCOIN_TESTNET
        data[0] = BITCOIN_SCRIPT_ADDRESS_TEST;
#endif
        d = BRScriptData(elem[l - 1], &l);
        if (d) BRHash160(&data[1], d, l);
    }
    else if (l >= 1 && *elem[l - 1] <= OP_PUSHDATA4 && *elem[l - 1] > 0) {
        // pay-to-pubkey scriptSig
        // TODO: implement Peter Wullie's pubKey recovery from signature
    }
    
    return (d) ? BRBase58CheckEncode(addr, addrLen, data, sizeof(data)) : 0;
}

size_t BRAddressScriptPubKey(uint8_t *script, size_t len, const char *addr)
{
    static uint8_t pubkeyAddress = BITCOIN_PUBKEY_ADDRESS, scriptAddress = BITCOIN_SCRIPT_ADDRESS;
    uint8_t data[21];
    size_t r = 0;
    
#if BITCOIN_TESTNET
    pubkeyAddress = BITCOIN_PUBKEY_ADDRESS_TEST;
    scriptAddress = BITCOIN_SCRIPT_ADDRESS_TEST;
#endif
    
    if (BRBase58CheckDecode(data, sizeof(data), addr) == 21) {
        if (data[0] == pubkeyAddress) {
            if (script && 25 <= len) {
                script[0] = OP_DUP;
                script[1] = OP_HASH160;
                script[2] = 20;
                memcpy(&script[3], &data[1], 20);
                script[23] = OP_EQUALVERIFY;
                script[24] = OP_CHECKSIG;
            }
            
            r = (! script || 25 <= len) ? 25 : 0;
        }
        else if (data[0] == scriptAddress) {
            if (script && 23 <= len) {
                script[0] = OP_HASH160;
                script[1] = 20;
                memcpy(&script[2], &data[1], 20);
                script[22] = OP_EQUAL;
            }
            
            r = (! script || 23 <= len) ? 23 : 0;
        }
    }

    return r;
}

int BRAddressIsValid(const char *addr)
{
    uint8_t data[21];
    int r = 0;
    
    if (BRBase58CheckDecode(data, sizeof(data), addr) == 21) {
        r = (data[0] == BITCOIN_PUBKEY_ADDRESS || data[0] == BITCOIN_SCRIPT_ADDRESS);
    
#if BITCOIN_TESTNET
        r = (data[0] == BITCOIN_PUBKEY_ADDRESS_TEST || data[0] == BITCOIN_SCRIPT_ADDRESS_TEST);
#endif
    }
    
    return r;
}

int BRAddressHash160(void *md, const char *addr)
{
    uint8_t data[21];
    int r = 0;
    
    r = (BRBase58CheckDecode(data, sizeof(data), addr) == 21);
    if (r) memcpy(md, &data[1], 20);
    return r;
}
