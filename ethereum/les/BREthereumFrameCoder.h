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
#include "BRKey.h"
#include "BRInt.h"
#include "../base/BREthereumBase.h"

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Opaque pointer for a Frame coder
 */
typedef struct BREthereumFrameCoderContext* BREthereumFrameCoder;
    

/**
 * Creates a frame coder
 */
extern BREthereumFrameCoder ethereumFrameCoderCreate(void);
    
/**
 * Initilaize a frame coder
 */
extern BREthereumBoolean ethereumFrameCoderInit(BREthereumFrameCoder fcoder,
                                                BRKey* remoteEphemeral,
                                                UInt256* remoteNonce,
                                                BRKey* localEphemeral,
                                                UInt256* localNonce,
                                                uint8_t* aukCipher,
                                                size_t aukCipherLen,
                                                uint8_t* authCiper,
                                                size_t authCipherLen,
                                                BREthereumBoolean didOriginate);
    
/**
 * Frees the memory of the frame coder 
 */
extern void ethereumFrameCoderRelease(BREthereumFrameCoder coder);

/**
 * Writes a single frame to the coder
 */
 extern void ethereumFrameCoderEncrypt(BREthereumFrameCoder fCoder, uint8_t* payload, size_t payloadSize, uint8_t** rlpBytes, size_t * rlpBytesSize);

/**
 * Authenticates and descrptys the header 
 */
extern BREthereumBoolean ethereumFrameCoderDecryptHeader(BREthereumFrameCoder fCoder, uint8_t * oBytes, size_t outSize);
 
/**
 * Authenticates and descrptys the frame
 */
extern BREthereumBoolean ethereumFrameCoderDecryptFrame(BREthereumFrameCoder fCoder, uint8_t * oBytes, size_t outSize);
  
 BREthereumBoolean _test(BREthereumFrameCoder fCoder, uint8_t * oBytes, size_t outSize);
 
extern int testFrameCoderInitiator(BREthereumFrameCoder fCoder);
extern int testFrameCoderReceiver(BREthereumFrameCoder fCoder);


 
#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Frame_Coder_h */
