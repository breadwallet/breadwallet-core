package com.breadwallet.coredemo;

import com.breadwallet.core.ethereum.BREthereumBlock;
import com.breadwallet.core.ethereum.BREthereumEWM;
import com.breadwallet.core.ethereum.BREthereumNetwork;
import com.breadwallet.core.ethereum.BREthereumTransfer;
import com.breadwallet.core.ethereum.BREthereumWallet;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class CoreDemoEthereumClient implements BREthereumEWM.Client {
    interface WalletListener {
        void announceWalletEvent (BREthereumEWM ewm,
                                  BREthereumWallet wallet,
                                  BREthereumEWM.WalletEvent event);
    }

    interface TransferListener {
        void announceTransferEvent (BREthereumEWM ewm,
                                    BREthereumWallet wallet,
                                    BREthereumTransfer transfer,
                                    BREthereumEWM.TransactionEvent event);
    }

    protected BREthereumNetwork network;
    protected BREthereumEWM ewm;

    public CoreDemoEthereumClient(BREthereumNetwork network,
                                  String paperKey) {
        this.network = network;
        this.ewm = new BREthereumEWM (this, network, paperKey, null);
    }

    @Override
    public void getGasPrice(int wid, int rid) {
        ewm.announceGasPrice(wid, "0x77", rid);
    }

    @Override
    public void getGasEstimate(int wid, int tid, String to, String amount, String data, int rid) {
        ewm.announceGasEstimate (wid, tid, "21000", rid);
    }

    @Override
    public void getBalance(int wid, String address, int rid) {
        ewm.announceBalance (wid, "0xffc0", rid);

    }

    @Override
    public void submitTransaction(int wid, int tid, String rawTransaction, int rid) {
        ewm.announceSubmitTransaction (wid, tid, "0xffaabb", rid);
    }

    @Override
    public void getTransactions(String address, int rid) {
        ewm.announceTransaction(rid,
                "0x4f992a47727f5753a9272abba36512c01e748f586f6aef7aed07ae37e737d220",
                ewm.getAddress(),
                "0xde0b295669a9fd93d5f28d9ec85e40f4cb697bae",
                "",
                "11113000000000",
                "21000",
                "21000000000",
                "0x",
                "118",
                "21000",
                "1627184",
                "0x0ef0110d68ee3af220e0d7c10d644fea98252180dbfc8a94cab9f0ea8b1036af",
                "339050",
                "3",
                "1516477482",
                "0");
    }

    @Override
    public void getLogs(String contract, String address, String event, int rid) {
        ewm.announceLog(rid,
                "0xa37bd8bd8b1fa2838ef65aec9f401f56a6279f99bb1cfb81fa84e923b1b60f2b",
                "0x722dd3f80bac40c951b51bdd28dd19d435762180",
                new String[]{
                        "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                        "0x0000000000000000000000000000000000000000000000000000000000000000",
                        "0x000000000000000000000000bdfdad139440d2db9ba2aa3b7081c2de39291508"},
                "0x0000000000000000000000000000000000000000000000000000000000002328",
                "0xba43b7400",
                "0xc64e",
                "0x",
                "0x1e487e",
                "0x",
                "0x59fa1ac9");
    }

    @Override
    public void getTokens(int rid) {
        ewm.announceToken(
                "0x558ec3152e2eb2174905cd19aea4e34a23de9ad6",
                "BRD",
                "BRD Token",
                "The BRD Token",
                18,
                null,
                null,
                rid);

        ewm.announceToken((this.network == BREthereumNetwork.mainnet
                        ? "0x9e3359f862b6c7f5c660cfd6d1aa6909b1d9504d"
                        : "0x6e67ccd648244b3b8e2f56149b40ba8de9d79b09"),
                "CCC",
                "Container Crypto Coin",
                "",
                18,
                null,
                null,
                rid);
    }

    @Override
    public void getBlockNumber(int rid) {
        ewm.announceBlockNumber("5900000", rid);
    }

    @Override
    public void getNonce(String address, int rid) {
        ewm.announceNonce(address, "17", rid);
    }

    @Override
    public void saveNodes() {
        System.out.println ("TST: saveNodes");
    }

    @Override
    public void saveBlocks() {
        System.out.println ("TST: saveBlocks");

    }

    @Override
    public void changeTransaction() {
        System.out.println ("TST: changeTransaction");

    }

    @Override
    public void changeLog() {
        System.out.println ("TST: changeLog");

    }

    @Override
    public void handleEWMEvent(BREthereumEWM.EWMEvent event, BREthereumEWM.Status status, String errorDescription) {
        System.out.println ("TST: EWMEvent: " + event.name());

    }

    @Override
    public void handlePeerEvent(BREthereumEWM.PeerEvent event, BREthereumEWM.Status status, String errorDescription) {
        System.out.println ("TST: PeerEvent: " + event.name());
    }

    //
    // Wallet Event
    //
    List<WalletListener> walletListeners = new ArrayList<>();

    public void addWalletListener (WalletListener l) {
        walletListeners.add(l);
    }

    @Override
    public void handleWalletEvent(BREthereumWallet wallet,
                                  BREthereumEWM.WalletEvent event,
                                  BREthereumEWM.Status status,
                                  String errorDescription) {
        System.out.println ("TST: WalletEvent: " + event.name());
        for (WalletListener l : walletListeners)
            l.announceWalletEvent(this.ewm, wallet, event);
    }

    //
    // Block Event
    //
    @Override
    public void handleBlockEvent(BREthereumBlock block, BREthereumEWM.BlockEvent event, BREthereumEWM.Status status, String errorDescription) {
        System.out.println ("TST: BlockEvent: " + event.name());
    }

    //
    // Transfer Event
    //
    Map<BREthereumWallet, TransferListener> transferListenersMap = new HashMap<>();

    public void addTransferListener (BREthereumWallet wallet,
                                     TransferListener listener) {
        transferListenersMap.put (wallet, listener);
    }

    @Override
    public void handleTransferEvent(BREthereumWallet wallet, BREthereumTransfer transaction, BREthereumEWM.TransactionEvent event, BREthereumEWM.Status status, String errorDescription) {
        System.out.println ("TST: WalletEvent: " + event.name());
        TransferListener l = transferListenersMap.get(wallet);
        if (null != l)
            l.announceTransferEvent(this.ewm, wallet, transaction, event);
    }
}
