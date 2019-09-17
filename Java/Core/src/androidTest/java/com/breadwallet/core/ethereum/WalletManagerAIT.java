/*
 * WalletManagerAIT
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 3/20/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core.ethereum;

import android.support.test.runner.AndroidJUnit4;

import com.breadwallet.core.BaseAIT;

import org.junit.Test;
import org.junit.runner.RunWith;

import static java.lang.Thread.sleep;
import static org.junit.Assert.assertTrue;

@RunWith(AndroidJUnit4.class)
public class WalletManagerAIT extends BaseAIT {
    @Test
    public void testWalletManagerOne () {
        ClientAIT client = new ClientAIT(BREthereumNetwork.mainnet, paperKey);
//        client.ewm.updateTokens();
        client.ewm.connect();

        try { sleep (10 * 1000); } catch (Exception ex) { assertTrue(false); }

        client.ewm.disconnect();
    }
}
