//
//  BRCryptoBlockChainDBTests.swift
//  BRCryptoTests
//
//  Created by Ehsan Rezaie on 2019-04-08.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//

import XCTest
@testable import BRCrypto

class BRCryptoBlockChainDBTests: XCTestCase {

    let testClient = TestClient()
    var blockchainDB: BlockChainDB!

    override func setUp() {
        blockchainDB = BlockChainDB(dispatcher: testClient)
    }

    override func tearDown() {
        // Put teardown code here. This method is called after the invocation of each test method in the class.
    }

    func testGetBlockchains() {
        let req = BlockChainDB.Request.GetBlockchains()
        testClient.dispatch(req) { result in
            switch result {
            case .success(let blockchains):
                XCTAssert(blockchains.count == 1)
            case .failure(_):
                XCTFail()
            }
        }
    }

}

// Mock JSON payloads decoded into models
struct TestClient: RequestDispatcher {
    var baseURL: URL { return URL(string:"")! }

    func dispatch<Request: APIRequest>(_ request: Request, completion: ResultCallback<Request.Response>) {

        var json: String

        switch request {
        case is BlockChainDB.Request.GetBlockchains:
            json = """
                {
                  "embedded": [
                    {
                      "block_height": 523445,
                      "fee_estimates": [
                        {
                          "estimated_confirmation_in": 360,
                          "fee": {
                            "amount": 100000000,
                            "currency_id": "btc",
                            "links": [
                              {
                                "deprecation": "string",
                                "href": "string",
                                "hreflang": "string",
                                "media": "string",
                                "rel": "string",
                                "templated": true,
                                "title": "string",
                                "type": "string"
                              }
                            ]
                          }
                        }
                      ],
                      "id": "bitcoin-mainnet",
                      "is_mainnet": true,
                      "links": [
                        {
                          "deprecation": "string",
                          "href": "string",
                          "hreflang": "string",
                          "media": "string",
                          "rel": "string",
                          "templated": true,
                          "title": "string",
                          "type": "string"
                        }
                      ],
                      "name": "Bitcoin",
                      "native_currency_id": "btc",
                      "network": "mainnet"
                    }
                  ],
                  "links": [
                    {
                      "deprecation": "string",
                      "href": "string",
                      "hreflang": "string",
                      "media": "string",
                      "rel": "string",
                      "templated": true,
                      "title": "string",
                      "type": "string"
                    }
                  ]
                }
            """
        default:
            return completion(.failure(BlockChainDB.QueryError.RequestFormat))
        }

        guard let data = json.data(using: .utf8) else { return completion(.failure(BlockChainDB.QueryError.Model)) }

        let result = decode(responseFor: request, from: data)
        completion(result)
    }
}

// Mock models
struct MockClient: RequestDispatcher {
    var baseURL: URL { return URL(string:"")! }

    func dispatch<Request: APIRequest>(_ request: Request, completion: ResultCallback<Request.Response>) {
        switch request {
        case let request as BlockChainDB.Request.GetBlockchains:
            var response: [BlockChainDB.Model.Blockchain] = []

            if request.testnet ?? false { // testnet or nil
                response.append(BlockChainDB.Model.Blockchain(id: "bitcoin-testnet",
                                                              name: "Bitcoin",
                                                              currencyId: "btc",
                                                              networkName: "testnet",
                                                              isMainnet: true,
                                                              blockHeight: 900000,
                                                              feeEstimates: []))

                response.append(BlockChainDB.Model.Blockchain(id: "bitcash-testnet",
                                                              name: "Bitcoin Cash",
                                                              currencyId: "bch",
                                                              networkName: "testnet",
                                                              isMainnet: true,
                                                              blockHeight: 1200000,
                                                              feeEstimates: []))

                response.append(BlockChainDB.Model.Blockchain(id: "ethereum-testnet",
                                                              name: "Ethereum",
                                                              currencyId: "eth",
                                                              networkName: "ropsten",
                                                              isMainnet: true,
                                                              blockHeight: 1000000,
                                                              feeEstimates: []))

                response.append(BlockChainDB.Model.Blockchain(id: "ethereum-rinkeby",
                                                              name: "Ethereum",
                                                              currencyId: "eth",
                                                              networkName: "rinkeby",
                                                              isMainnet: true,
                                                              blockHeight: 2000000,
                                                              feeEstimates: []))
            } else  { // mainnet
                response.append(BlockChainDB.Model.Blockchain(id: "bitcoin-mainnet",
                                                              name: "Bitcoin",
                                                              currencyId: "btc",
                                                              networkName: "mainnet",
                                                              isMainnet: true,
                                                              blockHeight: 600000,
                                                              feeEstimates: []))

                response.append(BlockChainDB.Model.Blockchain(id: "bitcash-mainnet",
                                                              name: "Bitcoin Cash",
                                                              currencyId: "bch",
                                                              networkName: "mainnet",
                                                              isMainnet: true,
                                                              blockHeight: 1000000,
                                                              feeEstimates: []))

                response.append(BlockChainDB.Model.Blockchain(id: "ethereum-mainnet",
                                                              name: "Ethereum",
                                                              currencyId: "eth",
                                                              networkName: "mainnet",
                                                              isMainnet: true,
                                                              blockHeight: 8000000,
                                                              feeEstimates: []))
            }

            guard let value = response as? Request.Response else { return completion(.failure(BlockChainDB.QueryError.Model)) }
            completion(.success(value))

        default:
            completion(.failure(BlockChainDB.QueryError.RequestFormat))
        }
    }
}
