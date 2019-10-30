/*
 * Created by Michael Carrara <michael.carrara@breadwallet.com> on 10/29/19.
 * Copyright (c) 2019 Breadwinner AG.  All right reserved.
 *
 * See the LICENSE file at the project root for license information.
 * See the CONTRIBUTORS file at the project root for a list of contributors.
 */
package com.breadwallet.corenative.crypto;

import com.breadwallet.corenative.utility.Cookie;
import com.breadwallet.corenative.utility.SizeT;
import com.google.common.base.Optional;
import com.google.common.primitives.UnsignedInts;
import com.sun.jna.Callback;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import javax.annotation.Nullable;

public class BRCryptoPayProtReqBitPayAndBip70Callbacks extends Structure {

    public static final String PKI_TYPE_NONE        = "none";
    public static final String PKI_TYPE_X509_SHA256 = "x509+sha256";
    public static final String PKI_TYPE_X509_SHA1   = "x509+sha1";

    //
    // Implementation Detail
    //

    public interface BRCryptoPayProtReqBitPayAndBip70Validator extends Callback {
        int callback(Pointer request,
                     Pointer cookie,
                     String pkiType,
                     long expires,
                     @Nullable Pointer certBytes,
                     @Nullable Pointer certLengths,
                     SizeT certCount,
                     @Nullable Pointer digest,
                     SizeT digestLen,
                     @Nullable Pointer sig,
                     SizeT sigLen);
    }

    public interface BRCryptoPayProtReqBitPayAndBip70CommonNameExtractor extends Callback {
        Pointer callback(Pointer request,
                         Pointer cookie,
                         String pkiType,
                         @Nullable Pointer certBytes,
                         @Nullable Pointer certLengths,
                         SizeT certCount);
    }

    //
    // Client Interface
    //

    public interface BitPayAndBip70Validator extends BRCryptoPayProtReqBitPayAndBip70Validator {
        BRCryptoPaymentProtocolError handle (BRCryptoPaymentProtocolRequest request,
                                             Cookie cookie,
                                             String pkiType,
                                             long expires,
                                             List<X509Certificate> certificates,
                                             byte[] digest,
                                             byte[] signature);

        @Override
        default int callback(Pointer request,
                             Pointer cookie,
                             String pkiType,
                             long expires,
                             @Nullable Pointer certBytes,
                             Pointer certLengths,
                             SizeT certCount,
                             @Nullable Pointer digest,
                             SizeT digestLen,
                             @Nullable Pointer sig,
                             SizeT sigLen) {
            return handle(
                    new BRCryptoPaymentProtocolRequest(request),
                    new Cookie(cookie),
                    pkiType,
                    expires,
                    getCertificates(certBytes, certLengths, certCount).orNull(),
                    digest == null ? null : digest.getByteArray(0, digestLen.intValue()),
                    sig == null ? null : sig.getByteArray(0, sigLen.intValue())
            ).toCore();
        }
    }

    public interface BitPayAndBip70CommonNameExtractor extends BRCryptoPayProtReqBitPayAndBip70CommonNameExtractor {
        Optional<String> handle(BRCryptoPaymentProtocolRequest request,
                                Cookie cookie,
                                String pkiType,
                                @Nullable String name,
                                @Nullable List<X509Certificate> certificates);

        @Override
        default Pointer callback(Pointer request,
                                 Pointer cookie,
                                 String pkiType,
                                 @Nullable Pointer certBytes,
                                 Pointer certLengths,
                                 SizeT certCount) {
            boolean isNoneType = PKI_TYPE_NONE.equals(pkiType);
            Optional<String> name = isNoneType ? getCertificateNameHack(certBytes, certLengths, certCount) : Optional.absent();
            Optional<List<X509Certificate>> certificates = isNoneType ? Optional.absent() : getCertificates(certBytes, certLengths, certCount);

            return handle(
                    new BRCryptoPaymentProtocolRequest(request),
                    new Cookie(cookie),
                    pkiType,
                    name.orNull(),
                    certificates.orNull()
            ).transform(n -> {
                byte[] nullBytes = new byte[] {0};
                byte[] nameBytes = n.getBytes(StandardCharsets.UTF_8);
                Pointer namePointer = new Pointer(Native.malloc(nameBytes.length + 1));
                namePointer.write(0, nameBytes, 0, nameBytes.length);
                namePointer.write(nameBytes.length, nullBytes, 0, nullBytes.length);
                return namePointer;
            }).orNull();
        }
    }

    // non-standard extention to include an un-certified request name
    private static Optional<String> getCertificateNameHack(@Nullable Pointer certBytes, @Nullable Pointer certLengths, SizeT certCount) {
        int certCountValue = certCount.intValue();
        if (certCountValue == 0 || null == certBytes) {
            return Optional.absent();
        }

        Pointer[] certBytesArray = certBytes.getPointerArray(0, certCountValue);
        return Optional.of(certBytesArray[0].getString(0, "UTF-8"));
    }

    private static Optional<List<X509Certificate>> getCertificates(@Nullable Pointer certBytes, @Nullable Pointer certLengths, SizeT certCount) {
        int certCountValue = certCount.intValue();
        if (certCountValue == 0 || null == certBytes) {
            return Optional.absent();
        }

        Pointer[] certBytesArray = certBytes.getPointerArray(0, certCountValue);
        int[] certLengthsArray = getNativeSizeTArray(certLengths, certCountValue);
        return getCertificates(certBytesArray, certLengthsArray, certCountValue);
    }

    private static Optional<List<X509Certificate>> getCertificates(Pointer[] certBytes, int[] certLengths, int certCount) {
        CertificateFactory certFactory;
        try {
            certFactory = CertificateFactory.getInstance("X509");
        } catch (CertificateException e) {
            return Optional.absent();
        }

        List<X509Certificate> certList = new ArrayList<>();
        for (int i = 0; i < certCount; i++) {
            InputStream inputStream = new ByteArrayInputStream(certBytes[i].getByteArray(0, certLengths[i]));

            X509Certificate cert;
            try {
                cert = (X509Certificate) certFactory.generateCertificate(inputStream);
            } catch (CertificateException | ClassCastException e) {
                return Optional.absent();
            }
            certList.add(cert);
        }
        return Optional.of(certList);
    }

    private static int[] getNativeSizeTArray(Pointer ptr, int count) {
        int[] sizetArray = new int[count];

        switch (Native.SIZE_T_SIZE) {
            case 1:
                byte[] lengthByteArray = ptr.getByteArray(0, count);
                for (int i = 0; i < count; i++) sizetArray[i] = lengthByteArray[i];
                break;
            case 2:
                short[] lengthShortArray = ptr.getShortArray(0, count);
                for (int i = 0; i < count; i++) sizetArray[i] = lengthShortArray[i];
                break;
            case 4:
                int[] lengthIntArray = ptr.getIntArray(0, count);
                for (int i = 0; i < count; i++) sizetArray[i] = lengthIntArray[i];
                break;
            case 8:
                long[] lengthLongArray = ptr.getLongArray(0, count);
                for (int i = 0; i < count; i++) sizetArray[i] = UnsignedInts.checkedCast(lengthLongArray[i]);
                break;
            default:
                throw new IllegalArgumentException("Invalid native size");
        }

        return sizetArray;
    }

    //
    // Client Struct
    //

    public Pointer context;
    public BRCryptoPayProtReqBitPayAndBip70Validator validator;
    public BRCryptoPayProtReqBitPayAndBip70CommonNameExtractor nameExtractor;

    public BRCryptoPayProtReqBitPayAndBip70Callbacks() {
        super();
    }

    public BRCryptoPayProtReqBitPayAndBip70Callbacks(Pointer peer) {
        super(peer);
    }

    public BRCryptoPayProtReqBitPayAndBip70Callbacks(Pointer context,
                                                     BitPayAndBip70Validator validator,
                                                     BitPayAndBip70CommonNameExtractor nameExtractor) {
        super();
        this.context = context;
        this.validator = validator;
        this.nameExtractor = nameExtractor;
    }

    protected List<String> getFieldOrder() {
        return Arrays.asList("context", "validator", "nameExtractor");
    }

    public ByValue toByValue() {
        ByValue other = new ByValue();
        other.context = this.context;
        other.validator = this.validator;
        other.nameExtractor = this.nameExtractor;
        return other;
    }

    public static class ByReference extends BRCryptoPayProtReqBitPayAndBip70Callbacks implements Structure.ByReference {

    }

    public static class ByValue extends BRCryptoPayProtReqBitPayAndBip70Callbacks implements Structure.ByValue {

    }
}
