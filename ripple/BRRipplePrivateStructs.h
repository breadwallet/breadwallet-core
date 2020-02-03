//
//  BRRipplePrivateStructs.h
//  Core
//
//  Created by Carl Cherry on 4/16/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BRRipple_private_structs_h
#define BRRipple_private_structs_h

#include <stdbool.h>
#include "BRKey.h"
#include "BRRippleBase.h"
#include "BRRippleTransaction.h"

// A structure to hold some bytes and a length
typedef struct _vl_bytes {
    uint8_t * value;
    int length;
} VLBytes;

typedef struct _ripple_memo {
    VLBytes *memoType;
    VLBytes *memoData;
    VLBytes *memoFormat;
} BRRippleMemo;

typedef struct memo_node {
    BRRippleMemo memo;
    struct memo_node *next;
} BRRippleMemoNode;

// Ripple has the concept of fields, which are sorted. This
// field stucture allows us to easily sort the fields when
// creating the serialized format
// *** NOTE ***
// This structure is used with the array_new, array_add, etc funtions
// so if any of the content includes allocate memory (like memos) then
// that needs to be cleanup up before array_free is called.
typedef struct _ripple_field {
    int typeCode;
    int fieldCode;
    BRRippleMemoNode *memos;
    union _data {
        uint8_t i8;
        uint16_t i16;
        uint32_t i32;
        uint64_t i64;
        BRRippleAmount amount;
        BRRippleAddress address;
        BRKey publicKey;
        BRRippleSignatureRecord signature;
        uint8_t hash[32]; // There are 3 potential hash fields - longest is 32 bytes
    } data;
} BRRippleField;

// Create the Variable length byte structure and initialize
inline static VLBytes * createVLBytes(int length)
{
    VLBytes * bytes = calloc(1, sizeof(VLBytes));
    bytes->length = length;
    bytes->value = calloc(1, length);
    return bytes;
}

// Each memo has up to 3 fields of variable length data
inline static void memoFieldsFree(BRRippleMemo * memo)
{
    if (memo->memoData) free(memo->memoData);
    if (memo->memoType) free(memo->memoType);
    if (memo->memoFormat) free(memo->memoFormat);
}

// Recursively delete the linked list of memos
inline static void memoListFree(BRRippleMemoNode *memos)
{
    if (memos) {
        memoListFree(memos->next);
        memoFieldsFree(&memos->memo);
        free(memos);
    }
}

// Add a new memo list node to this field
inline static BRRippleMemoNode * memoListAdd(BRRippleField * field)
{
    BRRippleMemoNode *newNode = calloc(1, sizeof(BRRippleMemoNode));
    
    // First case - no existing nodes
    if (!field->memos) {
        field->memos = newNode;
    } else {
        BRRippleMemoNode *node = field->memos;
        BRRippleMemoNode *nextNode = node->next;
        while(nextNode) {
            node = nextNode;
            nextNode = node->next;
        }
        node->next = newNode;
    }
    return newNode;
}

struct BRRippleTransferRecord {
    BRRippleAddress sourceAddress;
    BRRippleAddress targetAddress;
    BRRippleUnitDrops amount;
    BRRippleUnitDrops fee;
    BRRippleTransactionHash transactionId;
    uint64_t timestamp;
    uint64_t blockHeight;
    int error;
    BRRippleTransaction transaction;
};

#endif
