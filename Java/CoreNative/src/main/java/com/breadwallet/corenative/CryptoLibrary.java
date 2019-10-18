/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 7/1/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
*
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative;

import com.breadwallet.corenative.crypto.BRCryptoAccount;
import com.breadwallet.corenative.crypto.BRCryptoAddress;
import com.breadwallet.corenative.crypto.BRCryptoAmount;
import com.breadwallet.corenative.crypto.BRCryptoCWMClient;
import com.breadwallet.corenative.crypto.BRCryptoCWMListener;
import com.breadwallet.corenative.crypto.BRCryptoCWMClientCallbackState;
import com.breadwallet.corenative.crypto.BRCryptoCipher;
import com.breadwallet.corenative.crypto.BRCryptoCoder;
import com.breadwallet.corenative.crypto.BRCryptoCurrency;
import com.breadwallet.corenative.crypto.BRCryptoFeeBasis;
import com.breadwallet.corenative.crypto.BRCryptoHash;
import com.breadwallet.corenative.crypto.BRCryptoHasher;
import com.breadwallet.corenative.crypto.BRCryptoKey;
import com.breadwallet.corenative.crypto.BRCryptoNetwork;
import com.breadwallet.corenative.crypto.BRCryptoNetworkFee;
import com.breadwallet.corenative.crypto.BRCryptoPeer;
import com.breadwallet.corenative.crypto.BRCryptoSigner;
import com.breadwallet.corenative.crypto.BRCryptoTransfer;
import com.breadwallet.corenative.crypto.BRCryptoTransferState;
import com.breadwallet.corenative.crypto.BRCryptoUnit;
import com.breadwallet.corenative.crypto.BRCryptoWallet;
import com.breadwallet.corenative.crypto.BRCryptoWalletManager;
import com.breadwallet.corenative.crypto.BRCryptoWalletManagerState;
import com.breadwallet.corenative.crypto.BRCryptoWalletMigrator;
import com.breadwallet.corenative.crypto.BRCryptoWalletMigratorStatus;
import com.breadwallet.corenative.crypto.BRCryptoWalletSweeper;
import com.breadwallet.corenative.support.BRDisconnectReason;
import com.breadwallet.corenative.support.BRSyncStoppedReason;
import com.breadwallet.corenative.support.BRTransferSubmitError;
import com.breadwallet.corenative.support.UInt256;
import com.breadwallet.corenative.utility.SizeT;
import com.breadwallet.corenative.utility.SizeTByReference;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.NativeLibrary;
import com.sun.jna.Pointer;
import com.sun.jna.StringArray;
import com.sun.jna.ptr.IntByReference;

import java.nio.ByteBuffer;

public interface CryptoLibrary extends Library {

    String JNA_LIBRARY_NAME = "crypto";
    NativeLibrary LIBRARY = NativeLibrary.getInstance(CryptoLibrary.JNA_LIBRARY_NAME);
    CryptoLibrary INSTANCE = Native.load(CryptoLibrary.JNA_LIBRARY_NAME, CryptoLibrary.class);

    // crypto/BRCryptoNetwork.h
    // TODO(fix): Can this be migrated to CryptoLibraryNative?
    void cryptoNetworkSetNetworkFees(Pointer network, BRCryptoNetworkFee[] fees, SizeT count);
}
