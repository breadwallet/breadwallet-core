//
//  BREthereumProvision.c
//  Core
//
//  Created by Ed Gamble on 9/4/18.
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

#include "BREthereumProvision.h"

extern BREthereumLESMessageIdentifier
provisionGetMessageLESIdentifier (BREthereumProvisionType type) {
    switch (type) {
        case PROVISION_BLOCK_HEADERS:        return LES_MESSAGE_GET_BLOCK_HEADERS;
        case PROVISION_BLOCK_BODIES:         return LES_MESSAGE_GET_BLOCK_BODIES;
        case PROVISION_TRANSACTION_RECEIPTS: return LES_MESSAGE_GET_RECEIPTS;
        case PROVISION_ACCOUNTS:             return LES_MESSAGE_GET_PROOFS_V2;
        case PROVISION_TRANSACTION_STATUSES: return LES_MESSAGE_GET_TX_STATUS;
        case PROVISION_SUBMIT_TRANSACTION:   return LES_MESSAGE_SEND_TX2;
    }
}

extern BREthereumPIPRequestType
provisionGetMessagePIPIdentifier (BREthereumProvisionType type) {
    switch (type) {
        case PROVISION_BLOCK_HEADERS:        return PIP_REQUEST_HEADERS;
        case PROVISION_BLOCK_BODIES:         return PIP_REQUEST_BLOCK_BODY;
        case PROVISION_TRANSACTION_RECEIPTS: return PIP_REQUEST_BLOCK_RECEIPTS;
        case PROVISION_ACCOUNTS:             return PIP_REQUEST_ACCOUNT;
        case PROVISION_TRANSACTION_STATUSES: return PIP_REQUEST_TRANSACTION_INDEX;
        case PROVISION_SUBMIT_TRANSACTION:   assert (0); return -1;
    }
}
