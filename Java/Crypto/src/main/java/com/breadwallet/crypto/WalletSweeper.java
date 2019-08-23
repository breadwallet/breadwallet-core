package com.breadwallet.crypto;

import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.google.common.base.Optional;

public interface WalletSweeper {

    interface WalletSweeperError {}

    Optional<? extends Amount> getBalance();

    void getEstimatedFeeBasis(NetworkFee fee, CompletionHandler<TransferFeeBasis, FeeEstimationError> completion);

    void submit(TransferFeeBasis feeBasis);
}
