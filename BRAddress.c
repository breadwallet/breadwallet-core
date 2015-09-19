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
                if (off + sizeof(uint8_t) > len) return i;
                l = script[off];
                off += sizeof(uint8_t);
                break;
                
            case OP_PUSHDATA2:
                off++;
                if (off + sizeof(uint16_t) > len) return i;
                l = le16(*(uint16_t *)&script[off]);
                off += sizeof(uint16_t);
                break;
                
            case OP_PUSHDATA4:
                off++;
                if (off + sizeof(uint32_t) > len) return i;
                l = le32(*(uint32_t *)&script[off]);
                off += sizeof(uint32_t);
                break;
                
            default:
                l = (script[off] > OP_PUSHDATA4) ? 0 : script[off];
                off++;
                break;
        }
        
        if (off + l > len) return i;
        off += l;
        i++;
    }
        
    return i;
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

// NOTE: It's important here to be permissive with scriptSig (spends) and strict with scriptPubKey (receives). If we
// miss a receive transaction, only that transaction's funds are missed, however if we accept a receive transaction that
// we are unable to correctly sign later, then the entire wallet balance after that point would become stuck with the
// current coin selection code

BRAddress BRAddressFromScriptPubKey(const uint8_t *script, size_t len)
{
    if (! script || len == 0) return BR_ADDRESS_NONE;
    
    BRAddress address;
    uint8_t data[21];
    size_t l = BRScriptElements(NULL, 0, script, len);
    const uint8_t *elem[l], *d;
    
    data[0] = BITCOIN_PUBKEY_ADDRESS;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PUBKEY_ADDRESS_TEST;
#endif

    BRScriptElements(elem, l, script, len);
    
    if (l == 5 && *elem[0] == OP_DUP && *elem[1] == OP_HASH160 && *elem[2] == 20 && *elem[3] == OP_EQUALVERIFY &&
        *elem[4] == OP_CHECKSIG) {
        // pay-to-pubkey-hash scriptPubKey
        d = BRScriptData(elem[2], &l);
        if (! d || l != 20) return BR_ADDRESS_NONE;
        memcpy(&data[1], d, 20);
    }
    else if (l == 3 && *elem[0] == OP_HASH160 && *elem[1] == 20 && *elem[2] == OP_EQUAL) {
        // pay-to-script-hash scriptPubKey
        data[0] = BITCOIN_SCRIPT_ADDRESS;
#if BITCOIN_TESTNET
        data[0] = BITCOIN_SCRIPT_ADDRESS_TEST;
#endif
        d = BRScriptData(elem[1], &l);
        if (! d || l != 20) return BR_ADDRESS_NONE;
        memcpy(&data[1], d, 20);
    }
    else if (l == 2 && (*elem[0] == 65 || *elem[0] == 33) && *elem[1] == OP_CHECKSIG) {
        // pay-to-pubkey scriptPubKey
        d = BRScriptData(elem[0], &l);
        if (! d || (l != 65 && l != 33)) return BR_ADDRESS_NONE;
        BRHash160(&data[1], d, l);
    }
    else return BR_ADDRESS_NONE; // unknown script type
    
    BRBase58CheckEncode(address.s, sizeof(address), data, sizeof(data));
    return address;
}

BRAddress BRAddressFromScriptSig(const uint8_t *script, size_t len)
{
    if (! script || len == 0) return BR_ADDRESS_NONE;
    
    BRAddress address;
    uint8_t data[21];
    size_t l = BRScriptElements(NULL, 0, script, len);
    const uint8_t *elem[l], *d;

    data[0] = BITCOIN_PUBKEY_ADDRESS;
#if BITCOIN_TESTNET
    data[0] = BITCOIN_PUBKEY_ADDRESS_TEST;
#endif
    
    BRScriptElements(elem, l, script, len);
    
    if (l >= 2 && *elem[l - 2] <= OP_PUSHDATA4 && *elem[l - 2] > 0 && (*elem[l - 1] == 65 || *elem[l - 1] == 33)) {
        // pay-to-pubkey-hash scriptSig
        d = BRScriptData(elem[l - 1], &l);
        if (! d || (l != 65 && l != 33)) return BR_ADDRESS_NONE;
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
        if (! d) return BR_ADDRESS_NONE;
        BRHash160(&data[1], d, l);
    }
    else if (l >= 1 && *elem[l - 1] <= OP_PUSHDATA4 && *elem[l - 1] > 0) {
        // pay-to-pubkey scriptSig
        // TODO: implement Peter Wullie's pubKey recovery from signature
        return BR_ADDRESS_NONE;
    }
    else return BR_ADDRESS_NONE; // unknown script type
    
    BRBase58CheckEncode(address.s, sizeof(address), data, sizeof(data));
    return address;
}

size_t BRAddressScriptPubKey(uint8_t *script, size_t len, const char *addr)
{
    return 0;
}