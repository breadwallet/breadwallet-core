/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.utility.Cookie;
import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

public class BRCryptoCWMListener extends Structure {

    //
    // Implementation Detail
    //

    public interface BRCryptoCWMListenerWalletManagerEvent extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      BRCryptoWalletManagerEvent.ByValue event);
    }

    public interface BRCryptoCWMListenerWalletEvent extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer wallet,
                      BRCryptoWalletEvent.ByValue event);
    }

    public interface BRCryptoCWMListenerTransferEvent extends Callback {
        void callback(Pointer context,
                      Pointer manager,
                      Pointer wallet,
                      Pointer transfer,
                      BRCryptoTransferEvent.ByValue event);
    }

    //
    // Client Interface
    //

    public interface WalletManagerEventCallback extends BRCryptoCWMListenerWalletManagerEvent {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoWalletManagerEvent.ByValue event);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              BRCryptoWalletManagerEvent.ByValue event) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   event);
        }
    }

    public interface WalletEventCallback extends BRCryptoCWMListenerWalletEvent {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoWallet wallet,
                    BRCryptoWalletEvent.ByValue event);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer wallet,
                              BRCryptoWalletEvent.ByValue event) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoWallet(wallet),
                   event);
        }
    }

    public interface TransferEventCallback extends BRCryptoCWMListenerTransferEvent {
        void handle(Cookie context,
                    BRCryptoWalletManager manager,
                    BRCryptoWallet wallet,
                    BRCryptoTransfer transfer,
                    BRCryptoTransferEvent.ByValue event);

        @Override
        default void callback(Pointer context,
                              Pointer manager,
                              Pointer wallet,
                              Pointer transfer,
                              BRCryptoTransferEvent.ByValue event) {
            handle(new Cookie(context),
                   new BRCryptoWalletManager(manager),
                   new BRCryptoWallet(wallet),
                   new BRCryptoTransfer(transfer),
                   event);
        }
    }

    //
    // Client Struct
    //

    public Pointer context;
    public BRCryptoCWMListenerWalletManagerEvent walletManagerEventCallback;
    public BRCryptoCWMListenerWalletEvent walletEventCallback;
    public BRCryptoCWMListenerTransferEvent transferEventCallback;

    public BRCryptoCWMListener() {
        super();
    }

    public BRCryptoCWMListener(Pointer peer) {
        super(peer);
    }

    public BRCryptoCWMListener(Cookie context,
                               WalletManagerEventCallback walletManagerEventCallback,
                               WalletEventCallback walletEventCallback,
                               TransferEventCallback transferEventCallback) {
        super();
        this.context = context.getPointer();
        this.walletManagerEventCallback = walletManagerEventCallback;
        this.walletEventCallback = walletEventCallback;
        this.transferEventCallback = transferEventCallback;
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("context", "walletManagerEventCallback", "walletEventCallback", "transferEventCallback");
    }

    public ByValue toByValue() {
        ByValue other = new ByValue();
        other.context = this.context;
        other.walletManagerEventCallback = this.walletManagerEventCallback;
        other.walletEventCallback = this.walletEventCallback;
        other.transferEventCallback = this.transferEventCallback;
        return other;
    }

    public static class ByReference extends BRCryptoCWMListener implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoCWMListener implements Structure.ByValue {

    }
}
