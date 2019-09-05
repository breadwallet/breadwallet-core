package com.breadwallet.crypto;

public enum WalletManagerSyncDepth {
    SYNC_DEPTH_LOW,
    SYNC_DEPTH_MEDIUM,
    SYNC_DEPTH_HIGH;

    public int toSerialization () {
        switch (this) {
            case SYNC_DEPTH_LOW:    return 0xd0;
            case SYNC_DEPTH_MEDIUM: return 0xd1;
            case SYNC_DEPTH_HIGH:   return 0xd2;
            default: return 0; // error
        }
    }

    public static WalletManagerSyncDepth fromSerialization (int serialization) {
        switch (serialization) {
            case 0xd0: return SYNC_DEPTH_LOW;
            case 0xd1: return SYNC_DEPTH_MEDIUM;
            case 0xd2: return SYNC_DEPTH_HIGH;
            default: return null;
        }
    }
}
