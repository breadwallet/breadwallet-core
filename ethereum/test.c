//
//  test
//  breadwallet-core Ethereum
//
//  Created by Ed Gamble on 2/27/18.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

#include "BREthereum.h"
#include "../BRBIP39WordsEn.h"

#define SOME_RANDOM_TEST_PAPER_KEY "axis husband project any sea patch drip tip spirit tide bring belt"

void runAddressTests (BREthereumAddress address) {
    printf ("==== Address\n");
    printf ("    PaperKey: %s\n", SOME_RANDOM_TEST_PAPER_KEY);
//    printf ("  PrivateKey: %s\n", addressGetPrivateKeyString((address)));
    printf ("   PublicKey: %s\n", addressGetPublicKeyString(address));

//    (Ethereum) Root: wM5uZBNTYmaYGiK8VbcFnjp31nkcGkCmemfmeVBwHDCMVh7JeKrMqz7othgry4FYPcnQNsuGzs91FV9vkxMDA2PoQXPPx6v7w9nHUqD3N8imMkSo
//
//    Extended Private:
//    wM5uZBVNx184sk3iVjZ3g3jPf6vv7mfSXKynrjFoCRnfYihbs7UopynDPikFUrX6P9BNrWcf2rWQUi9PSP7Ln4DPCz9X9LdEkEz5pAWrXA9EAJvV
//
//    Extended Public:
//    wM5uZBVNx184sk3iVjZ3g3jPf6vv7mfSXKynrjFoCRnfYihbs7UopynDPikFUrdfrgJyyjXKnW7ZvcXySBPhTTrRLZPY8JDrs8kLTAG89rLNA1x7
//
//    Index 0:
//    address:		0xe8109e4bf446EE7ad86bbf90F4a25998B7aF771c
//    public key:   0x02d760a7701dd88c48b1918a77ec81c638a348c6c9028a43c31c78d4e33c8dd308
//    private key:	0x13bed3cd2077015d7a2ebeba041c5013b942ae2236f9861e3d424c14735ffd83

}

void runAccountTests () {
    BREthereumAccount account = accountCreate(SOME_RANDOM_TEST_PAPER_KEY);
    runAddressTests(accountGetPrimaryAddress(account));
}

void runTests () {
    installSharedWordList(BRBIP39WordsEn, BIP39_WORDLIST_COUNT);
    runAccountTests();
}

int main(int argc, const char *argv[]) {
    runTests();
}


