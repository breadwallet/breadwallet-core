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
 * @return BREthereumFrameCoder context
 * @post - this pointer needs to be released by the "ethereumFrameCoderRelease" function
 */
extern BREthereumFrameCoder ethereumFrameCoderCreate(void);
    
/**
 * Initilaize a frame coder context
 * @param fcoder - the context that will be initialized
 * @param remoteEphemeral - the emphemeral public key from a peer node
 * @param remoteNonce - the public nonce from a peer node
 * @param localEphemeral - the local emphemeral key
 * @param localNonce - the local nonce
 * @param aukCipher - the acknowledgement cipher buffer
 * @param aukCipherLen - the acknowledgement cipher buffer length
 * @param authCiper - the authentication cipher buffer
 * @param authCipherLen - the authentication cipher buffer length
 * @param didOriginate - Determines who initiated the handshake process, ETHEREUM_BOOLEAN_TRUE if the local node initiated the
   handshake process, otherwise ETHEREUM_BOOLEAN_FALSE (remote initiated handshake).
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
 * @param coder - the frame coder context
 */
extern void ethereumFrameCoderRelease(BREthereumFrameCoder coder);

/**
 * Encrypts a single packet that will be passed on the ethereum network
 * @param fCoder - the frame coder context
 * @param payload - the payload that will be encrypted
 * @param payloadSize - the size in bytes of the payload
 * @param rlpBytes - a pointer to the destination to store the encrypted packet
 * @param rlpBytesSize - the size in bytes of the encrypted payload
 */
 extern void ethereumFrameCoderEncrypt(BREthereumFrameCoder fCoder, uint8_t* payload, size_t payloadSize, uint8_t** rlpBytes, size_t * rlpBytesSize);

/**
 * Authenticates and decrypts the header from a packet
 * @param fCoder - the frame coder context
 * @param oBytes - the input/output bytes that will be authenticated and decrypted header
 * @param outSize - the size in bytes of the oBytes size
 * @return ETHEREUM_BOOLEAN_TRUE if the header passed authentication, otherwise ETHEREM_BOOLEAN_FALSE
 */
extern BREthereumBoolean ethereumFrameCoderDecryptHeader(BREthereumFrameCoder fCoder, uint8_t * oBytes, size_t outSize);
 
/**
 * Authenticates and decrypts the frame body from a packet
 * @param oBytes - the input/output bytes that will be authenticated and decrypted frame
 * @param outSize - the size in bytes of the oBytes size
 * @return ETHEREUM_BOOLEAN_TRUE if the frame passed authentication, otherwise ETHEREM_BOOLEAN_FALSE
 */
extern BREthereumBoolean ethereumFrameCoderDecryptFrame(BREthereumFrameCoder fCoder, uint8_t * oBytes, size_t outSize);
  
  
///////
/**
 * TODO: These really should be private functions for testing the frame coder
 */
BREthereumBoolean _test(BREthereumFrameCoder fCoder, uint8_t * oBytes, size_t outSize);
extern int testFrameCoderInitiator(BREthereumFrameCoder fCoder);
extern int testFrameCoderReceiver(BREthereumFrameCoder fCoder);
///////
 
#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_Frame_Coder_h */
