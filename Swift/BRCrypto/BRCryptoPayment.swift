//
//  BRCrypto.swift
//  BRCrypto
//
//  Created by Michael Carrara on 8/27/19.
//  Copyright © 2018 Breadwallet AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//
import Foundation
import BRCryptoC

public enum PaymentProtocolError: Error {
    case certificateMissing
    case certificateNotTrusted
    case signatureTypeUnsupported
    case signatureVerificationFailed
    case requestExpired

    internal init? (_ core: BRCryptoPaymentProtocolError) {
        switch core {
        case CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE:                          return nil
        case CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_MISSING:                  self = .certificateMissing
        case CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED:              self = .certificateNotTrusted
        case CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED:  self = .signatureTypeUnsupported
        case CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED: self = .signatureVerificationFailed
        case CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED:                       self = .requestExpired
        default: self = .signatureVerificationFailed; precondition(false)
        }
    }
}

public enum PaymentProtocolRequestType {
    case bip70
    case bitPay

    fileprivate init (_ core: BRCryptoPaymentProtocolType) {
        switch core {
        case CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70:  self = .bip70
        case CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY: self = .bitPay
        default: self = .bip70; precondition(false)
        }
    }
}

public final class PaymentProtocolRequest {

    public static func create(wallet: Wallet,
                              forBitPay json: Data) -> PaymentProtocolRequest?  {
        guard CRYPTO_TRUE == cryptoPaymentProtocolRequestValidateSupported (CRYPTO_PAYMENT_PROTOCOL_TYPE_BITPAY,
                                                                            wallet.manager.network.core,
                                                                            wallet.currency.core,
                                                                            wallet.core) else { return nil }

        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSZZZZZ"

        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .formatted(formatter)

        guard let req = try? decoder.decode(BitPayRequest.self, from: json) else { return nil }

        let builder = cryptoPaymentProtocolRequestBitPayBuilderCreate (wallet.manager.network.core,
                                                                       wallet.currency.core,
                                                                       req.network,
                                                                       UInt64(req.time.timeIntervalSince1970),
                                                                       UInt64(req.expires.timeIntervalSince1970),
                                                                       req.requiredFeeRate,
                                                                       req.memo,
                                                                       req.paymentUrl.absoluteString,
                                                                       nil,
                                                                       0)
        for output in req.outputs {
            cryptoPaymentProtocolRequestBitPayBuilderAddOutput (builder,
                                                                output.address,
                                                                output.amount)
        }

        return cryptoPaymentProtocolRequestBitPayBuilderBuild (builder)
            .map { PaymentProtocolRequest(core: $0,
                                          wallet: wallet)
        }
    }

    public static func create(wallet: Wallet,
                              forBip70 serialization: Data) -> PaymentProtocolRequest? {
        guard CRYPTO_TRUE == cryptoPaymentProtocolRequestValidateSupported (CRYPTO_PAYMENT_PROTOCOL_TYPE_BIP70,
                                                                            wallet.manager.network.core,
                                                                            wallet.currency.core,
                                                                            wallet.core) else { return nil }

        var bytes = [UInt8](serialization)
        return cryptoPaymentProtocolRequestCreateForBip70 (wallet.manager.network.core,
                                                           wallet.currency.core,
                                                           &bytes,
                                                           bytes.count)
            .map { PaymentProtocolRequest(core: $0,
                                          wallet: wallet)
        }
    }

    private static let nameExtractor: BRCryptoPayProtReqBitPayBip70NameExtractor = {(req, pkiType, certBytes, certLengths, certsSz) in
        var name: String? = nil

        var certArray = [Data]()
        for index in 0..<certsSz {
            certArray.append(Data (bytes: certBytes![index]!, count: certLengths![index]))
        }

        let pkiType = asUTF8String (pkiType!)
        if pkiType != "none" {


            for c in certArray {
                if let cert = SecCertificateCreateWithData(nil, c as CFData) {
                    name = SecCertificateCopySubjectSummary(cert) as String?
                    break
                }
            }
        } else if 0 != certsSz { // non-standard extention to include an un-certified request name
            name = String(data: certArray[0], encoding: .utf8)
        }

        return name.map { strdup ($0) }
    }

    private static let validator: BRCryptoPayProtReqBitPayBip70Validator = {(req, pkiType, expires, certBytes, certLengths, certsSz, digest, digestLen, signature, signatureLen) in
        let pkiType = asUTF8String (pkiType!)
        if pkiType != "none" {
            var certs = [SecCertificate]()
            let policies = [SecPolicy](repeating: SecPolicyCreateBasicX509(), count: 1)
            var trust: SecTrust?
            var trustResult = SecTrustResultType.invalid

            var certArray = [Data]()
            for index in 0..<certsSz {
                certArray.append(Data (bytes: certBytes![index]!, count: certLengths![index]))
            }

            for c in certArray {
                if let cert = SecCertificateCreateWithData(nil, c as CFData) { certs.append(cert) }
            }

            SecTrustCreateWithCertificates(certs as CFTypeRef, policies as CFTypeRef, &trust)
            if let trust = trust { SecTrustEvaluate(trust, &trustResult) } // verify certificate chain

            // .unspecified indicates a positive result that wasn't decided by the user
            guard trustResult == .unspecified || trustResult == .proceed else {
                return certs.isEmpty ? CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_MISSING : CRYPTO_PAYMENT_PROTOCOL_ERROR_CERT_NOT_TRUSTED
            }

            var status = errSecUnimplemented
            var pubKey: SecKey?
            if let trust = trust { pubKey = SecTrustCopyPublicKey(trust) }

            if let pubKey = pubKey, let signature = signature {
                if pkiType == "x509+sha256" {
                    status = SecKeyRawVerify(pubKey, .PKCS1SHA256, digest!, digestLen, signature, signatureLen)
                } else if pkiType == "x509+sha1" {
                    status = SecKeyRawVerify(pubKey, .PKCS1SHA1, digest!, digestLen, signature, signatureLen)
                }
            }

            guard status == errSecSuccess else {
                if status == errSecUnimplemented {
                    return CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_TYPE_NOT_SUPPORTED
                } else {
                    return CRYPTO_PAYMENT_PROTOCOL_ERROR_SIGNATURE_VERIFICATION_FAILED
                }
            }
        }

        guard expires == 0 || NSDate.timeIntervalSinceReferenceDate <= Double(expires) else {
            return CRYPTO_PAYMENT_PROTOCOL_ERROR_EXPIRED
        }

        return CRYPTO_PAYMENT_PROTOCOL_ERROR_NONE
    }

    public var type: PaymentProtocolRequestType {
        return PaymentProtocolRequestType(cryptoPaymentProtocolRequestGetType (core))
    }

    public var isSecure: Bool {
        return CRYPTO_TRUE == cryptoPaymentProtocolRequestIsSecure (core)
    }

    public var memo: String? {
        return cryptoPaymentProtocolRequestGetMemo (core)
            .map { asUTF8String($0) }
    }

    public var paymentURL: String? {
        return cryptoPaymentProtocolRequestGetPaymentURL (core)
            .map { asUTF8String($0) }
    }

    public var totalAmount: Amount? {
        return cryptoPaymentProtocolRequestGetTotalAmount (core)
            .map { Amount (core: $0, take: false)  }
    }

    public var primaryTargetAddress: Address? {
        return cryptoPaymentProtocolRequestGetPrimaryTargetAddress (core)
            .map { Address (core: $0, take: false)  }
    }

    public private(set) lazy var primaryTargetName: String? = {
        switch type {
        case .bitPay:
            return cryptoPaymentProtocolRequestGetPrimaryTargetNameBitPay(core, PaymentProtocolRequest.nameExtractor)
                .map { asUTF8String ($0, true) }
        case .bip70:
            return cryptoPaymentProtocolRequestGetPrimaryTargetNameBip70(core, PaymentProtocolRequest.nameExtractor)
                .map { asUTF8String ($0, true) }
        }
    }()

    private lazy var error: PaymentProtocolError? = {
        switch type {
        case .bitPay:
            return PaymentProtocolError(cryptoPaymentProtocolRequestIsValidBitPay(core, PaymentProtocolRequest.validator))
        case .bip70:
            return PaymentProtocolError(cryptoPaymentProtocolRequestIsValidBip70(core, PaymentProtocolRequest.validator))
        }
    }()

    public var requiredNetworkFee: NetworkFee? {
        return cryptoPaymentProtocolRequestGetRequiredNetworkFee (core)
            .map { NetworkFee (core: $0, take: false) }
    }

    internal let core: BRCryptoPaymentProtocolRequest
    private let manager: WalletManager
    private let wallet: Wallet

    private init(core: BRCryptoPaymentProtocolRequest,
                 wallet: Wallet) {
        self.core = core
        self.manager = wallet.manager
        self.wallet = wallet
    }

    public func isValid() -> PaymentProtocolError? {
        return self.error
    }

    public func estimateFee(fee: NetworkFee,
                            completion: @escaping (Result<TransferFeeBasis, Wallet.FeeEstimationError>) -> Void) {
        wallet.estimateFee(request: self, fee: fee, completion: completion)
    }

    public func createTransfer(estimatedFeeBasis: TransferFeeBasis) -> Transfer? {
        return wallet.createTransfer(request: self, estimatedFeeBasis: estimatedFeeBasis)
    }

    public func signTransfer(transfer: Transfer, paperKey: String) -> Bool {
        return manager.sign(transfer: transfer, paperKey: paperKey)
    }

    public func submitTransfer(transfer: Transfer) {
        manager.submit(transfer: transfer)
    }

    public func createPayment(transfer: Transfer) -> PaymentProtocolPayment? {
        return PaymentProtocolPayment.create(request: self,
                                             transfer: transfer,
                                             refund: wallet.target)
    }

    deinit {
        cryptoPaymentProtocolRequestGive (core)
    }
}

public final class PaymentProtocolPayment {

    fileprivate static func create(request: PaymentProtocolRequest,
                                   transfer: Transfer,
                                   refund: Address) -> PaymentProtocolPayment? {
        return cryptoPaymentProtocolPaymentCreate (request.core,
                                                   transfer.core,
                                                   refund.core)
            .map { PaymentProtocolPayment (core: $0) }
    }

    private let core: BRCryptoPaymentProtocolPayment

    private init (core: BRCryptoPaymentProtocolPayment) {
        self.core = core
    }

    public func encode() -> Data? {
        var bytesCount: Int = 0
        return cryptoPaymentProtocolPaymentEncode (core, &bytesCount)
            .map { Data (bytes: $0, count: bytesCount) }
    }
}

public final class PaymentProtocolPaymentACK {

    public static func create(forBitPay json: Data) -> PaymentProtocolPaymentACK? {
        guard let ack = try? JSONDecoder().decode(BitPayAck.self, from: json) else { return nil }
        return PaymentProtocolPaymentACK (bitPayAck: ack)
    }

    public static func create(forBip70 serialization: Data) -> PaymentProtocolPaymentACK? {
        var bytes = [UInt8](serialization)
        return cryptoPaymentProtocolPaymentACKCreateForBip70 (&bytes, bytes.count)
            .map { PaymentProtocolPaymentACK (core: $0) }
    }

    private let impl: Impl

    private init(core: BRCryptoPaymentProtocolPaymentACK){
        self.impl = .core(core)
    }

    private init(bitPayAck: BitPayAck){
        self.impl = .bitPay(bitPayAck)
    }

    public var memo: String? {
        return impl.memo
    }

    private enum Impl {
        case core (BRCryptoPaymentProtocolPaymentACK)
        case bitPay (BitPayAck)

        fileprivate var memo: String? {
            switch self {
            case .core (let core):
                return cryptoPaymentProtocolPaymentACKGetMemo (core)
                    .map { asUTF8String ($0, true) }
            case .bitPay (let ack):
                return ack.memo
            }
        }
    }
}

fileprivate struct BitPayRequest: Decodable {
    fileprivate struct Output: Decodable {
        let amount: UInt64
        let address: String
    }

    let network: String
    let currency: String
    let requiredFeeRate: Double
    let outputs: [Output]
    let time: Date
    let expires: Date
    let memo: String
    let paymentUrl: URL
    let paymentId: String
}

fileprivate struct BitPayPayment: Decodable {
    let currency: String
    let transactions: [String]
}

fileprivate struct BitPayAck: Decodable {
    let payment: BitPayPayment
    let memo: String?
}
