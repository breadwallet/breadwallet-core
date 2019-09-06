package com.breadwallet.crypto;

public enum WalletManagerSyncDepth {
    SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND,
    SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK,
    SYNC_DEPTH_FROM_CREATION;

    public int toSerialization () {
        switch (this) {
            case SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND: return 0xd0;
            case SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK:  return 0xd1;
            case SYNC_DEPTH_FROM_CREATION:            return 0xd2;
            default: return 0; // error
        }
    }

    public static WalletManagerSyncDepth fromSerialization (int serialization) {
        switch (serialization) {
            case 0xd0: return SYNC_DEPTH_FROM_LAST_CONFIRMED_SEND;
            case 0xd1: return SYNC_DEPTH_FROM_LAST_TRUSTED_BLOCK;
            case 0xd2: return SYNC_DEPTH_FROM_CREATION;
            default: return null;
        }
    }
}
