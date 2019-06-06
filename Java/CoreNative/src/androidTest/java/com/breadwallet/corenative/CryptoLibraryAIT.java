package com.breadwallet.corenative;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

public class CryptoLibraryAIT {
    @Test
    public void useAppContext() {
        assertNotNull(CryptoLibrary.INSTANCE);
    }
}
