//
//  BRBIP38Key.h
//
//  Created by Aaron Voisine on 9/7/15.
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

#ifndef BRBIP38Key_h
#define BRBIP38Key_h

#include "BRKey.h"

// BIP38 is a method for encrypting private keys with a passphrase
// https://github.com/bitcoin/bips/blob/master/bip-0038.mediawiki

// decrypts a BIP38 key using the given passphrase, returns false if passphrase is incorrect, passphrase must be unicode
// NFC normalized
int BRKeySetBIP38Key(BRKey *key, const char *bip38Key, const char *passphrase);

// generates an "intermediate code" for an EC multiply mode key, salt should be 64bits of random data, returns number of
// bytes written to code including NULL terminator, or total bytes needed if code is NULL, passphrase must be unicode
// NFC normalized
size_t BRKeyBIP38ItermediateCode(char *code, size_t codeLen, uint64_t salt, const char *passphrase);

// generates an "intermediate code" for an EC multiply mode key with a lot and sequence number, lot must be less than
// 1048576, sequence must be less than 4096, and salt should be 32bits of random data, returns number of bytes written
// to code including NULL terminator, or total bytes needed if code is NULL, passphrase must be unicode NFC normalized
size_t BRKeyBIP38ItermediateCodeLS(char *code, size_t codeLen, uint32_t lot, uint16_t sequence, uint32_t salt,
                                   const char *passphrase);

// generates a BIP38 key from an "intermediate code" and 24 bytes of cryptographically random data (seedb),
// compressed indicates if compressed pubKey format should be used for the bitcoin address, confcode will be set to the
// "confirmation code", returns number of bytes written to confcode including NULL terminator, or total size needed if
// confcode is NULL
size_t BRKeySetBIP38ItermediateCode(BRKey *key, const char *code, uint8_t *seedb, int compressed, char *confcode);

// returns true if the "confirmation code" confirms that the given bitcoin address depends on the specified passphrase,
// passphrase must be unicode NFC normalized
int BRKeyBIP38Confirm(const char *code, const char *address, const char *passphrase);

// encrypts key with passphrase, returns number of bytes written to bip38Key including NULL terminator, or total bytes
// needed if bip38Key is NULL, passphrase must be unicode NFC normalized
size_t BRKeyBIP38Key(BRKey *key, char *bip38Key, const char *passphrase);

#endif // BRBIP38Key_h
