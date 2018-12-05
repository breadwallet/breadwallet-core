package com.breadwallet.crypto;

public abstract class Wallet {
    public final WalletManager manager;

    public final String name;

    public final Currency currency;

    public abstract Amount getBalance ();

    public abstract Transfer[] getTransfers();


    /*
    /// The owning manager
    var manager: WalletManager { get }

    /// The name
    var name: String { get }

    /// The current balance for currency
    var balance: Amount { get }

    /// The transfers of currency yielding `balance`
    var transfers: [Transfer] { get }

    /// Use a hash to lookup a transfer
    func lookup (transfer: TransferHash) -> Transfer?

    /// The current state.
    var state: WalletState { get }

    /// The default TransferFeeBasis for created transfers.
    var defaultFeeBasis: TransferFeeBasis { get set }

    /// The default TransferFactory for creating transfers.
    var transferFactory: TransferFactory { get set }

    // func sign (transfer: Transfer)
    // submit
    // ... cancel, replace - if appropriate

    /// An address suitable for a transfer target (receiving).
    var target: Address { get }
*/

    public Wallet (WalletManager manager, Currency currency, String name) {
        this.manager = manager;
        this.currency = currency;
        this.name = name;
    }
}
