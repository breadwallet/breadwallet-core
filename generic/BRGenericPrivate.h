//
//  BRGenericPrivate.h
//  BRCore
//
//  Created by Ed Gamble on 10/14/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.


#ifndef BRGenericPrivate_h
#define BRGenericPrivate_h

#include "BRGeneric.h"
#include "BRGenericHandlers.h"
#include "ethereum/rlp/BRRlp.h"

#if !defined(private_extern)
#define private_extern extern
#endif

#define DECLARE_GENERIC_TYPE(name)                \
    struct BRGeneric##name##Record {              \
      const char *type;                           \
      BRGeneric##name##Handlers handlers;         \
      BRGeneric##name##Ref ref;                   \
    };                                            \
    private_extern BRGeneric##name                \
    gen##name##AllocAndInit (const char *type,    \
                             BRGeneric##name##Ref ref);

#define IMPLEMENT_GENERIC_TYPE(name,field)             \
    private_extern BRGeneric##name                     \
    gen##name##AllocAndInit (const char *type,         \
                             BRGeneric##name##Ref ref) { \
      BRGeneric##name __obj = calloc (1, sizeof (struct BRGeneric##name##Record)); \
      __obj->type = type;                              \
      __obj->handlers = genHandlerLookup(type)->field; \
      __obj->ref = ref;                                \
      return __obj;                                    \
    }

struct BRGenericNetworkRecord {
    const char *type;
    BRGenericNetworkHandlers handlers;
    BRGenericNetworkRef ref;
    int isMainnet;
};

private_extern BRGenericNetwork
genNetworkAllocAndInit (const char *type,
                        BRGenericNetworkRef ref,
                        int isMainet);

DECLARE_GENERIC_TYPE(Account)

DECLARE_GENERIC_TYPE(Address)

struct BRGenericTransferRecord {
    const char *type;
    char *uids;
    BRGenericTransferHandlers handlers;
    BRGenericTransferRef ref;
    BRGenericTransferState state;
    BRGenericTransferDirection direction;
    BRArrayOf(BRGenericTransferAttribute) attributes;
};

private_extern BRGenericTransfer
genTransferAllocAndInit (const char *type,
                         BRGenericTransferRef ref);


struct BRGenericWalletRecord {
    const char *type;
    BRGenericWalletHandlers handlers;
    BRGenericWalletRef ref;
    BRGenericFeeBasis defaultFeeBasis;
};

private_extern BRGenericWallet
genWalletAllocAndInit (const char *type,
                       BRGenericWalletRef ref);

typedef enum {
    GEN_TRANSFER_STATE_ENCODE_V1,
    GEN_TRANSFER_STATE_ENCODE_V2
} BRGenericTransferStateEncodeVersion;

extern BRRlpItem
genTransferStateEncode (BRGenericTransferState state,
                        BRGenericTransferStateEncodeVersion version,
                        BRRlpCoder coder);

extern BRGenericTransferState
genTransferStateDecode (BRRlpItem item,
                        BRRlpCoder coder);

//DECLARE_GENERIC_TYPE(Manager)

#endif /* BRGenericPrivate_h */
