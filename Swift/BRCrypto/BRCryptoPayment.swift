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

    public var primaryTargetName: String? {
        // TODO(fix): Implement this
        return nil
    }

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
        // TODO(fix): Implement this
//        if self.pkiType != "none" {
//            var certs = [SecCertificate]()
//            let policies = [SecPolicy](repeating: SecPolicyCreateBasicX509(), count: 1)
//            var trust: SecTrust?
//            var trustResult = SecTrustResultType.invalid
//
//            for c in self.certs {
//                if let cert = SecCertificateCreateWithData(nil, Data(c) as CFData) { certs.append(cert) }
//            }
//
//            if !certs.isEmpty {
//                self.cName = SecCertificateCopySubjectSummary(certs[0]) as String?
//            }
//
//            SecTrustCreateWithCertificates(certs as CFTypeRef, policies as CFTypeRef, &trust)
//            if let trust = trust { SecTrustEvaluate(trust, &trustResult) } // verify certificate chain
//
//            // .unspecified indicates a positive result that wasn't decided by the user
//            guard trustResult == .unspecified || trustResult == .proceed else {
//                return certs.isEmpty ? PaymentProtocolError.certificateMissing : PaymentProtocolError.certificateNotTrusted
//            }
//
//            var status = errSecUnimplemented
//            var pubKey: SecKey?
//            if let trust = trust { pubKey = SecTrustCopyPublicKey(trust) }
//
//            if let pubKey = pubKey, let signature = self.signature {
//                if pkiType == "x509+sha256" {
//                    status = SecKeyRawVerify(pubKey, .PKCS1SHA256, self.digest, self.digest.count, signature, signature.count)
//                } else if pkiType == "x509+sha1" {
//                    status = SecKeyRawVerify(pubKey, .PKCS1SHA1, self.digest, self.digest.count, signature, signature.count)
//                }
//            }
//
//            guard status == errSecSuccess else {
//                if status == errSecUnimplemented {
//                    return PaymentProtocolError.signatureTypeUnsupported
//                } else {
//                    return PaymentProtocolError.signatureVerificationFailed
//                }
//            }
//        } else if !self.certs.isEmpty { // non-standard extention to include an un-certified request name
//            self.cName = String(data: Data(self.certs[0]), encoding: .utf8)
//        }
//
//        guard self.details.expires == 0 || NSDate.timeIntervalSinceReferenceDate <= Double(details.expires) else {
//            return PaymentProtocolError.requestExpired
//        }

        return nil
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
