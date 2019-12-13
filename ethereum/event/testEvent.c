//
//  testEvent.c
//  CoreTests
//
//  Created by Ed Gamble on 7/23/18.
//  Copyright © 2018-2019 Breadwinner AG.  All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.

#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "ethereum/event/BREvent.h"
#include "ethereum/event/BREventAlarm.h"

static pthread_cond_t testEventAlarmConditional = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t testEventAlarmMutex = PTHREAD_MUTEX_INITIALIZER;
static BREventAlarmContext testEventAlarmContext = (void*) 1;
static struct timespec testEventAlarmPeriod = { 10, 0 };
static int testEventAlarmCount = 0;

static void
testEventAlarmCallback (BREventAlarmContext context,
                        struct timespec expiration,
                        BREventAlarmClock clock) {
    assert (clock == alarmClock);
    assert (context == testEventAlarmContext);
    testEventAlarmCount++;
    pthread_cond_signal(&testEventAlarmConditional);
}

static void
runEventTest (void) {
    alarmClockCreateIfNecessary (0);
    assert (NULL != alarmClock);

    BREventAlarmId alarm = alarmClockAddAlarmPeriodic(alarmClock, NULL, NULL, testEventAlarmPeriod);
    alarmClockRemAlarm(alarmClock, alarm);

    alarmClockStart(alarmClock);

    pthread_mutex_lock(&testEventAlarmMutex);
    alarm = alarmClockAddAlarmPeriodic(alarmClock, testEventAlarmContext, testEventAlarmCallback, testEventAlarmPeriod);
    pthread_cond_wait(&testEventAlarmConditional, &testEventAlarmMutex);
    alarmClockRemAlarm(alarmClock, alarm);
    assert (0 < testEventAlarmCount);
    alarmClockStop(alarmClock);
    alarmClockDestroy(alarmClock);
}

extern void
runEventTests (void) {
    runEventTest();
}
