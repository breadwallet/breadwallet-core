package com.breadwallet.core.ethereum;

import android.support.test.runner.AndroidJUnit4;

import com.breadwallet.core.BaseAIT;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

@RunWith(AndroidJUnit4.class)
public class AccountAIT extends BaseAIT {

    @Test
    public void testAccount() {
        ClientAIT client = new ClientAIT(BREthereumNetwork.mainnet, paperKey);

        BREthereumAccount account = client.ewm.getAccount();
        assertEquals(accountAsString, account.getPrimaryAddress());

        byte[] publicKey = account.getPrimaryAddressPublicKey();
        assertNotNull(publicKey);

        byte[] privateKey = account.getPrimaryAddressPrivateKey(paperKey);
        assertNotNull(privateKey);

        assertArrayEquals(publicKey, client.ewm.getAddressPublicKey());
    }
}