//
//  BREthereumLESBase.c
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/24/18.
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

#include <pthread.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include "BRKey.h"
#include "BREthereumLESBase.h"
#include "../base/BREthereumBase.h"
#include "BRCrypto.h"

//
// TODO: Don't really need this function. We can just XOR on union data of the Big Ints
//
void ethereumXORBytes(uint8_t * op1, uint8_t* op2, uint8_t* result, size_t len) {
    for (unsigned int i = 0; i < len;  ++i) {
        result[i] = op1[i] ^ op2[i];
    }

}


