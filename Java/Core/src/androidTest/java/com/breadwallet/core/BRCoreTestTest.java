package com.breadwallet.core;

import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;

import static org.junit.Assert.*;

public class BRCoreTestTest {
    static { System.loadLibrary("core"); }

    @Test
    public void runTests() {

        BRCoreTest coreTest = new BRCoreTest();
        int retVal = coreTest.runTests();

        assertEquals(1, retVal);
    }

    @Test
    public void runTestsSync() {

        BRCoreTest coreTest = new BRCoreTest();
        int retVal = coreTest.runTestsSync(1);

        assertEquals(1, retVal);
    }
}