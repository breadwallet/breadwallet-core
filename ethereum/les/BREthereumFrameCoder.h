//
//  BREthereumFrameCoder.h
//  breadwallet-core Ethereum
//
//  Created by Lamont Samuels on 4/16/18.
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

#ifndef BR_Ethereum_Frame_Coder_h
#define BR_Ethereum_Frame_Coder_h

#include <inttypes.h>
#include "BREthereumHandshake.h"
#include "BRKey.h"
#include "BRInt.h"
#include "BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Opaque pointer for a Frame coder
 */
typedef struct BREthereumFrameCoderContext* BREthereumFrameCoder;


/**
 * Creates and initializes a packet framer
 */
BREthereumFrameCoder ethereumFrameCoderCreate(UInt512* remoteEphemeral,
                                              UInt256* remoteNonce,
                                              BRKey* ecdheLocal,
                                              UInt256* localNonce,
                                              uint8_t* aukCipher,
                                              size_t aukCipherLen,
                                              uint8_t* authCiper,
                                              size_t authCipherLen,
                                              BREthereumBoolean didOriginate);
    

/**
 * Writes a single frame to the coder
 */
 void ethereumFrameCoderWrite(BREthereumFrameCoder coder, uint8_t* payload, uint8_t* oFrame);
 
 
#ifdef __cplusplus
}
#endif
#endif /* BR_Ethereum_Frame_Coder_h */
