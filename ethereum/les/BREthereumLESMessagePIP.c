//
//  BREthereumLESMessagePIP.c
//  Core
//
//  Created by Ed Gamble on 9/1/18.
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

#include "BREthereumLESMessagePIP.h"

extern const char *
messagePIPGetRequestName (BREthereumPIPRequestType type) {
    static const char *
    messagePIPRequestNames[] = {
        "Headers",
        "Headers Proof",
        "Tx Index",
        "Receipts",
        "Bodies",
        "Accounts",
        "Storage",
        "Code",
        "Execution"
    };

    return messagePIPRequestNames[type];
}

extern const char *
messagePIPGetIdentifierName (BREthereumPIPMessageType identifer) {
    static const char *
    messagePIPNames[] = {
        "Status",
        "Announce",
        "Request",
        "Response",
        "UpdCredit",
        "AckCredit",
        "RelayTx"
    };

    return messagePIPNames[identifer];
}

extern BRRlpItem
messagePIPEncode (BREthereumPIPMessage message,
                  BREthereumMessageCoder coder) {
    return NULL;
}

extern BREthereumPIPMessage
messagePIPDecode (BRRlpItem item,
                  BREthereumMessageCoder coder,
                  BREthereumPIPMessageType identifier) {
    return (BREthereumPIPMessage) {};
}

extern uint64_t
messagePIPGetRequestId (const BREthereumPIPMessage *message) {
    switch (message->type) {
        case PIP_MESSAGE_STATUS:   return PIP_MESSAGE_NO_REQUEST_ID;
        case PIP_MESSAGE_ANNOUNCE: return PIP_MESSAGE_NO_REQUEST_ID;
        case PIP_MESSAGE_REQUEST:  return message->u.request.reqId;
        case PIP_MESSAGE_RESPONSE: return message->u.response.reqId;
        case PIP_MESSAGE_UPDATE_CREDIT_PARAMETERS: return PIP_MESSAGE_NO_REQUEST_ID;
        case PIP_MESSAGE_ACKNOWLEDGE_UPDATE:       return PIP_MESSAGE_NO_REQUEST_ID;
        case PIP_MESSAGE_RELAY_TRANSACTIONS:       return PIP_MESSAGE_NO_REQUEST_ID;
    }
}
