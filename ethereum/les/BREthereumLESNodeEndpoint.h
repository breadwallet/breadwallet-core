//
//  BREthereumLESNodeEndpoint.h
//  Core
//
//  Created by Ed Gamble on 8/14/18.
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

#ifndef BR_Ethereum_LES_Node_Endpoint_H
#define BR_Ethereum_LES_Node_Endpoint_H

#include "BRKey.h"
#include "BRInt.h"
#include "BREthereumLESMessage.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_HOST_NAME 1024

//
// Endpoint
//
typedef struct {
    /** The ADDR_FAMILY (e.g. AF_INET, see <socket.h> - BE encoded (??) */
    int addr_family;  //  BE encoded 4-byte or 16-byte address (size determines ipv4 vs ipv6)
    
    /** */
    char hostname[MAX_HOST_NAME];
    
    /** Determines whether the address is a IPV4 (True) or IPV6 (False) */
    int isIPV4Address;
    
    /** BE encoded (??) */
    uint16_t portUDP;
    volatile int socketUDP;
    
    /** BE Encoded (??) */
    uint16_t portTCP;
    volatile int socketTCP;
    
    /** */
    uint64_t timestamp;
    
    /** Public Key (for the remote peer ) */
    BRKey key;
    
    /** The ephemeral public key */
    BRKey ephemeralKey;
    
    /** The nonce */
    UInt256 nonce;
    
    /** The Hello PSP message */
    BREthereumP2PMessage hello;     // BREthereumP2PMessageHello
    
    /** The Status LES message */
    BREthereumLESMessage status;    // BREthereumLESMessageStatus
    
} BREthereumLESNodeEndpoint;

// Use for 'local
extern BREthereumLESNodeEndpoint
nodeEndpointCreateRaw (const char *address,
                       uint16_t portUDP,
                       uint16_t portTCP,
                       BRKey key,
                       BRKey ephemeralKey,
                       UInt256 nonce);

extern BREthereumLESNodeEndpoint
nodeEndpointCreate (const char *address,
                    uint16_t portUDP,
                    uint16_t portTCP,
                    BRKey key);

extern void
nodeEndpointSetHello (BREthereumLESNodeEndpoint *endpoint,
                      BREthereumP2PMessage hello);

extern void
nodeEndpointSetStatus (BREthereumLESNodeEndpoint *endpoint,
                       BREthereumLESMessage status);

extern void
nodeEndpointOpen (BREthereumLESNodeEndpoint *endpoint);

extern void
nodeEndpointClose (BREthereumLESNodeEndpoint *endpoint);

extern int
nodeEndpointIsOpen (BREthereumLESNodeEndpoint *endpoint);

extern int
nodeEndpointHasRecvDataAvailable (BREthereumLESNodeEndpoint *endpoint,
                                  fd_set *readFds);

extern void
nodeEndpointSetRecvDataAvailableFDS (BREthereumLESNodeEndpoint *endpoint,
                                     fd_set *readFds);

extern void
nodeEndpointClrRecvDataAvailableFDS (BREthereumLESNodeEndpoint *endpoint,
                                     fd_set *readFds);

extern int
nodeEndpointGetRecvDataAvailableFDSNum (BREthereumLESNodeEndpoint *endpoint);

extern int
nodeEndpointRecvData (BREthereumLESNodeEndpoint *endpoint,
                      int socket,
                      uint8_t *bytes,
                      size_t *bytesCount,
                      int needBytesCount);

extern int
nodeEndpointSendData (BREthereumLESNodeEndpoint *endpoint,
                      int socket,
                      uint8_t *bytes,
                      size_t bytesCount);

#ifdef __cplusplus
}
#endif

#endif /* BR_Ethereum_LES_Node_Endpoint_H */
