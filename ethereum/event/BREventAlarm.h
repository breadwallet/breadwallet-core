//
//  BREventAlarm.h
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
//

#ifndef BR_Event_Alarm_H
#define BR_Event_Alarm_H

#include "BREvent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREventAlarmClock *BREventAlarmClock;
typedef int BREventAlarmId;

typedef void* BREventAlarmContext;
typedef void (*BREventAlarmCallback) (BREventAlarmContext context,
                                      struct timespec expiration,
                                      BREventAlarmClock clock);

extern BREventAlarmClock alarmClock;

/**
 * Create `alarmClock` the default, and only one needed, clock.  Optionally start it.
 */
extern void
alarmClockCreateIfNecessary (int start);

extern BREventAlarmClock
alarmClockCreate (void);

extern void
alarmClockDestroy (BREventAlarmClock clock);

extern void
alarmClockStart (BREventAlarmClock clock);

extern void
alarmClockStop (BREventAlarmClock clock);

extern BREventAlarmId
alarmClockAddAlarmPeriodic (BREventAlarmClock clock,
                            BREventAlarmContext context,
                            BREventAlarmCallback callback,
                            struct timespec period);

extern BREventAlarmId
alarmClockAddAlarm  (BREventAlarmClock clock,
                     BREventAlarmContext context,
                     BREventAlarmCallback callback,
                     struct timespec expiration);

extern void
alarmClockRemAlarm (BREventAlarmClock clock,
                    BREventAlarmId identifier);

#ifdef __cplusplus
}
#endif

#endif /* BR_Event_Alarm_H */

