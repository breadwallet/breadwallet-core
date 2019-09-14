/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 5/31/18.
 * Copyright (c) 2018-2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BRCryptoCWMListener extends Structure {

    public interface BRCryptoCWMListenerWalletManagerEvent extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoWalletManagerEvent.ByValue event);
    };
    public interface BRCryptoCWMListenerWalletEvent extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoWallet wallet, BRCryptoWalletEvent.ByValue event);
    };
    public interface BRCryptoCWMListenerTransferEvent extends Callback {
        void apply(Pointer context, BRCryptoWalletManager manager, BRCryptoWallet wallet, BRCryptoTransfer transfer, BRCryptoTransferEvent.ByValue event);
    };

    public Pointer context;
    public BRCryptoCWMListenerWalletManagerEvent walletManagerEventCallback;
    public BRCryptoCWMListenerWalletEvent walletEventCallback;
    public BRCryptoCWMListenerTransferEvent transferEventCallback;
    public BRCryptoCWMListener() {
        super();
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("context", "walletManagerEventCallback", "walletEventCallback", "transferEventCallback");
    }

    public BRCryptoCWMListener(Pointer context, BRCryptoCWMListenerWalletManagerEvent walletManagerEventCallback, BRCryptoCWMListenerWalletEvent walletEventCallback, BRCryptoCWMListenerTransferEvent transferEventCallback) {
        super();
        this.context = context;
        this.walletManagerEventCallback = walletManagerEventCallback;
        this.walletEventCallback = walletEventCallback;
        this.transferEventCallback = transferEventCallback;
    }

    public BRCryptoCWMListener(Pointer peer) {
        super(peer);
    }

    public static class ByReference extends BRCryptoCWMListener implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMListener implements Structure.ByValue {
        public ByValue(Pointer context, BRCryptoCWMListenerWalletManagerEvent walletManagerEventCallback, BRCryptoCWMListenerWalletEvent walletEventCallback, BRCryptoCWMListenerTransferEvent transferEventCallback) {
            super(context, walletManagerEventCallback, walletEventCallback, transferEventCallback);
        }
    }
}
