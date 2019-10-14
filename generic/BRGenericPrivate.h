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

#if !defined(private_extern)
#define private_extern extern
#endif

#define DECLARE_GENERIC_TYPE(name)            \
struct BRGeneric##name##Record {              \
  const char *type;                           \
  BRGeneric##name##Handlers handlers;         \
};                                            \
private_extern BRGeneric##name                \
gen##name##AllocAndInit (const char *type,    \
                         size_t sizeInBytes);

#define IMPLEMENT_GENERIC_TYPE(name,ref)         \
private_extern BRGeneric##name                   \
gen##name##AllocAndInit (const char *type,       \
                         size_t sizeInBytes) {   \
  BRGeneric##name ref = calloc (1, sizeInBytes); \
  ref->type = type;                              \
  ref->handlers = genHandlerLookup(type)->ref;   \
  return ref;                                    \
}

DECLARE_GENERIC_TYPE(Network)

DECLARE_GENERIC_TYPE(Account)

DECLARE_GENERIC_TYPE(Address)

DECLARE_GENERIC_TYPE(FeeBasis)

DECLARE_GENERIC_TYPE(Transfer)

DECLARE_GENERIC_TYPE (Wallet)

//DECLARE_GENERIC_TYPE(Manager)

#endif /* BRGenericPrivate_h */
