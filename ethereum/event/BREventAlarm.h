//
//  BREventAlarm.h
//  BRCore
//
//  Created by Ed Gamble on 5/7/18.
//  Copyright © 2018 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
#ifndef BR_Event_Alarm_H
#define BR_Event_Alarm_H

#include "BREvent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BREventAlarmClock *BREventAlarmClock;
typedef unsigned int BREventAlarmId;

#define ALARM_ID_NONE       ((BREventAlarmId) 0)

typedef void* BREventAlarmContext;
typedef void (*BREventAlarmCallback) (BREventAlarmContext context,
                                      struct timespec expiration,
                                      BREventAlarmClock clock);

extern BREventAlarmClock alarmClock;

/**
 * Create `alarmClock` the default, and the only one needed, clock.  Optionally start it.
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

extern int
alarmClockIsRunning (BREventAlarmClock clock);

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

/**
 * Check if `identifier` is an alarm in `clock`.
 *
 * @param clock the clock
 * @param identifier the alarm id
 *
 * @return Return true (1) if `clock` has an alarm `identifier`; false (0) otherwise.
 */
extern int
alarmClockHasAlarm (BREventAlarmClock clock,
                    BREventAlarmId identifier);

#ifdef __cplusplus
}
#endif

#endif /* BR_Event_Alarm_H */

