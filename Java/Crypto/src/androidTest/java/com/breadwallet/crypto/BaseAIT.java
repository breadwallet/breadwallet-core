package com.breadwallet.crypto;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class BaseAIT {
    // Which?
    protected Context context    = InstrumentationRegistry.getInstrumentation().getContext();
    protected Context appContext = InstrumentationRegistry.getTargetContext();

    @Test
    public void useAppContext() {
        assertEquals("com.breadwallet.crypto.test", appContext.getPackageName());
        assertEquals("com.breadwallet.crypto.test", context.getPackageName());
    }

}

