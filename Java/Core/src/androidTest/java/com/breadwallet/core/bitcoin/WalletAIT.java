package com.breadwallet.core.bitcoin;

import android.support.test.runner.AndroidJUnit4;

import com.breadwallet.core.BRCoreAddress;
import com.breadwallet.core.BRCoreMasterPubKey;
import com.breadwallet.core.BRCoreTransaction;
import com.breadwallet.core.BRCoreWallet;
import com.breadwallet.core.BaseAIT;

import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public class WalletAIT extends BaseAIT {
    public static final String somePaperKey = "library tilt payment fit list note original worth member ensure hurt pistol";
    public static final String someBTCRecvAddress = "1CcQgbJiW6gojk49KK8oVfjkif81BdjRfw";

    public static class Listener implements BRCoreWallet.Listener {
        @Override
        public void balanceChanged(long balance) {

        }

        @Override
        public void onTxAdded(BRCoreTransaction transaction) {

        }

        @Override
        public void onTxUpdated(String hash, int blockHeight, int timeStamp) {

        }

        @Override
        public void onTxDeleted(String hash, int notifyUser, int recommendRescan) {

        }
    }

    @Test
    public void testWalletOne () {
        byte[] paperKeyAsBytes = paperKey.getBytes();
        BRCoreMasterPubKey key = new BRCoreMasterPubKey(paperKeyAsBytes, true);

        byte[] serializedMPK = key.serialize();
        BRCoreMasterPubKey keyToo = new BRCoreMasterPubKey(serializedMPK, false);

        Listener listener = new Listener();

        BRCoreTransaction transactions[] = new BRCoreTransaction[]{};

        try {
            BRCoreWallet wallet = new BRCoreWallet(transactions, key, 0, listener);
            BRCoreWallet walletToo = new BRCoreWallet(transactions, keyToo, 0, listener);

            BRCoreAddress address = wallet.getReceiveAddress();
            BRCoreAddress addressToo = walletToo.getReceiveAddress();

            assertEquals (address.stringify(), addressToo.stringify());
        }
        catch (Exception ex) {
            assertTrue(false);
        }
    }

    @Test
    public void testWalletTwo () {
        byte[] paperKeyAsBytes = somePaperKey.getBytes();
        BRCoreMasterPubKey key = new BRCoreMasterPubKey(paperKeyAsBytes, true);

        Listener listener = new Listener();

        BRCoreTransaction transactions[] = new BRCoreTransaction[]{};

        try {
            BRCoreWallet wallet = new BRCoreWallet(transactions, key, 0, listener);

            String a1 = wallet.getReceiveAddress().stringify();
            String a2 = wallet.getLegacyAddress().stringify();

            assertEquals (wallet.getLegacyAddress().stringify(), someBTCRecvAddress);
        }
        catch (Exception ex) {
            assertTrue(false);
        }
    }
}
