/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/29/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corecrypto;

import com.breadwallet.corenative.cleaner.ReferenceCleaner;
import com.breadwallet.corenative.crypto.BRCryptoPayProtReqBitPayAndBip70Callbacks;
import com.breadwallet.corenative.crypto.BRCryptoPayProtReqBitPayAndBip70Callbacks.BitPayAndBip70Validator;
import com.breadwallet.corenative.crypto.BRCryptoPayProtReqBitPayAndBip70Callbacks.BitPayAndBip70CommonNameExtractor;
import com.breadwallet.corenative.crypto.BRCryptoPaymentProtocolError;
import com.breadwallet.corenative.crypto.BRCryptoPaymentProtocolRequest;
import com.breadwallet.corenative.crypto.BRCryptoPaymentProtocolRequestBitPayBuilder;
import com.breadwallet.crypto.PaymentProtocolRequestType;
import com.breadwallet.crypto.errors.FeeEstimationError;
import com.breadwallet.crypto.errors.PaymentProtocolError;
import com.breadwallet.crypto.utility.CompletionHandler;
import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Optional;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;
import com.google.common.primitives.UnsignedLong;
import com.google.common.primitives.UnsignedLongs;

import java.security.InvalidKeyException;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.security.Signature;
import java.security.SignatureException;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.Date;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.net.ssl.TrustManager;
import javax.net.ssl.TrustManagerFactory;
import javax.net.ssl.X509TrustManager;

import static com.google.common.base.Preconditions.checkNotNull;

/* package */
final class PaymentProtocolRequest implements com.breadwallet.crypto.PaymentProtocolRequest {

    /* package */
    static Optional<PaymentProtocolRequest> createForBitPay(com.breadwallet.crypto.Wallet w, String json) {
        Wallet wallet = Wallet.from(w);
        WalletManager manager = wallet.getWalletManager();
        Network network = manager.getNetwork();
        Currency currency = wallet.getCurrency();

        if (!BRCryptoPaymentProtocolRequest.validateForBitPay(
                network.getCoreBRCryptoNetwork(),
                currency.getCoreBRCryptoCurrency(),
                wallet.getCoreBRCryptoWallet())) {
            return Optional.absent();
        }

        Optional<BitPayRequest> maybeRequest = BitPayRequest.asBitPayRequest(json);
        if (!maybeRequest.isPresent()) {
            return Optional.absent();
        }
        BitPayRequest request = maybeRequest.get();

        Optional<BRCryptoPaymentProtocolRequestBitPayBuilder> maybeBuilder = BRCryptoPaymentProtocolRequestBitPayBuilder.create(
                network.getCoreBRCryptoNetwork(),
                currency.getCoreBRCryptoCurrency(),
                BIT_PAY_AND_BIP_70_CALLBACKS,
                request.network,
                request.time,
                request.expires,
                request.requiredFeeRate,
                request.memo,
                request.paymentUrl,
                null
        );
        if (!maybeBuilder.isPresent()) {
            return Optional.absent();
        }
        BRCryptoPaymentProtocolRequestBitPayBuilder builder = maybeBuilder.get();

        try {
            for (BitPayOutput output: request.outputs) {
                builder.addOutput(output.address, output.amount);
            }

            return builder.build().transform(
                    p -> PaymentProtocolRequest.create(p, wallet)
            );
        } finally {
            builder.give();
        }
    }

    /* package */
    static Optional<PaymentProtocolRequest> createForBip70(com.breadwallet.crypto.Wallet w, byte[] serialization) {
        Wallet wallet = Wallet.from(w);
        WalletManager manager = wallet.getWalletManager();
        Network network = manager.getNetwork();
        Currency currency = wallet.getCurrency();

        if (!BRCryptoPaymentProtocolRequest.validateForBip70(
                network.getCoreBRCryptoNetwork(),
                currency.getCoreBRCryptoCurrency(),
                wallet.getCoreBRCryptoWallet())) {
            return Optional.absent();
        }

        return BRCryptoPaymentProtocolRequest.createForBip70(
                network.getCoreBRCryptoNetwork(),
                currency.getCoreBRCryptoCurrency(),
                BIT_PAY_AND_BIP_70_CALLBACKS,
                serialization
        ).transform(
                p -> PaymentProtocolRequest.create(p, wallet)
        );
    }

    /* package */
    static PaymentProtocolRequest create(BRCryptoPaymentProtocolRequest core, Wallet wallet) {;
        PaymentProtocolRequest request = new PaymentProtocolRequest(core, wallet);
        ReferenceCleaner.register(request, core::give);
        return request;
    }

    private final BRCryptoPaymentProtocolRequest core;
    private final WalletManager manager;
    private final Wallet wallet;

    private final Supplier<Boolean> isSecureSupplier;
    private final Supplier<Optional<String>> memoSupplier;
    private final Supplier<Optional<String>> paymentUrlSupplier;
    private final Supplier<Optional<String>> commonNameSupplier;
    private final Supplier<Optional<Amount>> amountSupplier;
    private final Supplier<Optional<Address>> addressSupplier;
    private final Supplier<Optional<NetworkFee>> networkFeeSupplier;
    private final Supplier<Optional<PaymentProtocolError>> validitySupplier;

    private PaymentProtocolRequest(BRCryptoPaymentProtocolRequest core, Wallet wallet) {
        this.core = core;
        this.manager = wallet.getWalletManager();
        this.wallet = wallet;

        this.isSecureSupplier = Suppliers.memoize(core::isSecure);
        this.memoSupplier = Suppliers.memoize(core::getMemo);
        this.paymentUrlSupplier = Suppliers.memoize(core::getPaymentUrl);
        this.commonNameSupplier = Suppliers.memoize(core::getCommonName);
        this.amountSupplier = Suppliers.memoize(() -> core.getTotalAmount().transform(Amount::create));
        this.addressSupplier = Suppliers.memoize(() -> core.getPrimaryTargetAddress().transform(Address::create));
        this.networkFeeSupplier = Suppliers.memoize(() -> core.getRequiredNetworkFee().transform(NetworkFee::create));
        this.validitySupplier = Suppliers.memoize(() -> Utilities.paymentProtocolErrorFromCrypto(core.isValid()));
    }

    @Override
    public PaymentProtocolRequestType getType() {
        return Utilities.paymentProtocolRequestTypeFromCrypto(core.getType());
    }

    @Override
    public boolean isSecure() {
        return isSecureSupplier.get();
    }

    @Override
    public Optional<String> getMemo() {
        return memoSupplier.get();
    }

    @Override
    public Optional<String> getPaymentUrl() {
        return paymentUrlSupplier.get();
    }

    @Override
    public Optional<Amount> getTotalAmount() {
        return amountSupplier.get();
    }

    @Override
    public Optional<Address> getPrimaryTarget() {
        return addressSupplier.get();
    }

    @Override
    public Optional<String> getCommonName() {
        return commonNameSupplier.get();
    }

    @Override
    public Optional<NetworkFee> getRequiredNetworkFee() {
        return networkFeeSupplier.get();
    }

    @Override
    public Optional<PaymentProtocolError> validate() {
        return validitySupplier.get();
    }

    @Override
    public void estimate(com.breadwallet.crypto.NetworkFee fee,
                         CompletionHandler<com.breadwallet.crypto.TransferFeeBasis, FeeEstimationError> completion) {
        wallet.estimateFee(this, fee, completion);
    }

    @Override
    public Optional<Transfer> createTransfer(com.breadwallet.crypto.TransferFeeBasis feeBasis) {
        return wallet.createTransfer(this, feeBasis);
    }

    @Override
    public boolean signTransfer(com.breadwallet.crypto.Transfer transfer, byte[] phraseUtf8) {
        return manager.sign(transfer, phraseUtf8);
    }

    @Override
    public void submitTransfer(com.breadwallet.crypto.Transfer transfer) {
        manager.submit(transfer);
    }

    @Override
    public Optional<PaymentProtocolPayment> createPayment(com.breadwallet.crypto.Transfer transfer) {
        return PaymentProtocolPayment.create(this, Transfer.from(transfer), wallet.getTarget());
    }

    /* package */
    BRCryptoPaymentProtocolRequest getBRCryptoPaymentProtocolRequest() {
        return core;
    }

    //
    // BitPay
    //

    private static final class BitPayRequest {

        static Optional<BitPayRequest> asBitPayRequest(String json) {
            ObjectMapper mapper = new ObjectMapper();

            BitPayRequest request;
            try {
                request = mapper.readValue(json, BitPayRequest.class);
            } catch (JsonProcessingException e) {
                request = null;
            }

            return Optional.fromNullable(request);
        }

        @JsonCreator
        static BitPayRequest create(@JsonProperty("network") String network,
                                    @JsonProperty("currency") String currency,
                                    @JsonProperty("requiredFeeRate") Double requiredFeeRate,
                                    @JsonProperty("time") Date time,
                                    @JsonProperty("expires") Date expires,
                                    @JsonProperty("memo") String memo,
                                    @JsonProperty("paymentUrl") String paymentUrl,
                                    @JsonProperty("paymentId") String paymentId,
                                    @JsonProperty("outputs") List<BitPayOutput> outputs) {
            return new BitPayRequest(
                    checkNotNull(network),
                    checkNotNull(currency),
                    checkNotNull(requiredFeeRate),
                    checkNotNull(time),
                    checkNotNull(expires),
                    checkNotNull(memo),
                    checkNotNull(paymentUrl),
                    checkNotNull(paymentId),
                    checkNotNull(outputs)
            );
        }

        private final String network;
        private final String currency;
        private final double requiredFeeRate;
        private final List<BitPayOutput> outputs;
        private final Date time;
        private final Date expires;
        private final String memo;
        private final String paymentUrl;
        private final String paymentId;

        private BitPayRequest(String network,
                              String currency,
                              double requiredFeeRate,
                              Date time,
                              Date expires,
                              String memo,
                              String paymentUrl,
                              String paymentId,
                              List<BitPayOutput> outputs) {
            this.network = network;
            this.currency = currency;
            this.requiredFeeRate = requiredFeeRate;
            this.outputs = outputs;
            this.time = time;
            this.expires = expires;
            this.memo = memo;
            this.paymentUrl = paymentUrl;
            this.paymentId = paymentId;
        }
    }

    private static final class BitPayOutput {

        @JsonCreator
        static BitPayOutput create(@JsonProperty("amount") String amount,
                                   @JsonProperty("address") String address) {
            return new BitPayOutput(
                    checkNotNull(UnsignedLong.fromLongBits(UnsignedLongs.decode(amount))),
                    checkNotNull(address)
            );
        }

        private final UnsignedLong amount;
        private final String address;

        BitPayOutput(UnsignedLong amount, String address) {
            this.amount = amount;
            this.address = address;
        }
    }

    //
    // Validation
    //

    private static final BitPayAndBip70Validator BIT_PAY_AND_BIP70_VALIDATOR = (request,
                                                                                cookie,
                                                                                pkiType,
                                                                                expires,
                                                                                certificates,
                                                                                digest,
                                                                                signature) -> {
        if (expires != 0 && (java.lang.System.currentTimeMillis() / 1000) > expires) {
            // request expired
            return BRCryptoPaymentProtocolError.CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED;

        } else if (BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_NONE.equals(pkiType)) {
            // no PKI work required
            return BRCryptoPaymentProtocolError.CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE;

        } else if (!BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_X509_SHA256.equals(pkiType) &&
                   !BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_X509_SHA1.equals(pkiType)) {
            // PKI check required but algorithm not supported
            return BRCryptoPaymentProtocolError.CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED;

        } else if (null == digest || null == signature || null == certificates || certificates.isEmpty()) {
            // PKI check required but no material provided
            return BRCryptoPaymentProtocolError.CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED;

        } else {
            // PKI check required, let's get after it
            X509Certificate[] certificatesArray = certificates.toArray(new X509Certificate[0]);

            // check the validity of the certificate chain
            try {
                String trustAlgName = pkiTypeToTrustAlgorithm(pkiType);
                verifyTrust(trustAlgName, certificatesArray);

            } catch (NoSuchAlgorithmException | KeyStoreException | CertificateException e) {
                return BRCryptoPaymentProtocolError.CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED;
            }

            // check the signature based on the validated leaf certificate
            try {
                PublicKey publicKey = certificatesArray[0].getPublicKey();
                String sigAlgName = pkiTypeToSignatureAlgorithm(pkiType);
                verifySignature(sigAlgName, publicKey, digest, signature);

            } catch (NoSuchAlgorithmException e) {
                return BRCryptoPaymentProtocolError.CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED;

            } catch (InvalidKeyException | SignatureException e) {
                return BRCryptoPaymentProtocolError.CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED;
            }

            // if we made it here, we are golden
            return BRCryptoPaymentProtocolError.CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE;
        }
    };

    private static final BitPayAndBip70CommonNameExtractor BIT_PAY_AND_BIP_70_COMMON_NAME_EXTRACTOR = (request,
                                                                                                       cookie,
                                                                                                       pkiType,
                                                                                                       name,
                                                                                                       certificates) -> {
        String extractedName;

        if (BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_NONE.equals(pkiType)) {
            extractedName = name;

        } else if (!BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_X509_SHA256.equals(pkiType) &&
                   !BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_X509_SHA1.equals(pkiType)) {
            // we don't support this type
            extractedName = null;

        } else if (null == certificates || certificates.isEmpty()) {
            // nothing to extract from
            extractedName = null;

        } else {
            // extract the name from the PKI data
            extractedName = null;
            for (X509Certificate certificate: certificates) {
                extractedName  = getCommonName(certificate);
                if (null != extractedName) {
                    break;
                }
            }
        }

        return Optional.fromNullable(extractedName);
    };

    private static final BRCryptoPayProtReqBitPayAndBip70Callbacks BIT_PAY_AND_BIP_70_CALLBACKS = new BRCryptoPayProtReqBitPayAndBip70Callbacks(
            null,
            BIT_PAY_AND_BIP70_VALIDATOR,
            BIT_PAY_AND_BIP_70_COMMON_NAME_EXTRACTOR
    );

    private static String pkiTypeToTrustAlgorithm(String pkiType)
            throws NoSuchAlgorithmException {
        switch (pkiType) {
            case BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_X509_SHA256:    return "RSA";
            case BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_X509_SHA1:      return "RSA";
            default: throw new NoSuchAlgorithmException();
        }
    }

    private static String pkiTypeToSignatureAlgorithm(String pkiType)
            throws NoSuchAlgorithmException {
        switch (pkiType) {
            case BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_X509_SHA256:    return "SHA256withRSA";
            case BRCryptoPayProtReqBitPayAndBip70Callbacks.PKI_TYPE_X509_SHA1:      return "SHA1withRSA";
            default: throw new NoSuchAlgorithmException();
        }
    }

    private static void verifyTrust(String algName, X509Certificate[] certificates)
            throws NoSuchAlgorithmException, KeyStoreException, CertificateException {
        TrustManagerFactory tmf = TrustManagerFactory.getInstance("X509");
        tmf.init((KeyStore) null);

        for (TrustManager m : tmf.getTrustManagers()) {
            X509TrustManager xtm = (X509TrustManager) m;
            xtm.checkServerTrusted(certificates, algName);
        }
    }

    private static void verifySignature(String algName, PublicKey publicKey, byte[] digest, byte[] signature)
            throws NoSuchAlgorithmException, InvalidKeyException, SignatureException {
        Signature verifier = Signature.getInstance(algName);
        verifier.initVerify(publicKey);
        verifier.update(digest);
        verifier.verify(signature);
    }

    private static final Pattern COMMON_NAME_PATTERN    = Pattern.compile("(?:^|,\\s?)(?:CN=(?<val>\"(?:[^\"]|\"\")+\"|[^,]+))");
    private static final int COMMON_NAME_PATTERN_GROUP  = 1;

    private static String getCommonName(X509Certificate certificate) {
        String distinguishedName = certificate.getSubjectX500Principal().getName();
        if (distinguishedName != null) {

            Matcher matcher = COMMON_NAME_PATTERN.matcher(distinguishedName);
            if (matcher.find()) {
                return matcher.group(COMMON_NAME_PATTERN_GROUP);
            }
        }
        return null;
    }
}
