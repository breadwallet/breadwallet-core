//
//  BRBlockChainDB+Support.swift
//  BRCrypto
//
//  Created by Ehsan Rezaie on 2019-04-09.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

import Foundation
import BRCore

// MARK: API Support Types

public enum HTTPMethod: String {
    case get    = "GET"
    case post   = "POST"
    case put    = "PUT"
    case patch  = "PATCH"
    case delete = "DELETE"
}

public typealias ResultCallback<Value> = (Result<Value, Error>) -> Void

public protocol APIRequest: Encodable {
    /// Type of the unwrapped return value
    associatedtype Response: Decodable
    /// Type of the raw response
//    associatedtype RawResponse: Decodable

    var method: HTTPMethod { get }
    var path: String { get }
    var queryItems: [URLQueryItem]? { get }
    var body: Data? { get }
}

extension APIRequest {
    // defaults
//    public typealias RawResponse = Response

    public var method: HTTPMethod { return .get }
    public var body: Data? { return nil }

    // by default all stored properties of the request are treated as query parameters
    public var queryItems: [URLQueryItem]? {
        // encode them as JSON then back to a dictionary to map to URLQueryItem
        guard let data = try? JSONEncoder().encode(self),
            let params = try? JSONSerialization.jsonObject(with: data, options: .allowFragments) as? [String: Any?] else { return nil }
        var queryItems = [URLQueryItem]()
        for (key, value) in params {
            if let value = value { // omit parameters with nil values
                queryItems.append(URLQueryItem(name: key, value: "\(value)"))
            }
        }
        return queryItems
    }
}

/// HAL embedded resource container
public struct EmbeddedContainer<T: Decodable>: Decodable {
    let embedded: T
}

// MARK: Dispatcher

public protocol RequestDispatcher {
    var baseURL: URL { get }

    func endpoint<Request: APIRequest>(for request: Request) -> URL?
    func dispatch<Request: APIRequest>(_ request: Request, completion: ResultCallback<Request.Response>)
}

extension RequestDispatcher {
    public func endpoint<Request: APIRequest>(for request: Request) -> URL? {
        guard var components = URLComponents(url: baseURL, resolvingAgainstBaseURL: true) else {
            return nil
        }

        components.path = components.path.appending(request.path)
        components.queryItems = request.queryItems

        return components.url
    }

    public func decode<Request: APIRequest>(responseFor: Request, from jsonData: Data) -> Result<Request.Response, Error> {
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .formatted(DateFormatter.iso8601Full)
        do {
            if let container = try? decoder.decode(EmbeddedContainer<Request.Response>.self, from: jsonData) {
                return .success(container.embedded)
            } else {
                let response = try decoder.decode(Request.Response.self, from: jsonData)
                return .success(response)
            }
        } catch let error {
            return .failure(error)
        }
    }
}

// MARK: - Typed Identifiers

public protocol Identifiable {
    var id: Identifier<Self> { get }
}

public struct Identifier<Value: Identifiable>: Hashable {
    let string: String
}

extension Identifier: ExpressibleByStringLiteral {
    public init(stringLiteral value: String) {
        string = value
    }
}

extension Identifier: CustomStringConvertible {
    public var description: String {
        return string
    }
}

extension Identifier: Codable {
    public init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        string = try container.decode(String.self)
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        try container.encode(string)
    }
}

// MARK: Date Decoding

extension DateFormatter {
    static let iso8601Full: DateFormatter = {
        let formatter = DateFormatter()
        formatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss.SSSZZZZZ"
        formatter.calendar = Calendar(identifier: .iso8601)
        formatter.timeZone = TimeZone(secondsFromGMT: 0)
        formatter.locale = Locale(identifier: "en_US_POSIX")
        return formatter
    }()
}

// MARK: UInt256 Decoding

extension UInt256: Codable {
    public init(from decoder: Decoder) throws {
        let container = try decoder.singleValueContainer()
        let str = try container.decode(String.self)
        if str.hasPrefix("0x") {
            self.init(hexString: str)
        } else {
            self.init(string: str, radix: 10)
        }
    }

    public func encode(to encoder: Encoder) throws {
        var container = encoder.singleValueContainer()
        try container.encode(hexString)
    }

    public init(hexString: String) {
        self.init(string: hexString.withoutHexPrefix, radix: 16)
    }

    public init(string: String, radix: Int = 10) {
        var status: BRCoreParseStatus = CORE_PARSE_OK
        self = createUInt256Parse(string, Int32(radix), &status)
    }

    public func string(radix: Int) -> String {
        guard let buf = coerceString(self, Int32(radix)) else { return "" }
        let str = String(cString: buf)
        free(buf)
        return str.trimmedLeadingZeros
    }

    public var hexString: String {
        return string(radix: 16).withHexPrefix
    }
}

extension String {
    var withoutHexPrefix: String {
        guard self.hasPrefix("0x") else { return self }
        return String(self.dropFirst(2))
    }

    var withHexPrefix: String {
        guard !self.hasPrefix("0x") else { return self }
        return "0x\(self)"
    }

    var trimmedLeadingZeros: String {
        let trimmed = String(self.drop { $0 == "0" })
        return trimmed.isEmpty ? "0" : trimmed
    }
}
