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

        try { sleep (120 * 1000); } catch (Exception ex) { assertTrue(false); }

        client.ewm.disconnect();
    }
}
