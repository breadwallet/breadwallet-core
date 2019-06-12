package com.breadwallet.corenative.ethereum;

import com.breadwallet.corenative.CryptoLibrary;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInteger;
import com.sun.jna.Pointer;
import com.sun.jna.PointerType;

public class BREthereumEwm extends PointerType {

    public BREthereumEwm(Pointer address) {
        super(address);
    }

    public BREthereumEwm() {
        super();
    }

    public BREthereumToken getToken(BREthereumWallet wid) {
        return CryptoLibrary.INSTANCE.ewmWalletGetToken(this, wid);
    }

    public BREthereumNetwork getNetwork() {
        return CryptoLibrary.INSTANCE.ewmGetNetwork(this);
    }

    public void announceWalletBalance(BREthereumWallet wid, String data, int rid) {
        CryptoLibrary.INSTANCE.ewmAnnounceWalletBalance(this, wid, data, rid);
    }

    public void announceGasPrice(BREthereumWallet wid, String data, int rid) {
        CryptoLibrary.INSTANCE.ewmAnnounceGasPrice(this, wid, data, rid);
    }

    public void announceTransaction(int rid, String hash, String sourceAddr, String targetAddr, String contractAddr,
                                    String amount, String gasLimit, String gasPrice, String data,
                                    String nonce, String gasUsed, String blockNumber, String blockHash,
                                    String blockConfirmations, String blockTransacionIndex, String blockTimestamp,
                                    String isError) {
        CryptoLibrary.INSTANCE.ewmAnnounceTransaction(this, rid, hash, sourceAddr, targetAddr, contractAddr, amount,
                gasLimit, gasPrice, data, nonce, gasUsed, blockNumber, blockHash, blockConfirmations,
                blockTransacionIndex, blockTimestamp, isError);
    }

    public void announceGasEstimate(BREthereumWallet wid, BREthereumTransfer tid, String gasEstimate, int rid) {
        CryptoLibrary.INSTANCE.ewmAnnounceGasEstimate(this, wid, tid, gasEstimate, rid);
    }

    public void announceTransactionComplete(int rid, boolean success) {
        CryptoLibrary.INSTANCE.ewmAnnounceTransactionComplete(this, rid, success ?
                BREthereumBoolean.ETHEREUM_BOOLEAN_TRUE :
                BREthereumBoolean.ETHEREUM_BOOLEAN_FALSE);
    }

    public void announceLogComplete(int rid, boolean success) {
        CryptoLibrary.INSTANCE.ewmAnnounceLogComplete(this, rid, success ? BREthereumBoolean.ETHEREUM_BOOLEAN_TRUE :
                BREthereumBoolean.ETHEREUM_BOOLEAN_FALSE);
    }

    public void announceToken(int rid, String address, String symbol, String name, String description, UnsignedInteger decimals, String defaultGasLimit, String defaultGasPrice) {
        CryptoLibrary.INSTANCE.ewmAnnounceToken(this, rid, address, symbol, name, description, decimals.intValue(), defaultGasLimit, defaultGasPrice);
    }

    public void announceTokenComplete(int rid, boolean success) {
        CryptoLibrary.INSTANCE.ewmAnnounceTokenComplete(this, rid, success ? BREthereumBoolean.ETHEREUM_BOOLEAN_TRUE :
                BREthereumBoolean.ETHEREUM_BOOLEAN_FALSE);
    }

    public void announceBlockNumber(String number, int rid) {
        CryptoLibrary.INSTANCE.ewmAnnounceBlockNumber(this, number, rid);
    }

    public void announceNonce(String address, String nonce, int rid) {
        CryptoLibrary.INSTANCE.ewmAnnounceNonce(this, address, nonce, rid);
    }
}
