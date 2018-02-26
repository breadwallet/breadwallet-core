//
//  BBREthereumAddress.c
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/21/2018.
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

#include <malloc.h>
#include "BREthereumEther.h"
#include "BREthereumAccount.h"

struct BREthereumAddressRecord {
    UInt160 addr;
};

struct BREthereumAccountRecord {
    int foo;
};

extern BREthereumAccount
accountCreate(/* private key - derived from paper key - can create BIP32 addresses */) {
    BREthereumAccount account = (BREthereumAccount) calloc (1, sizeof (struct BREthereumAccountRecord));
    return account;
}

extern BREthereumAddress
accountCreateAddress(BREthereumAccount account) {
    BREthereumAddress address = calloc (1, sizeof (struct BREthereumAddressRecord));

    // Generate from account.
    return address;
}

extern BREthereumSignature
accountSignBytes(BREthereumAccount account,
                 BREthereumAddress address,
                 BREthereumSignatureType type,
                 uint8_t *bytes,
                 size_t bytesCount) {
    BREthereumSignature signature;

    signature.type = type;

    // TODO: Implement
    switch (type) {
        case SIGNATURE_TYPE_FOO:
            break;
        case SIGNATURE_TYPE_VRS:
            break;
    }
    return signature;
}

