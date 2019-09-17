/*
 * WalletAIT
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

import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public class WalletAIT extends BaseAIT {
    @Test
    public void testWalletOne() {
        ClientAIT client = new ClientAIT(BREthereumNetwork.mainnet, paperKey);
        BREthereumWallet wallet = client.ewm.getWallet();

        assertNotNull (wallet);

        assertEquals("ETH", wallet.getSymbol());
        assertEquals("0.000000000000000000", wallet.getBalance());
        assertEquals("0.000000000000000000", wallet.getBalance(BREthereumAmount.Unit.ETHER_ETHER));
        assertEquals(21000, wallet.getDefaultGasLimit());
        assertEquals(500000000, wallet.getDefaultGasPrice());
        assertNull (wallet.getToken());
        assertEquals(BREthereumNetwork.mainnet, wallet.getNetwork());
        assertEquals(client.ewm.getAccount(), wallet.getAccount());
        assertEquals(0, wallet.getTransfers().length);

        assertTrue(wallet.walletHoldsEther());
    }

    @Test
    public void testWalletTransfer () {
        ClientAIT client = new ClientAIT(BREthereumNetwork.mainnet, paperKey);
        BREthereumWallet wallet = client.ewm.getWallet();

        assertNotNull(wallet);

        BREthereumTransfer transfer = wallet.createTransfer("0xb3Cf25fa25D4535A91B9491BD16CC2FE36462E39",
                "1.0",
                BREthereumAmount.Unit.ETHER_ETHER);

        assertNotNull(transfer);

        assertEquals("10500.000000000", transfer.getFee());
        assertEquals("1.000000000000000000", transfer.getAmount());
        assertEquals("0xb3Cf25fa25D4535A91B9491BD16CC2FE36462E39", transfer.getTargetAddress());
        assertEquals(accountAsString, transfer.getSourceAddress());
        assertEquals(0, transfer.getBlockConfirmations());
        assertFalse(transfer.isConfirmed());
        assertFalse(transfer.isSubmitted());

        assertEquals(21000, transfer.getGasLimit());
        assertEquals("500000000", transfer.getGasPrice(BREthereumAmount.Unit.ETHER_WEI));
        assertEquals("0.500000000", transfer.getGasPrice(BREthereumAmount.Unit.ETHER_GWEI));
        assertEquals("0.500000000", transfer.getGasPrice());

        assertEquals(0, transfer.getGasUsed());
        assertEquals(-1, transfer.getNonce());   // nonce not assigned (must be signed).
    }
}
