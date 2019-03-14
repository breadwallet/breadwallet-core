package com.breadwallet.core.bitcoin;

import android.support.test.runner.AndroidJUnit4;

import com.breadwallet.core.BRCoreMasterPubKey;
import com.breadwallet.core.BaseAIT;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public class MasterPublicKeyAIT extends BaseAIT {
    @Test
    public void testMPKOne () {
        byte[] paperKeyAsBytes = paperKey.getBytes();
        BRCoreMasterPubKey key = new BRCoreMasterPubKey(paperKeyAsBytes, true);

        byte[] serializedMPK = key.serialize();
        BRCoreMasterPubKey keyToo = new BRCoreMasterPubKey(serializedMPK, false);

        assertArrayEquals(key.getPubKey(), keyToo.getPubKey());
    }
}
