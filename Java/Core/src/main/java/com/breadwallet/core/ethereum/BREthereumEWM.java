/*
 * EthereumEWM
 *
 * Created by Ed Gamble <ed@breadwallet.com> on 3/7/18.
 * Copyright (c) 2018 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.core.ethereum;

import android.support.annotation.Nullable;

import com.breadwallet.core.BRCoreJniReference;

import java.lang.ref.WeakReference;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import static com.breadwallet.core.ethereum.BREthereumToken.jniGetTokenBRD;
import static com.breadwallet.core.ethereum.BREthereumToken.jniTokenAll;

/**
 *
 */

public class BREthereumEWM extends BRCoreJniReference {
    public enum Status {
        SUCCESS,

        // Reference access
        ERROR_UNKNOWN_NODE,
        ERROR_UNKNOWN_TRANSACTION,
        ERROR_UNKNOWN_ACCOUNT,
        ERROR_UNKNOWN_WALLET,
        ERROR_UNKNOWN_BLOCK,
        ERROR_UNKNOWN_LISTENER,

        // Node
        ERROR_NODE_NOT_CONNECTED,

        // Transaction
        ERROR_TRANSACTION_X,

        // Acount
        // Wallet
        // Block
        // Listener

        // Numeric
        ERROR_NUMERIC_PARSE,
    }

    static int NUMBER_OF_STATUS_EVENTS = 10;

    //
    // Wallet Event
    //
    public enum WalletEvent {
        CREATED,
        BALANCE_UPDATED,
        DEFAULT_GAS_LIMIT_UPDATED,
        DEFAULT_GAS_PRICE_UPDATED,
        DELETED
    }

    static int NUMBER_OF_WALLET_EVENTS = 5;

    //
    // Token Event
    //
    public enum TokenEvent {
        CREATED,
        DELETED
    }

    static int NUMBER_OF_TOKEN_EVENTS = 2;

    //
    // Block Event
    //
    public enum BlockEvent {
        CREATED,
        CHAINED,
        ORPHANED,
        DELETED
    }

    static int NUMBER_OF_BLOCK_EVENT = 4;

    //
    // Transaction Event
    //
    public enum TransactionEvent {
        CREATED,
        SIGNED,
        SUBMITTED,
        INCLUDED,  // aka confirmed
        ERRORED,
        GAS_ESTIMATE_UPDATED,
        BLOCK_CONFIRMATIONS_UPDATED,
        DELETED
    }

    static int NUMBER_OF_TRANSACTION_EVENTS = 8;

    //
    // EWM Event
    //
    public enum EWMEvent {
        CREATED,
        SYNC_STARTED,
        SYNC_CONTINUES,
        SYNC_STOPPED,
        NETWORK_UNAVAILABLE,
        DELETED
    }

    static int NUMBER_OF_EWM_EVENTS = 6;

    //
    // Peer Event
    //
    public enum PeerEvent {
        CREATED,
        DELETED
    }

    static int NUMBER_OF_PEER_EVENTS = 2;

    //
    // Client
    //
    public interface Client {
        //        typedef void (*BREthereumClientHandlerGetGasPrice) (BREthereumClientContext context,
        //                                    BREthereumEWM ewm,
        //                                    BREthereumWalletId wid,
        //                                    int rid);
        void getGasPrice(long wid, int rid);

        //        typedef void (*BREthereumClientHandlerEstimateGas) (BREthereumClientContext context,
        //                                    BREthereumEWM ewm,
        //                                    BREthereumWalletId wid,
        //                                    BREthereumTransactionId tid,
        //                                    const char *from,
        //                                    const char *to,
        //                                    const char *amount,
        //                                    const char *data,
        //                                    int rid);

        void getGasEstimate(long wid, long tid, String from,  String to, String amount, String data, int rid);

        //        typedef void (*BREthereumClientHandlerGetBalance) (BREthereumClientContext context,
        //                                   BREthereumEWM ewm,
        //                                   BREthereumWalletId wid,
        //                                   const char *address,
        //                                   int rid);
        void getBalance(long wid, String address, int rid);

        //        typedef void (*BREthereumClientHandlerSubmitTransaction) (BREthereumClientContext context,
        //                                          BREthereumEWM ewm,
        //                                          BREthereumWalletId wid,
        //                                          BREthereumTransactionId tid,
        //                                          const char *transaction,
        //                                          int rid);
        void submitTransaction(long wid, long tid, String rawTransaction, int rid);

        //        typedef void (*BREthereumClientHandlerGetTransactions) (BREthereumClientContext context,
        //                                        BREthereumEWM ewm,
        //                                        const char *address,
        //                                        int rid);
        void getTransactions(String address, long begBlockNumber, long endBlockNumber, int rid);

        //        typedef void (*BREthereumClientHandlerGetLogs) (BREthereumClientContext context,
        //                                BREthereumEWM ewm,
        //                                const char *contract,
        //                                const char *address,
        //                                const char *event,
        //                                int rid);
        void getLogs(String contract, String address, String event, long begBlockNumber, long endBlockNumber, int rid);


        //typedef void
        //(*BREthereumClientHandlerGetBlocks) (BREthereumClientContext context,
        //                                     BREthereumEWM ewm,
        //                                     const char *address,
        //                                     BREthereumSyncInterestSet interests,
        //                                     uint64_t blockNumberStart,
        //                                     uint64_t blockNumberStop,
        //                                     int rid);
        void getBlocks (String address, int interests, long blockNumberStart, long blockNumberStop, int rid);

        void getTokens(int rid);

        //        typedef void (*BREthereumClientHandlerGetBlockNumber) (BREthereumClientContext context,
        //                                                    BREthereumEWM ewm,
        //                                                    int rid);
        void getBlockNumber(int rid);

        //        typedef void (*BREthereumClientHandlerGetNonce) (BREthereumClientContext context,
        //                                                        BREthereumEWM ewm,
        //                                                        const char *address,
        //                                                        int rid);
        void getNonce(String address, int rid);

        //
        void handleEWMEvent(EWMEvent event,
                            Status status,
                            String errorDescription);

        void handlePeerEvent(PeerEvent event,
                             Status status,
                             String errorDescription);

        void handleWalletEvent(BREthereumWallet wallet, WalletEvent event,
                               Status status,
                               String errorDescription);

        void handleTokenEvent(BREthereumToken token, TokenEvent event);

        void handleBlockEvent(BREthereumBlock block, BlockEvent event,
                              Status status,
                              String errorDescription);

        void handleTransferEvent(BREthereumWallet wallet,
                                 BREthereumTransfer transaction,
                                 TransactionEvent event,
                                 Status status,
                                 String errorDescription);
    }

    //
    // Client Announcers
    //

    public void announceBalance(long wid, String balance, int rid) {
        jniAnnounceBalance(wid, balance, rid);
    }

    public void announceGasPrice(long wid, String gasPrice, int rid) {
        jniAnnounceGasPrice(wid, gasPrice, rid);
    }

    public void announceGasEstimate(long wid, long tid, String gasEstimate, int rid) {
        jniAnnounceGasEstimate(wid, tid, gasEstimate, rid);
    }

    public void announceSubmitTransaction(long wid, long tid, String hash, int errorCode, String errorMessage, int rid) {
        jniAnnounceSubmitTransaction(wid, tid, hash, errorCode, errorMessage, rid);
    }

    public void announceTransaction(int id,
                                    String hash,
                                    String from,
                                    String to,
                                    String contract,
                                    String amount, // value
                                    String gasLimit,
                                    String gasPrice,
                                    String data,
                                    String nonce,
                                    String gasUsed,
                                    String blockNumber,
                                    String blockHash,
                                    String blockConfirmations,
                                    String blockTransactionIndex,
                                    String blockTimestamp,
                                    // cumulative gas used,
                                    // confirmations
                                    // txreceipt_status
                                    String isError) {
        jniAnnounceTransaction(id, hash, from, to, contract, amount, gasLimit, gasPrice, data, nonce, gasUsed,
                blockNumber, blockHash, blockConfirmations, blockTransactionIndex, blockTimestamp,
                isError);
    }

    public void announceTransactionComplete (int id, boolean success) {
        jniAnnounceTransactionComplete(id, success);
    }

    public void announceLog(int id,
                            String hash,
                            String contract,
                            String[] topics,
                            String data,
                            String gasPrice,
                            String gasUsed,
                            String logIndex,
                            String blockNumber,
                            String blockTransactionIndex,
                            String blockTimestamp) {
        jniAnnounceLog(id, hash, contract, topics, data, gasPrice, gasUsed, logIndex,
                blockNumber, blockTransactionIndex, blockTimestamp);
    }

    public void announceLogComplete (int id, boolean success) {
        jniAnnounceLogComplete(id, success);
    }

    public void announceBlockNumber(String blockNumber, int rid) {
        jniAnnounceBlockNumber(blockNumber, rid);
    }

    public void announceNonce(String address, String nonce, int rid) {
        jniAnnounceNonce(address, nonce, rid);
    }

    public void announceToken(String address,
                              String symbol,
                              String name,
                              String description,
                              int decimals,
                              String defaultGasLimit,
                              String defaultGasPrice,
                              int rid) {
        jniAnnounceToken(address, symbol, name, description, decimals,
                defaultGasLimit, defaultGasPrice,
                rid);
    }

    public void announceTokenComplete (int rid, boolean success) {
        jniAnnounceTokenComplete (rid, success);
    }
    //
    // EWM
    //
    WeakReference<Client> client;

    //
    // Network
    //
    BREthereumNetwork network;

    public BREthereumNetwork getNetwork() {
        return network;
    }

    //
    // Account
    //
    BREthereumAccount account;

    public BREthereumAccount getAccount() {
        return account;
    }

    public String getAddress() {
        return account.getPrimaryAddress();
    }

    public byte[] getAddressPublicKey() {
        return account.getPrimaryAddressPublicKey();
    }


    protected Executor executor = Executors.newSingleThreadExecutor();

    //
    // Wallet
    //
    // We hold a mapping, from identifier to wallet, for all wallets held/managed by this node.
    // The Core already holds wallets and thus we don't actually need to 'duplicate' that
    // functionality; however, to not hold wallets here would mean that every getWallet(), every
    // event handler would need to create another Wallet (feeling like a 'value type').  We don't
    // do that - but we could, and might some day.
    //
    // We could hold a WeakReference (and we probably should) - but, at least with the current
    // Android app, we witnessed wallets being reclaimed between each event update.  The consequence
    // was that we re-created the wallets each time; only to have them reclaimed.  Now, that is
    // actually not that big a deal and it should disappear completely when the Android app holds
    // on to wallets that have transactions.
    //
    // Of course, if the wallet shows up, then it is in Core Ethereum, and it shouldn't be
    // a WeakReference() - since it clearly exists in Core.  We'll leave this as a string
    // reference and explicitly delete wallets on a 'DELETE' event.
    //
    protected Map<Long, BREthereumWallet> wallets = new HashMap<>();

    protected synchronized BREthereumWallet walletLookupOrCreate(long wid, BREthereumToken token) {
        BREthereumWallet wallet = wallets.get(wid);

        // If we never had a wallet, then create one.
        if (null == wallet) {

            // If `token` is null, then lookup the token for wallet.
            if (null == token) {
                long tokenRef = jniEWMWalletGetToken(wid);
                if (0 != tokenRef)
                    token = lookupTokenByReference(tokenRef);
            }

            wallet = (null == token
                    ? new BREthereumWallet(this, wid, account, network)
                    : new BREthereumWallet(this, wid, account, network, token));

            wallets.put(wid, wallet);

        }

        return wallet;
    }

    public BREthereumWallet getWallet() {
        long wid = jniEWMGetWallet();
        return walletLookupOrCreate(wid, null);
    }

    public BREthereumWallet getWallet(BREthereumToken token) {
        long wid = jniEWMGetWalletToken(token.getIdentifier());
        return walletLookupOrCreate(wid, token);
    }

    // TODO: Remove once 'client callbacks' are EWM trampolines
    public BREthereumWallet getWalletByIdentifier(long wid) {
        return walletLookupOrCreate(wid, null);
    }

    //
    // Transaction
    //
    // We'll hold a mapping, from identifier to transaction, for all transactions.
    //
    protected Map<Long, WeakReference<BREthereumTransfer>> transactions = new HashMap<>();

    protected synchronized BREthereumTransfer transactionLookupOrCreate(long tid) {
        WeakReference<BREthereumTransfer> transactionRef = transactions.get(tid);

        if (null == transactionRef || null == transactionRef.get()) {
            long tokenReference = jniTransactionGetToken(tid);

            transactionRef = new WeakReference<>(
                    new BREthereumTransfer(this, tid,
                            (0 == tokenReference
                                    ? BREthereumAmount.Unit.ETHER_ETHER
                                    : BREthereumAmount.Unit.TOKEN_DECIMAL)));
            transactions.put(tid, transactionRef);
        }

        return transactionRef.get();
    }

    //
    // Block
    //
    protected Map<Long, BREthereumBlock> blocks = new HashMap<>();

    protected synchronized BREthereumBlock blockLookupOrCreate(long bid) {
        BREthereumBlock block = blocks.get(bid);

        if (null == block) {
            block = new BREthereumBlock(this, bid);
            blocks.put(bid, block);
        }
        return block;
    }

    public long getBlockHeight() {
        return jniEWMGetBlockHeight();
    }

    //
    // Tokens
    //
    protected final HashMap<String, BREthereumToken> tokensByAddress   = new HashMap<>();
    protected final HashMap<Long,   BREthereumToken> tokensByReference = new HashMap<>();

    public synchronized BREthereumToken[] getTokens() {
        BREthereumToken[] tokens = new BREthereumToken [tokensByAddress.size()];

        int i = 0;
        for (BREthereumToken token : tokensByAddress.values())
            tokens[i++] = token;
        return tokens;
    }

    protected synchronized BREthereumToken lookupTokenByReference(long reference) {
        return tokensByReference.get(reference);
    }

    protected synchronized BREthereumToken addTokenByReference (long reference) {
        BREthereumToken token = new BREthereumToken (reference);
        tokensByReference.put (token.getIdentifier(),            token);
        tokensByAddress.put   (token.getAddress().toLowerCase(), token);
        return token;
    }
    public @Nullable synchronized BREthereumToken getTokenBRD () {
        return lookupTokenByReference(jniGetTokenBRD());
    }

    public @Nullable synchronized BREthereumToken lookupToken(String address) {
        return tokensByAddress.get(address.toLowerCase());
    }

    public void updateTokens () { jniUpdateTokens(); }

    //
    // Constructor
    //

    public BREthereumEWM(Client client, BREthereumNetwork network, String storagePath, String paperKey, String[] wordList) {
        this(BREthereumEWM.jniCreateEWM(client, network.getIdentifier(), storagePath, paperKey, wordList),
                client, network);
    }

    public BREthereumEWM(Client client, BREthereumNetwork network, String storagePath, byte[] publicKey) {
        this(BREthereumEWM.jniCreateEWM_PublicKey(client, network.getIdentifier(), storagePath, publicKey),
                client, network);
    }


    private BREthereumEWM(long identifier, Client client, BREthereumNetwork network) {
        super(identifier);

        // Map identifier->this - for use in statically-declared trampoline functions.
        ewmMap.put(identifier, new WeakReference<BREthereumEWM> (this));

        this.client = new WeakReference<>(client);
        this.network = network;
        this.account = new BREthereumAccount(this, jniEWMGetAccount());
    }

    //
    // Connect // Disconnect
    //
    public boolean connect() {
        return jniEWMConnect();
    }

    public boolean disconnect() {
        return jniEWMDisconnect();
    }

    public static boolean addressIsValid (String address) {
        assert (null != address);
        return jniAddressIsValid(address);
    }

    static void ensureValidAddress (String address) {
        if (!addressIsValid(address))
            throw new RuntimeException ("Invalid Ethereum Address");
    }

    //
    // JNI: Constructors
    //
    protected static native long jniCreateEWM(Client client, long network, String storagePath, String paperKey, String[] wordList);

    protected static native long jniCreateEWM_PublicKey(Client client, long network, String storagePath, byte[] publicKey);

    protected static native boolean jniAddressIsValid (String address);

    //
    // JNI: Announcements
    //
    protected native void jniAnnounceTransaction(int id,
                                                 String hash,
                                                 String from,
                                                 String to,
                                                 String contract,
                                                 String amount, // value
                                                 String gasLimit,
                                                 String gasPrice,
                                                 String data,
                                                 String nonce,
                                                 String gasUsed,
                                                 String blockNumber,
                                                 String blockHash,
                                                 String blockConfirmations,
                                                 String blockTransactionIndex,
                                                 String blockTimestamp,
                                                 // cumulative gas used,
                                                 // confirmations
                                                 // txreceipt_status
                                                 String isError);

    protected native void jniAnnounceTransactionComplete (int id, boolean success);

    protected native void jniAnnounceLog(int id,
                                         String hash,
                                         String contract,
                                         String[] topics,
                                         String data,
                                         String gasPrice,
                                         String gasUsed,
                                         String logIndex,
                                         String blockNumber,
                                         String blockTransactionIndex,
                                         String blockTimestamp);

    protected native void jniAnnounceLogComplete (int id, boolean success);

    protected native void jniAnnounceBalance(long wid, String balance, int rid);

    protected native void jniAnnounceGasPrice(long wid, String gasPrice, int rid);

    protected native void jniAnnounceGasEstimate(long wid, long tid, String gasEstimate, int rid);

    protected native void jniAnnounceSubmitTransaction(long wid, long tid, String hash, int errorCode, String errorMessage, int rid);

    protected native void jniAnnounceBlockNumber(String blockNumber, int rid);

    protected native void jniAnnounceNonce(String address, String nonce, int rid);

    protected native void jniAnnounceToken (String address,
                                            String symbol,
                                            String name,
                                            String description,
                                            int decimals,
                                            String defaultGasLimit,
                                            String defaultGasPrice,
                                            int rid);

    protected native void jniAnnounceTokenComplete (int rid, boolean success);

    // JNI: Account & Address
    protected native long jniEWMGetAccount();

    protected native String jniGetAccountPrimaryAddress(long accountId);

    protected native byte[] jniGetAccountPrimaryAddressPublicKey(long accountId);

    protected native byte[] jniGetAccountPrimaryAddressPrivateKey(long accountId, String paperKey);

    // JNI: Wallet
    protected native long jniEWMGetWallet();

    protected native long jniEWMGetWalletToken(long tokenId);

    protected native long jniEWMCreateWalletToken(long tokenId);

    protected native long jniEWMWalletGetToken(long wid);

    protected native String jniGetWalletBalance(long walletId, long unit);

    protected native void jniEstimateWalletGasPrice(long walletId);

    protected native long jniWalletGetDefaultGasPrice(long wallet);

    protected native void jniWalletSetDefaultGasPrice(long wallet, long value);

    protected native long jniWalletGetDefaultGasLimit(long wallet);

    protected native void jniWalletSetDefaultGasLimit(long wallet, long value);

    //
    // JNI: Wallet Transactions
    //
    protected native long jniCreateTransaction(long walletId,
                                               String to,
                                               String amount,
                                               long amountUnit);

    protected native long jniCreateTransactionGeneric(long walletId,
                                                      String to,
                                                      String amount,
                                                      long amountUnit,
                                                      String gasPrice,
                                                      long gasPriceUnit,
                                                      String gasLimit,
                                                      String data);

    protected native void jniSignTransaction(long walletId,
                                             long transactionId,
                                             String paperKey);

    protected native void jniSignTransactionWithPrivateKey(long walletId,
                                                           long transactionId,
                                                           byte[] privateKey);

    protected native void jniSubmitTransaction(long walletId,
                                               long transactionId);

    protected native long[] jniGetTransactions(long walletId);

    protected native void jniTransactionEstimateGas(long walletId,
                                                    long transactionId);

    protected native String jniTransactionEstimateFee(long walletId,
                                                      String amount,
                                                      long amountUnit,
                                                      long resultUnit);

    //
    // JNI: Transactions
    //
    protected native boolean jniTransactionHasToken(long transactionId);

    protected native String jniTransactionGetAmount(long transactionId, long unit);

    protected native String jniTransactionGetFee(long transactionId, long unit);

    protected native String jniTransactionSourceAddress(long transactionId);

    protected native String jniTransactionTargetAddress(long transactionId);

    protected native String jniTransactionGetIdentifier(long transactionId);

    protected native String jniTransactionOriginatingTransactionHash(long transactionId);

    protected native String jniTransactionGetGasPrice(long transactionId, long unit);

    protected native long jniTransactionGetGasLimit(long transactionId);

    protected native long jniTransactionGetGasUsed(long transactionId);

    protected native long jniTransactionGetNonce(long transactionId);

    protected native long jniTransactionGetBlockNumber(long transactionId);

    protected native long jniTransactionGetBlockTimestamp(long transactionId);

    protected native long jniTransactionGetBlockConfirmations(long transactionId);

    protected native long jniTransactionGetToken(long transactionId);

    protected native boolean jniTransactionIsConfirmed(long transactionId);

    protected native boolean jniTransactionIsSubmitted(long transactionId);

    protected native boolean jniTransactionIsErrored(long transactionId);

    protected native String jniTransactionGetErrorDescription(long transactionId);

    //
    // JNI: Tokens
    //
    // protected native String jniTokenGetAddress (long tokenId);
    protected native void jniUpdateTokens();

    //
    // JNI: Block
    //
    protected native long jniEWMGetBlockHeight();

//    protected native long jniBlockGetNumber(long bid);
//    //    protected native long jniBlockGetTimestamp (long bid);
//    protected native String jniBlockGetHash(long bid);

    //
    // JNI: Connect & Disconnect
    //
    protected native boolean jniEWMConnect();

    protected native boolean jniEWMDisconnect();

    // JNI: Initialize
    protected static native void initializeNative();

    static {
        initializeNative();
    }

    //
    // Support
    //

    //
    // Reference
    //
    static class Reference {
        WeakReference<BREthereumEWM> ewm;
        long identifier;

        Reference(BREthereumEWM ewm, long identifier) {
            this.ewm = new WeakReference<>(ewm);
            this.identifier = identifier;
        }

        static {
            try { System.loadLibrary("core"); }
            catch (UnsatisfiedLinkError e) {
                e.printStackTrace();
                System.err.println ("Native code library failed to load.\\n\" + " + e);
            }
        }
    }

    //
    // Reference With Default Unit
    //
    static class ReferenceWithDefaultUnit extends Reference {
        protected BREthereumAmount.Unit defaultUnit;
        protected boolean defaultUnitUsesToken = false;

        public BREthereumAmount.Unit getDefaultUnit() {
            return defaultUnit;
        }

        public void setDefaultUnit(BREthereumAmount.Unit unit) {
            validUnitOrException(unit);
            this.defaultUnit = unit;
        }

        //
        // Constructor
        //
        protected ReferenceWithDefaultUnit(BREthereumEWM ewm,
                                           long identifier,
                                           BREthereumAmount.Unit unit) {
            super(ewm, identifier);
            this.defaultUnit = unit;
            this.defaultUnitUsesToken = unit.isTokenUnit();
        }

        //
        // Support
        //
        protected boolean validUnit(BREthereumAmount.Unit unit) {
            return (!defaultUnitUsesToken
                    ? (unit == BREthereumAmount.Unit.ETHER_WEI || unit == BREthereumAmount.Unit.ETHER_GWEI || unit == BREthereumAmount.Unit.ETHER_ETHER)
                    : (unit == BREthereumAmount.Unit.TOKEN_DECIMAL || unit == BREthereumAmount.Unit.TOKEN_INTEGER));
        }

        protected void validUnitOrException(BREthereumAmount.Unit unit) {
            if (!validUnit(unit))
                throw new IllegalArgumentException("Invalid Unit for instance type: " + unit.toString());
        }
    }

    //
    // Map EID -> BREthereumEWM
    //
    static protected Map<Long, WeakReference<BREthereumEWM>> ewmMap = new HashMap<>();

    static BREthereumEWM lookupEWM (long eid) {
        WeakReference<BREthereumEWM> ewm = ewmMap.get(eid);
        return (null== ewm ? null : ewm.get());
    }

    static Client lookupClient (BREthereumEWM ewm) {
        return (null == ewm ? null : ewm.client.get());
    }

    //
    // Callback Announcements :: JNI
    //
    // In the JNI Code, we had a problem directly accessing the Client methods.  We had to
    // dereference the WeakReference<Client> to get at the client and then its methods.  So instead
    // we'll access these methods below and then 'bounce' to method calls for the listener.
    //
    // These methods also give us a chance to convert the `event`, as a `long`, to the Event.
    //
    static protected void trampolineGetGasPrice(final long eid, final long wid, final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getGasPrice(wid, rid);
            }
        });
    }

    static protected void trampolineGetGasEstimate(final long eid, final long wid, final long tid,
                                                   final String from,
                                                   final String to,
                                                   final String amount,
                                                   final String data,
                                                   final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getGasEstimate(wid, tid, from, to, amount, data, rid);
            }
        });
    }

    static protected void trampolineGetBalance(final long eid, final long wid,
                                               final String address,
                                               final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getBalance(wid, address, rid);
            }
        });
    }

    static protected void trampolineSubmitTransaction(final long eid, final long wid, final long tid,
                                                      final String rawTransaction,
                                                      final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.submitTransaction(wid, tid, rawTransaction, rid);
            }
        });
    }

    static protected void trampolineGetTransactions(final long eid,
                                                    final String address,
                                                    final long begBlockNumber,
                                                    final long endBlockNumber,
                                                    final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getTransactions(address, begBlockNumber, endBlockNumber, rid);
            }
        });
    }

    static protected void trampolineGetLogs(final long eid,
                                            final String contract,
                                            final String address,
                                            final String event,
                                            final long begBlockNumber,
                                            final long endBlockNumber,
                                            final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getLogs(contract, address, event, begBlockNumber, endBlockNumber, rid);
            }
        });
    }

    static protected void trampolineGetBlocks(final long eid,
                                              final String address,
                                              final int interests,
                                              final long blockNumberStart,
                                              final long blockNumberStop,
                                              final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;
        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getBlocks(address, interests, blockNumberStart, blockNumberStop, rid);
            }
        });
    }

    static protected void trampolineGetTokens(final long eid,
                                              final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getTokens(rid);
            }
        });
    }

    static protected void trampolineGetBlockNumber(final long eid,
                                                   final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getBlockNumber(rid);
            }
        });
    }

    static protected void trampolineGetNonce(final long eid,
                                             final String address,
                                             final int rid) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.getNonce(address, rid);
            }
        });
    }

    static protected void trampolineEWMEvent(final long eid,
                                             final int event,
                                             final int status,
                                             final String errorDescription) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.handleEWMEvent(EWMEvent.values()[event],
                        Status.values()[status],
                        errorDescription);
            }
        });
    }

    static protected void trampolinePeerEvent(final long eid,
                                              final int event,
                                              final int status,
                                              final String errorDescription) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.handlePeerEvent(PeerEvent.values()[event],
                        Status.values()[status],
                        errorDescription);
            }
        });
    }

    static protected void trampolineWalletEvent(final long eid,
                                                final long wid,
                                                final int event,
                                                final int status,
                                                final String errorDescription) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        // TODO: Resolve Bug
        if (event < 0 || event >= NUMBER_OF_WALLET_EVENTS) return;
        if (status < 0 || status >= NUMBER_OF_STATUS_EVENTS) return;

        // Lookup the wallet - this will create the wallet if it doesn't exist.  Thus, if the
        // `event` is `create`, we get a wallet; and even, if the `event` is `delete`, we get a
        // wallet too.
        final BREthereumWallet wallet = ewm.walletLookupOrCreate(wid, null);

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                // Invoke handler
                client.handleWalletEvent(wallet,
                        WalletEvent.values()[(int) event],
                        Status.values()[(int) status],
                        errorDescription);
            }
        });
    }

    static protected void trampolineTokenEvent(final long eid,
                                               final long tokenId,
                                               final int event) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client)
            return;

        // TODO: Resolve Bug
        if (event < 0 || event >= NUMBER_OF_TOKEN_EVENTS)
            return;

        final BREthereumToken token = ewm.addTokenByReference(tokenId);

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                // Invoke handler
                client.handleTokenEvent(token, TokenEvent.values()[(int) event]);
            }
        });
    }

    /*
        static protected void trampolineBlockEvent(long eid, int bid, int event, int status, String errorDescription) {
            final BREthereumEWM ewm = lookupEWM(eid);
            final Client client = lookupClient (ewm);
            if (null == client) return;

            // TODO: Resolve Bug
            if (event < 0 || event >= NUMBER_OF_BLOCK_EVENT) return;
            if (status < 0 || status >= NUMBER_OF_STATUS_EVENTS) return;

            // Nothing, at this point
            BREthereumBlock block = ewm.blockLookupOrCreate(bid);

            client.handleBlockEvent(block,
                    BlockEvent.values()[(int) event],
                    Status.values()[(int) status],
                    errorDescription);
        }
    */
    static protected void trampolineTransferEvent(final long eid, final long wid, final long tid,
                                                  final int event,
                                                  final int status,
                                                  final String errorDescription) {
        final BREthereumEWM ewm = lookupEWM(eid);
        final Client client = lookupClient(ewm);
        if (null == client) return;

        // TODO: Resolve Bug
        if (event < 0 || event >= NUMBER_OF_TRANSACTION_EVENTS) return;
        if (status < 0 || status >= NUMBER_OF_STATUS_EVENTS) return;

        final BREthereumWallet wallet = ewm.walletLookupOrCreate(wid, null);
        final BREthereumTransfer transaction = ewm.transactionLookupOrCreate(tid);

        ewm.executor.execute(new Runnable() {
            @Override
            public void run() {
                client.handleTransferEvent(wallet, transaction,
                        TransactionEvent.values()[(int) event],
                        Status.values()[(int) status],
                        errorDescription);
            }
        });
    }
}
