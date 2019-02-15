//
//  BREvent.h
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
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

#ifndef BR_Event_h
#define BR_Event_h

#include <stdlib.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward Declarations */
typedef struct BREventHandlerRecord *BREventHandler;

typedef struct BREventTypeRecord BREventType;
typedef struct BREventRecord BREvent;

/**
 * An EventDispatcher handles an event.  The dispatcher runs in the Handler's thread and should
 * generally be short in duration.
 */
typedef void
(*BREventDispatcher) (BREventHandler handler,
                      BREvent *event);

/**
 * An EventDestroyer destroys an event.  The destroyer runs only when the BREventQueue is
 * destroyed an pending events needs to be destroyed themselves.  Specifically, there are
 * some events the own memory; that memory needs to be freed so as to avoid memory leaks.
 */
typedef void
(*BREventDestroyer) (BREvent *event);

/**
 * An EventType defines the types of events that will be handled.  Each individual Event will hold
 * a reference to an EventType; when the Event is handled, the EventType's eventDispathver will
 * be invoked.  The `eventSize` is used by the handler to allocate a cache of events.
 */
struct BREventTypeRecord{
    const char *eventName;
    size_t eventSize;
    BREventDispatcher eventDispatcher;
    BREventDestroyer eventDestroyer;
};

/**
 * A Event is an asynchronous occurance with an arbitrary set of data and a specified type.
 */
struct BREventRecord {
    struct BREventRecord *next;
    BREventType *type;
    // Add 'context'
    
    // arguments
};

/**
 * The EventStatus (is really an EventHandlerStatus).
 */
typedef enum {
    EVENT_STATUS_SUCCESS,
    EVENT_STATUS_NOT_STARTED,
    EVENT_STATUS_UNKNOWN_TYPE,
    EVENT_STATUS_NULL_EVENT,
    EVENT_STATUS_NONE_PENDING
} BREventStatus;

//
// Timeout Event
//
typedef void *BREventTimeoutContext;

typedef struct {
    struct BREventRecord base;
    BREventTimeoutContext context;
    struct timespec time;
} BREventTimeout;

//
// Event Handler
//

//
// Create / Destroy
//
extern BREventHandler
eventHandlerCreate (const char *name,
                    const BREventType *types[],
                    unsigned int typesCount);

/**
 * Optional specify a periodic TimeoutDispatcher.  The `dispatcher` will run every
 * `timeInMilliseconds` (and will be passed a NULL event).  The event will be delivered OOB (out-of-band)
 * and placed on the front of the event queue.
 */
extern void
eventHandlerSetTimeoutDispatcher (BREventHandler handler,
                                  unsigned int timeInMilliseconds,
                                  BREventDispatcher dispatcher,
                                  BREventTimeoutContext context);

extern void
eventHandlerDestroy (BREventHandler handler);

//
// Start / Stop
//
extern void
eventHandlerStart (BREventHandler handler);

extern void
eventHandlerStop (BREventHandler handler);

extern int
eventHandlerIsRunning (BREventHandler handler);

/**
 * Signal `event` by announcing/sending it to `handler`.  The handler will queue the event
 * at the TAIL of the queue (aka 'first-in, first-out' basis, except for OOB events). The event
 * is handled within the handler's thread.
 *
 * @Note: `event` is added to the TAIL of pending events.
 *
 * This function may block as the event is queued.
 */
extern BREventStatus
eventHandlerSignalEvent (BREventHandler handler,
                         BREvent *event);

/**
 * Signal `event` by announcing/sending it to `handler`. The handler will queue the event
 * at the HEAD of the queue (aka 'Out-Of-Band'); thus the event will be handled immediately.  The
 * event is handled within the handler's thread.
 *
 * @Note: `event` is added to the HEAD (Out-Of-Band) of pending events.
 */
extern BREventStatus
eventHandlerSignalEventOOB (BREventHandler handler,
                            BREvent *event);


/**
 * Clean the handlers' event queue.
 *
 * @param handler
 */
extern void
eventHandlerClear (BREventHandler handler);

#ifdef __cplusplus
}
#endif

#endif /* BR_Event_h */
