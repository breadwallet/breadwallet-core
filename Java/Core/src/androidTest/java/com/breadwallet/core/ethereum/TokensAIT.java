package com.breadwallet.core.ethereum;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import com.breadwallet.core.CoreLibraryLoad;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import static java.lang.Thread.sleep;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/**
 * Instrumented test, which will execute on an Android device.
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
@RunWith(AndroidJUnit4.class)
public class TokensAIT extends CoreLibraryLoad {
    @Test
    public void testTokenOne () {
        ClientAIT client = new ClientAIT(BREthereumNetwork.mainnet,
                "boring head harsh green empty clip fatal typical found crane dinner timber");

        assertEquals (0, client.ewm.getTokens().length);

        client.ewm.announceToken(
                "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",
                "BRD",
                "BRD Token",
                "The BRD Token",
                18,
                null,
                null,
                0);

        assertEquals (0, client.ewm.getTokens().length);

        client.ewm.connect();

        // Wait for token-event
        try { sleep (1 * 1000); } catch (Exception ex) { assertTrue(false); }
        assertEquals (1, client.ewm.getTokens().length);

        BREthereumToken tokenBRD = client.ewm.getTokenBRD();

        assertNotNull(tokenBRD);
        assertNotNull(client.ewm.lookupToken ("0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"));

        assertTrue(new HashSet<BREthereumToken> (Arrays.asList (client.ewm.getTokens())).contains(tokenBRD));

        client.ewm.announceToken(
                "0x9e3359f862b6c7f5c660cfd6d1aa6909b1d9504d",
                "CCC",
                "Container Crypto Coin",
                "",
                18,
                null,
                null,
                1);

        try { sleep (1 * 1000); } catch (Exception ex) { assertTrue(false); }
        assertEquals (2, client.ewm.getTokens().length);

        assertNotNull(client.ewm.lookupToken ("0x9e3359f862b6c7f5c660cfd6d1aa6909b1d9504d"));

        client.ewm.updateTokens();
        try { sleep (1 * 1000); } catch (Exception ex) { assertTrue(false); }
        assertEquals (2, client.ewm.getTokens().length);
        assertNotNull(client.ewm.lookupToken ("0x558ec3152e2eb2174905cd19aea4e34a23de9ad6"));
        assertNotNull(client.ewm.lookupToken ("0x9e3359f862b6c7f5c660cfd6d1aa6909b1d9504d"));
    }
}
