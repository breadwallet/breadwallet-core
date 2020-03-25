//
//  BRCryptoDebugSupport.swift
//  BRCryptoTests
//
//  Created by Ed Gamble on 8/15/19.
//  Copyright © 2019 Breadwinner AG. All rights reserved.
//
//  See the LICENSE file at the project root for license information.
//  See the CONTRIBUTORS file at the project root for a list of contributors.
//

@testable import BRCrypto

///
/// Event Matching
///
protocol MatchableEvent {
    func match (_ that: Self, strict: Bool) -> Bool
}

extension TransferEvent:  MatchableEvent {
    public func match (_ that: TransferEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created):
            return true
        case (let .changed (oldState1, newState1), let .changed (oldState2, newState2)):
            return !strict || (oldState1 == oldState2 && newState1 == newState2)
        case (.deleted, .deleted):
            return true
        default: return false
        }
    }
}

extension WalletEvent: MatchableEvent {
    public func match (_ that: WalletEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created):
            return true
        case (let .changed (oldState1, newState1), let .changed (oldState2, newState2)):
            return !strict || (oldState1 == oldState2 && newState1 == newState2)
        case (.deleted, .deleted):
            return true

        case (let .transferAdded (transfer1), let .transferAdded(transfer2)):
            return !strict || (transfer1 == transfer2)
        case (let .transferChanged (transfer1), let .transferChanged(transfer2)):
            return !strict || (transfer1 == transfer2)
        case (let .transferDeleted (transfer1), let .transferDeleted(transfer2)):
            return !strict || (transfer1 == transfer2)
        case (let .transferSubmitted (transfer1, success1), let .transferSubmitted(transfer2, success2)):
            return !strict || (transfer1 == transfer2 && success1 == success2)

        case (let .balanceUpdated (amount1), let .balanceUpdated (amount2)):
            return !strict || (amount1 == amount2)
        case (let .feeBasisUpdated (feeBasis1), let .feeBasisUpdated (feeBasis2)):
            return !strict || (feeBasis1 == feeBasis2)
        case (let .feeBasisEstimated (feeBasis1), let .feeBasisEstimated (feeBasis2)):
            return !strict || (feeBasis1 == feeBasis2)

        default: return false
        }
    }
}

extension WalletManagerEvent: MatchableEvent {
    public func match (_ that: WalletManagerEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created): return true
        case (let .changed (oldState1, newState1), let .changed (oldState2, newState2)):
            return !strict || (oldState1 == oldState2 && newState1 == newState2)
        case (let .walletAdded (wallet1), let .walletAdded(wallet2)):
            return !strict || (wallet1 == wallet2)
        case (let .walletChanged (wallet1), let .walletChanged(wallet2)):
            return !strict || (wallet1 == wallet2)
        case (let .walletDeleted (wallet1), let .walletDeleted(wallet2)):
            return !strict || (wallet1 == wallet2)
        case (.syncStarted, .syncStarted): return true
        case (let .syncProgress (ts1, pc1), let .syncProgress (ts2, pc2)):
            return !strict || (ts1 == ts2 && pc1 == pc2)
        case (let .syncRecommended (depth1), let .syncRecommended (depth2)):
            return !strict || (depth1 == depth2)
        case (let .syncEnded (error1), let .syncEnded (error2)):
            return !strict || (error1 == error2)
        case (let .blockUpdated (height1), let .blockUpdated (height2)):
            return !strict || (height1 == height2)
        default: return false
        }
    }
}

extension NetworkEvent:       MatchableEvent {
    func match (_ that: NetworkEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created):
            return true
        case (.feesUpdated, .feesUpdated):
            return true
        default:
            return false
        }
    }
}

extension SystemEvent: MatchableEvent {
    public func match (_ that: SystemEvent, strict: Bool) -> Bool {
        switch (self, that) {
        case (.created, .created):
            return true
        case (let .networkAdded (network1), let .networkAdded (network2)):
            return !strict || (network1 == network2)
        case (let .managerAdded (manager1), let .managerAdded (manager2)):
            return !strict || (manager1 == manager2)
        default:
            return false
        }
    }
}


struct EventMatcher<ME: MatchableEvent> {
    let event: ME

    // When `strict` expect an exact match
    let strict: Bool

    // Given an array of events, if `scan` is true, then this EventMatcher can match any event
    // in the array (scan over events until a match is found)
    let scan: Bool

    // Return `true` is this `EventMatcher` matches `other` using `strict`
    func match (_ other: ME) -> Bool {
        return self.event.match (other, strict: strict)
    }

    init (event: ME, strict: Bool = true, scan: Bool = false) {
        self.event = event
        self.strict = strict
        self.scan = scan
    }

    // if `self` matches in `others` from `index` return the index at the match.  This will
    // use `scan`.
    func match (_ others: [ME], from index: Int) -> Int? {
        if index >= others.count { return nil }         // No event -> no match
        else if match (others[index]) { return index }  // On match -> index
        else if !scan { return nil }                    // No scan -> no match
        else { return match (others, from: 1 + index) } // keep looking
    }

    // Return the first index in `others` where `self` matches
    func match (_ others: [ME]) -> Int? {
        return match(others, from: 0)
    }
}

extension Array where Element:MatchableEvent {
    private func match (selfIndex: Int, _ matchers: [EventMatcher<Element>], matchIndex: Int, matchRequired: Bool) -> Bool {
        if matchIndex >= matchers.count { return !matchRequired  } // No matchers left -> success
        else if selfIndex >= self.count { return false } // Matchers but no elements -> failure
        else if let index = matchers[matchIndex].match (self, from: selfIndex) {
            // Got a match -> next matcher, next index
            return match (selfIndex: 1 + index, matchers, matchIndex: 1 + matchIndex, matchRequired: false)
        }
        else { return false}
    }

    // Return true if every `matcher` has a match in self
    func match (_ matchers: [EventMatcher<Element>]) -> Bool {
        return match (selfIndex: 0, matchers, matchIndex: 0, matchRequired: !matchers.isEmpty)
    }
}
