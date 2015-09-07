//
//  BRBIP39Mnemonic.h
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

#ifndef BRBIP39Mnemonic_h
#define BRBIP39Mnemonic_h

#include <stddef.h>

// BIP39 is method for generating a deterministic wallet seed from a mnemonic phrase
// https://github.com/bitcoin/bips/blob/master/bip-0039.mediawiki

#define BIP39_CREATION_TIME (1388534400.0 - NSTimeIntervalSince1970)

size_t BRBIP39Encode(char *phrase, size_t plen, const char *wordlist[], const void *data, size_t dlen);
size_t BRBIP39Decode(void *data, size_t dlen, const char *wordlist[], const char *phrase);
int BRBIP39PhraseIsValid(const char *wordlist[], const char *phrase);

// phrase and passphrase must be unicode NFKD normalized, key must hold 64 bytes
void BIP39DeriveKey(void *key, const char *phrase, const char *passphrase);

#endif // BRBIP39Mnemonic_h
