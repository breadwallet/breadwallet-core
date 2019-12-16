//
//  BRGenericHandlers.c
//  Core
//
//  Created by Ed Gamble on 6/19/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <pthread.h>
#include "support/BRArray.h"
#include "BRGenericHandlers.h"

static pthread_once_t  _handlers_once = PTHREAD_ONCE_INIT;
//static pthread_mutex_t _handler_lock;

static BRArrayOf(BRGenericHandlers) arrayOfHandlers = NULL;

static void _handlers_init (void) {
    array_new (arrayOfHandlers, 10);
}

extern void
genHandlersInstall (const BRGenericHandlers handlers) {
    pthread_once (&_handlers_once, _handlers_init);
    array_add (arrayOfHandlers, handlers);

}

extern const BRGenericHandlers
genHandlerLookup (const char *type) {
    pthread_once (&_handlers_once, _handlers_init);
    for (size_t index = 0; index < array_count(arrayOfHandlers); index++)
        if (0 == strcmp (type, arrayOfHandlers[index]->type))
            return arrayOfHandlers[index];

    assert(0);
    return NULL;
}

