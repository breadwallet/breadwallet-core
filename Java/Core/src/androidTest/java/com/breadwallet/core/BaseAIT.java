package com.breadwallet.core;

import android.content.Context;
import android.support.test.InstrumentationRegistry;

public class BaseAIT {
    protected Context context = InstrumentationRegistry.getInstrumentation().getContext();

    // This is a compromised mainnet paper key.
    protected static final String paperKey =
            "boring head harsh green empty clip fatal typical found crane dinner timber";

    protected static final String accountAsString = "0xa9de3dbD7d561e67527bC1Ecb025c59D53b9F7Ef";
}
