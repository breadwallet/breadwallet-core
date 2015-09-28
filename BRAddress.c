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

size_t BRScriptElements(const uint8_t *elems[], size_t elemsCount, const uint8_t *script, size_t len)
{
    size_t off = 0, i = 0, l;
    
    while (off < len) {
        if (elems) {
            if (i >= elemsCount) return 0;
            elems[i] = &script[off];
        }
        
        switch (script[off]) {
            case OP_PUSHDATA1:
                off++;
                if (off + sizeof(uint8_t) > len) return 0;
                l = script[off];
                off += sizeof(uint8_t);
                break;
                
            case OP_PUSHDATA2:
                off++;
                if (off + sizeof(uint16_t) > len) return 0;
                l = le16(*(uint16_t *)&script[off]);
                off += sizeof(uint16_t);
                break;
                
            case OP_PUSHDATA4:
                off++;
                if (off + sizeof(uint32_t) > len) return 0;
                l = le32(*(uint32_t *)&script[off]);
                off += sizeof(uint32_t);
                break;
                
            default:
                l = (script[off] > OP_PUSHDATA4) ? 0 : script[off];
                off++;
                break;
        }
        
        if (off + l > len) return 0;
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
        if (script && scriptLen >= len) script[0] = dataLen;
    }
    else if (dataLen < UINT8_MAX) {
        len = 1 + sizeof(uint8_t) + dataLen;
        
        if (script && scriptLen >= len) {
            script[0] = OP_PUSHDATA1;
            script[1] = dataLen;
        }
    }
    else if (dataLen < UINT16_MAX) {
        len = 1 + sizeof(uint16_t) + dataLen;
        
        if (script && scriptLen >= len) {
            script[0] = OP_PUSHDATA2;
            *(uint16_t *)&script[1] = le16((uint16_t)dataLen);
        }
    }
    else {
        len = 1 + sizeof(uint32_t) + dataLen;
        
        if (script && scriptLen >= len) {
            script[0] = OP_PUSHDATA4;
            *(uint32_t *)&script[1] = le32((uint32_t)dataLen);
        }
    }
    
    if (script && scriptLen >= len) memcpy(script + len - dataLen, data, dataLen);
    return len;
}

// NOTE: It's important here to be permissive with scriptSig (spends) and strict with scriptPubKey (receives). If we
// miss a receive transaction, only that transaction's funds are missed, however if we accept a receive transaction that
// we are unable to correctly sign later, then the entire wallet balance after that point would become stuck with the
// current coin selection code

size_t BRAddressFromScriptPubKey(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen)
{
    if (! script || scriptLen == 0) return 0;
    
    uint8_t data[21];
    size_t l = BRScriptElements(NULL, 0, script, scriptLen);
    const uint8_t *elem[l], *d;
    
    data[0] = BITCOIN_PUBKEY_ADDRESS;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PUBKEY_ADDRESS_TEST;
#endif

    BRScriptElements(elem, l, script, scriptLen);
    
    if (l == 5 && *elem[0] == OP_DUP && *elem[1] == OP_HASH160 && *elem[2] == 20 && *elem[3] == OP_EQUALVERIFY &&
        *elem[4] == OP_CHECKSIG) {
        // pay-to-pubkey-hash scriptPubKey
        d = BRScriptData(elem[2], &l);
        if (! d || l != 20) return 0;
        memcpy(&data[1], d, 20);
    }
    else if (l == 3 && *elem[0] == OP_HASH160 && *elem[1] == 20 && *elem[2] == OP_EQUAL) {
        // pay-to-script-hash scriptPubKey
        data[0] = BITCOIN_SCRIPT_ADDRESS;
#if BITCOIN_TESTNET
        data[0] = BITCOIN_SCRIPT_ADDRESS_TEST;
#endif
        d = BRScriptData(elem[1], &l);
        if (! d || l != 20) return 0;
        memcpy(&data[1], d, 20);
    }
    else if (l == 2 && (*elem[0] == 65 || *elem[0] == 33) && *elem[1] == OP_CHECKSIG) {
        // pay-to-pubkey scriptPubKey
        d = BRScriptData(elem[0], &l);
        if (! d || (l != 65 && l != 33)) return 0;
        BRHash160(&data[1], d, l);
    }
    else return 0; // unknown script type
    
    return BRBase58CheckEncode(addr, addrLen, data, sizeof(data));
}

size_t BRAddressFromScriptSig(char *addr, size_t addrLen, const uint8_t *script, size_t scriptLen)
{
    if (! script || scriptLen == 0) return 0;
    
    uint8_t data[21];
    size_t l = BRScriptElements(NULL, 0, script, scriptLen);
    const uint8_t *elem[l], *d;

    data[0] = BITCOIN_PUBKEY_ADDRESS;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PUBKEY_ADDRESS_TEST;
#endif
    
    BRScriptElements(elem, l, script, scriptLen);
    
    if (l >= 2 && *elem[l - 2] <= OP_PUSHDATA4 && *elem[l - 2] > 0 && (*elem[l - 1] == 65 || *elem[l - 1] == 33)) {
        // pay-to-pubkey-hash scriptSig
        d = BRScriptData(elem[l - 1], &l);
        if (! d || (l != 65 && l != 33)) return 0;
        BRHash160(&data[1], d, l);
    }
    else if (l >= 2 && *elem[l - 2] <= OP_PUSHDATA4 && *elem[l - 2] > 0 && *elem[l - 1] <= OP_PUSHDATA4 &&
             *elem[l - 1] > 0) {
        // pay-to-script-hash scriptSig
        data[0] = BITCOIN_SCRIPT_ADDRESS;
#if BITCOIN_TESTNET
        data[0] = BITCOIN_SCRIPT_ADDRESS_TEST;
#endif
        d = BRScriptData(elem[l - 1], &l);
        if (! d) return 0;
        BRHash160(&data[1], d, l);
    }
    else if (l >= 1 && *elem[l - 1] <= OP_PUSHDATA4 && *elem[l - 1] > 0) {
        // pay-to-pubkey scriptSig
        // TODO: implement Peter Wullie's pubKey recovery from signature
        return 0;
    }
    else return 0; // unknown script type
    
    return BRBase58CheckEncode(addr, addrLen, data, sizeof(data));
}

size_t BRAddressScriptPubKey(uint8_t *script, size_t len, const char *addr)
{
    static uint8_t pubkeyAddress = BITCOIN_PUBKEY_ADDRESS, scriptAddress = BITCOIN_SCRIPT_ADDRESS;
    uint8_t data[21];
    
    if (BRBase58CheckDecode(data, sizeof(data), addr) != sizeof(data)) return 0;
    
#if BITCOIN_TESTNET
    pubkeyAddress = BITCOIN_PUBKEY_ADDRESS_TEST;
    scriptAddress = BITCOIN_SCRIPT_ADDRESS_TEST;
#endif
    
    if (data[0] == pubkeyAddress) {
        if (script && 25 <= len) {
            script[0] = OP_DUP;
            script[1] = OP_HASH160;
            script[2] = 20;
            memcpy(&script[3], &data[1], 20);
            script[23] = OP_EQUALVERIFY;
            script[24] = OP_CHECKSIG;
        }

        return (! script || 25 <= len) ? 25 : 0;
    }
    else if (data[0] == scriptAddress) {
        if (script && 23 <= len) {
            script[0] = OP_HASH160;
            script[1] = 20;
            memcpy(&script[2], &data[1], 20);
            script[22] = OP_EQUAL;
        }
        
        return (! script || 23 <= len) ? 23 : 0;
    }
    else return 0;
}