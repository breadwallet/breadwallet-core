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

size_t BRScriptElements(const uint8_t **elems, size_t elemsCount, const uint8_t *script, size_t len)
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

BRAddress BRAddressSetScriptPubKey(const uint8_t *script, size_t len)
{
    return BR_ADDRESS_NONE;
}

BRAddress BRAddressSetScriptSig(const uint8_t *script, size_t len)
{
    return BR_ADDRESS_NONE;
}

size_t BRAddressScriptPubKey(uint8_t *script, size_t len, const char *addr)
{
    return 0;
}