//
//  BREventQueue.h
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

#ifndef BR_Event_Queue_H
#define BR_Event_Queue_H

#include "BREvent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREventQueueRecord *BREventQueue;

/**
 * Create an Event Queue with `size` as the maximum event size and with the
 * optional `lock`.
 */
extern BREventQueue
eventQueueCreate (size_t size);

extern void
eventQueueDestroy (BREventQueue queue);

extern void
eventQueueEnqueueTail (BREventQueue queue,
                       const BREvent *event);
extern void
eventQueueEnqueueHead (BREventQueue queue,
                       const BREvent *event);

extern BREventStatus
eventQueueDequeue (BREventQueue queue,
                   BREvent *event);

extern int
eventQueueHasPending (BREventQueue queue);

extern void
eventQueueClear (BREventQueue queue);

#ifdef __cplusplus
}
#endif

#endif /* BR_Event_Queue_H */
