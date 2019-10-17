package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.support.BRSyncDepth;
import com.sun.jna.Pointer;

public class BRCryptoWalletManagerEventReader {

    private static final BRCryptoWalletManagerEvent STRUCTURE = new BRCryptoWalletManagerEvent();
    private static final int OFFSET_OF_UNION = STRUCTURE.offsetOfUnion();

    public static BRCryptoWalletManagerEventType readType(Pointer baseOfStruct){
        int typeEnum = baseOfStruct.getInt(0);
        return BRCryptoWalletManagerEventType.fromCore(typeEnum);
    }

    public static BRSyncDepth readAsSyncRecommended(Pointer baseOfStruct) {
        int depthEnum = baseOfStruct.getInt(OFFSET_OF_UNION);
        BRSyncDepth depth = BRSyncDepth.fromCore(depthEnum);
        return depth;
    }

    public static BRCryptoWallet readAsWallet(Pointer baseOfStruct) {
        Pointer pointer = baseOfStruct.getPointer(OFFSET_OF_UNION);
        BRCryptoWallet wallet = new BRCryptoWallet(pointer);
        return wallet;
    }

    private BRCryptoWalletManagerEventReader() {}
}
