/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.io.File;

public class SystemAIT {

    private Account account;
    private File coreDataDir;

    @Before
    public void setup() {
        account = HelpersAIT.defaultAccount();

        coreDataDir = HelpersAIT.generateCoreDataDir();
        HelpersAIT.createOrOverwriteDirectory(coreDataDir);
    }

    @After
    public void teardown() {
        HelpersAIT.deleteFile(coreDataDir);
    }

    @Test
    public void testSystemBTC() {

    }
}
