package com.breadwallet.crypto;

public abstract class Transfer {
    public Wallet wallet;

    public Address source;

    public Address target;

    public Amount amount;

    public Amount fee;

    /*
    /// The owning wallet
    var wallet: Wallet { get }

    /// The source pays the fee and sends the amount.
    var source: Address? { get }

    /// The target receives the amount
    var target: Address? { get }

    /// The amount to transfer
    var amount: Amount { get }

    /// The fee paid - before the transfer is confirmed, this is the estimated fee.
    var fee: Amount { get }

    /// The basis for the fee.
    var feeBasis: TransferFeeBasis { get }

    /// An optional confirmation.
    var confirmation: TransferConfirmation? { get }

    /// An optional hash
    var hash: TransferHash? { get }

    /// The current state
    var state: TransferState { get }
*/

    protected Transfer(Wallet wallet, Address source, Address target, Amount amount, Amount fee) {
        this.wallet = wallet;
        this.source = source;
        this.target = target;
        this.amount = amount;
        this.fee = fee;
    }
}
