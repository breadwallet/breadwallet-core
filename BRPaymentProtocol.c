//
//  BRPaymentProtocol.c
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

#include "BRPaymentProtocol.h"

// buf must contain a serialized details struct, result must be freed by calling BRPayementProtocolDetailsFree()
BRPaymentProtocolDetails *BRPaymentProtocolDetailsDeserialize(const uint8_t *buf, size_t len)
{
    return NULL;
}

// writes serialized details struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolDetailsSerialize(BRPaymentProtocolDetails *details, uint8_t *buf, size_t len)
{
    return 0;
}

// frees memory allocated for details struct
void BRPayementProtocolDetailsFree(BRPaymentProtocolDetails *details)
{
}

// buf must contain a serialized request struct, result must be freed by calling BRPaymentProtocolRequestFree()
BRPaymentProtocolRequest *BRPaymentProtocolRequestDeserialize(const uint8_t *buf, size_t len)
{
    return NULL;
}

// writes serialized request struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolRrequestSerialize(BRPaymentProtocolRequest *request, uint8_t *buf, size_t len)
{
    return 0;
}

// writes the DER encoded certificate corresponding to index to cert, returns the number of bytes written to cert, or
// the total certLen needed if cert is NULL, returns 0 if index of out-of-bounds
size_t BRPaymentProtocolRequestCert(BRPaymentProtocolRequest *request, uint8_t *cert, size_t certLen, size_t index)
{
    return 0;
}

// writes the hash of the request to md needed to sign or verify the request, returns the number of bytes written, or
// the total bytes needed if md is NULL
size_t BRPaymentProtocolRequestDigest(BRPaymentProtocolRequest *request, uint8_t *md, size_t mdLen)
{
    return 0;
}

// frees memory allocated for request struct
void BRPayementProtocolRequestFree(BRPaymentProtocolRequest *request)
{
}

// buf must contain a serialized payment struct, result must be freed by calling BRPayementProtocolPaymentFree()
BRPaymentProtocolPayment *BRPaymentProtocolPaymentDeserialize(const uint8_t *buf, size_t len)
{
    return NULL;
}

// writes serialized payment struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolPaymentSerialize(BRPaymentProtocolPayment *payment, uint8_t *buf, size_t len)
{
    return 0;
}

// frees memory allocated for payment struct
void BRPayementProtocolPaymentFree(BRPaymentProtocolPayment *payment)
{
}

// buf must contain a serialized ACK struct, result must be freed by calling BRPayementProtocolACKFree()
BRPaymentProtocolPayment *BRPaymentProtocolACKDeserialize(const uint8_t *buf, size_t len)
{
    return NULL;
}

// writes serialized ACK struct to buf, returns number of bytes written, or total len needed if buf is NULL
size_t BRPaymentProtocolACKSerialize(BRPaymentProtocolPayment *payment, uint8_t *buf, size_t len)
{
    return 0;
}

// frees memory allocated for ACK struct
void BRPayementProtocolACKFree(BRPaymentProtocolPayment *payment)
{
}
