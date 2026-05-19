/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

const UNIPOKER_59864: number = 59864;
const UNIPOKER_120476: number = 120476;
const UNIPOKER_101226: number = 101226;
const UNIPOKER_11359: number = 11359;
const UNIPOKER_5083: number = 5083;
const UNIPOKER_982: number = 982;
const UNIPOKER_456: number = 456;
const UNIPOKER_370: number = 370;
const UNIPOKER_45: number = 45;
const UNIPOKER_3: number = 3;

const UNIPOKER_60020: number = 60020;
const UNIPOKER_120166: number = 120166;
const UNIPOKER_101440: number = 101440;
const UNIPOKER_11452: number = 11452;
const UNIPOKER_5096: number = 5096;
const UNIPOKER_942: number = 942;
const UNIPOKER_496: number = 496;
const UNIPOKER_333: number = 333;
const UNIPOKER_67: number = 67;
const UNIPOKER_8: number = 8;

const UNIPOKER_60065: number = 60065;
const UNIPOKER_120262: number = 120262;
const UNIPOKER_101345: number = 101345;
const UNIPOKER_11473: number = 11473;
const UNIPOKER_5093: number = 5093;
const UNIPOKER_941: number = 941;
const UNIPOKER_472: number = 472;
const UNIPOKER_335: number = 335;
const UNIPOKER_76: number = 76;

const UNIPOKER_60064: number = 60064;
const UNIPOKER_120463: number = 120463;
const UNIPOKER_101218: number = 101218;
const UNIPOKER_11445: number = 11445;
const UNIPOKER_5065: number = 5065;
const UNIPOKER_938: number = 938;
const UNIPOKER_446: number = 446;
const UNIPOKER_364: number = 364;
const UNIPOKER_58: number = 58;

const UNIPOKER_0XF: number = 0xf;
const UNIPOKER_2000: number = 2000;
const UNIPOKER_1000: number = 1000;
const UNIPOKER_120: number = 20;
const UNIPOKER_20: number = 20;
const UNIPOKER_16: number = 16;
const UNIPOKER_12: number = 12;
const UNIPOKER_10: number = 10;
const UNIPOKER_6: number = 6;
const UNIPOKER_5: number = 5;
const UNIPOKER_4: number = 4;
const UNIPOKER_2: number = 2;

const UNIPOKER_0X900000: number = 0x900000;
const UNIPOKER_0X800000: number = 0x800000;
const UNIPOKER_0X700000: number = 0x700000;
const UNIPOKER_0X600000: number = 0x600000;
const UNIPOKER_0X500000: number = 0x500000;
const UNIPOKER_0X400000: number = 0x400000;
const UNIPOKER_0X300000: number = 0x300000;
const UNIPOKER_0X200000: number = 0x200000;
const UNIPOKER_0X100000: number = 0x100000;

('use strict');
declare interface ArkTools {
  timeInUs(args: number): number;
}
/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  players: Player[] = [];
  /*
   * @State
   */
  constructor() {
    this.players.push(new Player('Player 1'));
    this.players.push(new Player('Player 2'));
    this.players.push(new Player('Player 3'));
    this.players.push(new Player('Player 4'));
  }
  /*
   * @Benchmark
   */
  runIteration(): void {
    playHands(this.players);
  }

  validate(): void {
    let isInBrowser = true;
    if (this.players.length !== playerExpectations.length) {
      throw new Error('Expect ' + playerExpectations.length + ', but actually have ' + this.players.length);
    }

    if (isInBrowser) {
      for (let playerIdx = 0; playerIdx < playerExpectations.length; playerIdx++) {
        playerExpectations[playerIdx].validate(this.players[playerIdx]);
      }
    }
  }
}

class PlayerExpectation {
  private wins: number;
  private handTypeCounts: number[];
  /*
   * @State
   */
  constructor(wins: number, handTypeCounts: number[]) {
    this.wins = wins;
    this.handTypeCounts = handTypeCounts;
  }

  validate(player: Player): void {
    if (player.wins !== this.wins) {
      throw new Error('Expected ' + player.name + ' to have ' + this.wins + ', but they have ' + player.wins);
    }

    let actualHandTypeCounts = player.handTypeCounts;
    if (player.handTypeCounts.length !== actualHandTypeCounts.length) {
      throw new Error('Expected ' + player.name + ' to have ' + this.handTypeCounts.length + ' hand types, but they have ' + actualHandTypeCounts.length);
    }

    for (let handTypeIdx = 0; handTypeIdx < this.handTypeCounts.length; handTypeIdx++) {
      if (this.handTypeCounts[handTypeIdx] !== actualHandTypeCounts[handTypeIdx]) {
        throw new Error(
          'Expected ' +
            player.name +
            ' to have ' +
            this.handTypeCounts[handTypeIdx] +
            ' ' +
            PlayerExpectation.handTypes[handTypeIdx] +
            ' hands, but they have ' +
            actualHandTypeCounts[handTypeIdx]
        );
      }
    }
  }

  private static handTypes: string[] = [
    'High Cards',
    'Pairs',
    'Two Pairs',
    'Three of a Kinds',
    'Straights',
    'Flushes',
    'Full Houses',
    'Four of a Kinds',
    'Straight Flushes',
    'Royal Flushes'
  ];
}

let playerExpectations: PlayerExpectation[] = [];
playerExpectations.push(
  new PlayerExpectation(UNIPOKER_59864, [
    UNIPOKER_120476,
    UNIPOKER_101226,
    UNIPOKER_11359,
    UNIPOKER_5083,
    UNIPOKER_982,
    UNIPOKER_456,
    UNIPOKER_370,
    UNIPOKER_45,
    UNIPOKER_3,
    0
  ])
);
playerExpectations.push(
  new PlayerExpectation(UNIPOKER_60020, [
    UNIPOKER_120166,
    UNIPOKER_101440,
    UNIPOKER_11452,
    UNIPOKER_5096,
    UNIPOKER_942,
    UNIPOKER_496,
    UNIPOKER_333,
    UNIPOKER_67,
    UNIPOKER_8,
    0
  ])
);
playerExpectations.push(
  new PlayerExpectation(UNIPOKER_60065, [
    UNIPOKER_120262,
    UNIPOKER_101345,
    UNIPOKER_11473,
    UNIPOKER_5093,
    UNIPOKER_941,
    UNIPOKER_472,
    UNIPOKER_335,
    UNIPOKER_76,
    UNIPOKER_3,
    0
  ])
);
playerExpectations.push(
  new PlayerExpectation(UNIPOKER_60064, [
    UNIPOKER_120463,
    UNIPOKER_101218,
    UNIPOKER_11445,
    UNIPOKER_5065,
    UNIPOKER_938,
    UNIPOKER_446,
    UNIPOKER_364,
    UNIPOKER_58,
    UNIPOKER_3,
    0
  ])
);

class CardDeck {
  private cards: string[] = [];
  /*
   * @State
   */
  constructor() {
    this.newDeck();
  }

  newDeck(): void {
    // Make a shallow copy of a new deck
    this.cards = CardDeck.newDeck.slice(0);
  }

  shuffle(): void {
    this.newDeck();

    for (let index = 52; index !== 0; ) {
      // Select a random card
      let randomIndex = Math.floor(Math.random() * index);
      index -= 1;

      let tempCard = this.cards[index];
      this.cards[index] = this.cards[randomIndex];
      this.cards[randomIndex] = tempCard;
    }
  }
  // dealOneCard
  dealOneCard(): string | undefined {
    return this.cards.shift();
  }

  static cardRank(card: string): number {
    // This returns a numeric value for a card.
    // Ace is highest.

    let rankOfCard = card.codePointAt(0)! & UNIPOKER_0XF;
    if (rankOfCard === 0x1) {
      // Make Aces higher than Kings
      return UNIPOKER_0XF;
    }
    return rankOfCard;
  }

  static cardName(card: string): string {
    if (typeof card === 'string') {
      let result = card.codePointAt(0)!;
      return CardDeck.rankNames[result & UNIPOKER_0XF];
    }
    return '';
  }

  private static rankNames = ['', 'Ace', '2', '3', '4', '5', '6', '7', '8', '9', '10', 'Jack', '', 'Queen', 'King'];
  private static newDeck = [
    // Spades
    '\u{1f0a1}',
    '\u{1f0a2}',
    '\u{1f0a3}',
    '\u{1f0a4}',
    '\u{1f0a5}',
    '\u{1f0a6}',
    '\u{1f0a7}',
    '\u{1f0a8}',
    '\u{1f0a9}',
    '\u{1f0aa}',
    '\u{1f0ab}',
    '\u{1f0ad}',
    '\u{1f0ae}',
    // Hearts
    '\u{1f0b1}',
    '\u{1f0b2}',
    '\u{1f0b3}',
    '\u{1f0b4}',
    '\u{1f0b5}',
    '\u{1f0b6}',
    '\u{1f0b7}',
    '\u{1f0b8}',
    '\u{1f0b9}',
    '\u{1f0ba}',
    '\u{1f0bb}',
    '\u{1f0bd}',
    '\u{1f0be}',
    // Clubs
    '\u{1f0d1}',
    '\u{1f0d2}',
    '\u{1f0d3}',
    '\u{1f0d4}',
    '\u{1f0d5}',
    '\u{1f0d6}',
    '\u{1f0d7}',
    '\u{1f0d8}',
    '\u{1f0d9}',
    '\u{1f0da}',
    '\u{1f0db}',
    '\u{1f0dd}',
    '\u{1f0de}',
    // Diamondss
    '\u{1f0c1}',
    '\u{1f0c2}',
    '\u{1f0c3}',
    '\u{1f0c4}',
    '\u{1f0c5}',
    '\u{1f0c6}',
    '\u{1f0c7}',
    '\u{1f0c8}',
    '\u{1f0c9}',
    '\u{1f0ca}',
    '\u{1f0cb}',
    '\u{1f0cd}',
    '\u{1f0ce}'
  ];
}
/*
 * @Generator
 */
class Hand {
  private mRank: number = 0;
  private cards: string[] = [];
  /*
   * @State
   */
  constructor() {
    this.clear();
  }
  clear(): void {
    this.cards = [];
    this.mRank = 0;
  }

  takeCard(card: string): void {
    this.cards.push(card);
  }
  remainingCard(secondOfAKindResult: RegExpMatchArray, firstOfAKind: string, handString: string): void {
    let secondOfAKind = secondOfAKindResult[0];
    // Three of a Kinds + Pairs, or Pairs + Three of a Kinds
    if (
      (firstOfAKind.length === UNIPOKER_6 && secondOfAKind.length === UNIPOKER_4) ||
      (firstOfAKind.length === UNIPOKER_4 && secondOfAKind.length === UNIPOKER_6)
    ) {
      let threeOfAKindCardRank = 0;
      let twoOfAKindCardRank = 0;

      if (firstOfAKind.length === UNIPOKER_6) {
        threeOfAKindCardRank = CardDeck.cardRank(firstOfAKind.slice(0, UNIPOKER_2));
        twoOfAKindCardRank = CardDeck.cardRank(secondOfAKind.slice(0, UNIPOKER_2));
      } else {
        threeOfAKindCardRank = CardDeck.cardRank(secondOfAKind.slice(0, UNIPOKER_2));
        twoOfAKindCardRank = CardDeck.cardRank(firstOfAKind.slice(0, UNIPOKER_2));
      }
      this.mRank =
        Hand.fullHouse |
        (threeOfAKindCardRank << UNIPOKER_16) |
        (threeOfAKindCardRank << UNIPOKER_12) |
        (threeOfAKindCardRank << UNIPOKER_8) |
        (twoOfAKindCardRank << UNIPOKER_4) |
        twoOfAKindCardRank;
    } else if (firstOfAKind.length === UNIPOKER_4 && secondOfAKind.length === UNIPOKER_4) {
      let firstPairCardRank = CardDeck.cardRank(firstOfAKind.slice(0, UNIPOKER_2));
      let secondPairCardRank = CardDeck.cardRank(secondOfAKind.slice(0, UNIPOKER_2));
      let otherCardRank = 0;
      // Due to sorting, the other card is at index 0, 4 or 8
      if (firstOfAKind.codePointAt(0) === handString.codePointAt(0)) {
        if (secondOfAKind.codePointAt(0) === handString.codePointAt(UNIPOKER_4)) {
          otherCardRank = CardDeck.cardRank(handString.slice(UNIPOKER_8, UNIPOKER_10));
        } else {
          otherCardRank = CardDeck.cardRank(handString.slice(UNIPOKER_4, UNIPOKER_6));
        }
      } else {
        otherCardRank = CardDeck.cardRank(handString.slice(0, UNIPOKER_2));
      }
      this.mRank =
        Hand.twoPair |
        (firstPairCardRank << UNIPOKER_16) |
        (firstPairCardRank << UNIPOKER_12) |
        (secondPairCardRank << UNIPOKER_8) |
        (secondPairCardRank << UNIPOKER_4) |
        otherCardRank;
    }
  }
  // When comparing lengths, a matched unicode character has a length of 2.
  // Therefore expected lengths are doubled, e.g a pair will have a match length of 4.
  comparing(ofAKindResult: RegExpMatchArray, handString: string): void {
    if (ofAKindResult[0].length === UNIPOKER_8) {
      this.mRank = Hand.fourOfAKind | CardDeck.cardRank(this.cards[0]);
    } else {
      // Found pair or three of a kind.  Check for two pair or full house.
      let firstOfAKind = ofAKindResult[0];
      let remainingCardsIndex = handString.indexOf(firstOfAKind) + firstOfAKind.length;
      let secondOfAKindResult = handString.slice(remainingCardsIndex).match(Hand.ofAKindRegExp);
      if (remainingCardsIndex <= UNIPOKER_6 && secondOfAKindResult) {
        this.remainingCard(secondOfAKindResult, firstOfAKind, handString);
      } else {
        let ofAKindCardRank = CardDeck.cardRank(firstOfAKind.slice(0, UNIPOKER_2));
        let otherCardsRank = 0;
        for (let card of this.cards) {
          let cardRank = CardDeck.cardRank(card);
          if (cardRank !== ofAKindCardRank) {
            otherCardsRank = (otherCardsRank << UNIPOKER_4) | cardRank;
          }
        }
        if (firstOfAKind.length === UNIPOKER_6) {
          this.mRank =
            Hand.threeOfAKind | (ofAKindCardRank << UNIPOKER_16) | (ofAKindCardRank << UNIPOKER_12) | (ofAKindCardRank << UNIPOKER_8) | otherCardsRank;
        } else {
          this.mRank = Hand.pair | (ofAKindCardRank << UNIPOKER_16) | (ofAKindCardRank << UNIPOKER_12) | otherCardsRank;
        }
      }
    }
  }
  score(): void {
    // Sort highest rank to lowest
    this.cards.sort((a, b): number => {
      return CardDeck.cardRank(b) - CardDeck.cardRank(a);
    });

    let handString = this.cards.join('');

    let flushResult = handString.match(Hand.flushRegExp);
    let straightResult = handString.match(Hand.straightRegExp);
    let ofAKindResult = handString.match(Hand.ofAKindRegExp);

    if (flushResult !== null && flushResult !== undefined) {
      if (straightResult !== null && straightResult !== undefined) {
        if (straightResult[1] !== null && straightResult[1] !== undefined) {
          this.mRank = Hand.royalFlush;
        } else {
          this.mRank = Hand.straightFlush;
        }
      } else {
        this.mRank = Hand.flush;
      }
      this.mRank |= (CardDeck.cardRank(this.cards[0]) << UNIPOKER_16) | (CardDeck.cardRank(this.cards[1]) << UNIPOKER_12);
    } else if (straightResult !== null && straightResult !== undefined) {
      this.mRank = Hand.straight | (CardDeck.cardRank(this.cards[0]) << UNIPOKER_16) | (CardDeck.cardRank(this.cards[1]) << UNIPOKER_12);
    } else if (ofAKindResult !== null && ofAKindResult !== undefined) {
      this.comparing(ofAKindResult, handString);
    } else {
      this.mRank = 0;
      for (let card of this.cards) {
        let cardRank = CardDeck.cardRank(card);
        this.mRank = (this.mRank << UNIPOKER_4) | cardRank;
      }
    }
  }
  get getRank(): number {
    return this.mRank;
  }

  toString(): string {
    return this.cards.join('');
  }

  private static flushRegExp: RegExp = new RegExp(
    '([\u{1f0a1}-\u{1f0ae}]{5})|([\u{1f0b1}-\u{1f0be}]{5})|([\u{1f0c1}-\u{1f0ce}]{5})|([\u{1f0d1}-\u{1f0de}]{5})',
    'u'
  );
  private static straightRegExp: RegExp = new RegExp(
    '([\u{1f0a1}\u{1f0b1}\u{1f0d1}\u{1f0c1}][\u{1f0ae}\u{1f0be}\u{1f0de}\u{1f0ce}][\u{1f0ad}\u{1f0bd}\u{1f0dd}\u{1f0cd}][\u{1f0ab}\u{1f0bb}\u{1f0db}\u{1f0cb}][\u{1f0aa}\u{1f0ba}\u{1f0da}\u{1f0ca}])|[\u{1f0ae}\u{1f0be}\u{1f0de}\u{1f0ce}][\u{1f0ad}\u{1f0bd}\u{1f0dd}\u{1f0cd}][\u{1f0ab}\u{1f0bb}\u{1f0db}\u{1f0cb}][\u{1f0aa}\u{1f0ba}\u{1f0da}\u{1f0ca}][\u{1f0a9}\u{1f0b9}\u{1f0d9}\u{1f0c9}]|[\u{1f0ad}\u{1f0bd}\u{1f0dd}\u{1f0cd}][\u{1f0ab}\u{1f0bb}\u{1f0db}\u{1f0cb}][\u{1f0aa}\u{1f0ba}\u{1f0da}\u{1f0ca}][\u{1f0a9}\u{1f0b9}\u{1f0d9}\u{1f0c9}][\u{1f0a8}\u{1f0b8}\u{1f0d8}\u{1f0c8}]|[\u{1f0ab}\u{1f0bb}\u{1f0db}\u{1f0cb}][\u{1f0aa}\u{1f0ba}\u{1f0da}\u{1f0ca}][\u{1f0a9}\u{1f0b9}\u{1f0d9}\u{1f0c9}][\u{1f0a8}\u{1f0b8}\u{1f0d8}\u{1f0c8}][\u{1f0a7}\u{1f0b7}\u{1f0d7}\u{1f0c7}]|[\u{1f0aa}\u{1f0ba}\u{1f0da}\u{1f0ca}][\u{1f0a9}\u{1f0b9}\u{1f0d9}\u{1f0c9}][\u{1f0a8}\u{1f0b8}\u{1f0d8}\u{1f0c8}][\u{1f0a7}\u{1f0b7}\u{1f0d7}\u{1f0c7}][\u{1f0a6}\u{1f0b6}\u{1f0d6}\u{1f0c6}]|[\u{1f0a9}\u{1f0b9}\u{1f0d9}\u{1f0c9}][\u{1f0a8}\u{1f0b8}\u{1f0d8}\u{1f0c8}][\u{1f0a7}\u{1f0b7}\u{1f0d7}\u{1f0c7}][\u{1f0a6}\u{1f0b6}\u{1f0d6}\u{1f0c6}][\u{1f0a5}\u{1f0b5}\u{1f0d5}\u{1f0c5}]|[\u{1f0a8}\u{1f0b8}\u{1f0d8}\u{1f0c8}][\u{1f0a7}\u{1f0b7}\u{1f0d7}\u{1f0c7}][\u{1f0a6}\u{1f0b6}\u{1f0d6}\u{1f0c6}][\u{1f0a5}\u{1f0b5}\u{1f0d5}\u{1f0c5}][\u{1f0a4}\u{1f0b4}\u{1f0d4}\u{1f0c4}]|[\u{1f0a7}\u{1f0b7}\u{1f0d7}\u{1f0c7}][\u{1f0a6}\u{1f0b6}\u{1f0d6}\u{1f0c6}][\u{1f0a5}\u{1f0b5}\u{1f0d5}\u{1f0c5}][\u{1f0a4}\u{1f0b4}\u{1f0d4}\u{1f0c4}][\u{1f0a3}\u{1f0b3}\u{1f0d3}\u{1f0c3}]|[\u{1f0a6}\u{1f0b6}\u{1f0d6}\u{1f0c6}][\u{1f0a5}\u{1f0b5}\u{1f0d5}\u{1f0c5}][\u{1f0a4}\u{1f0b4}\u{1f0d4}\u{1f0c4}][\u{1f0a3}\u{1f0b3}\u{1f0d3}\u{1f0c3}][\u{1f0a2}\u{1f0b2}\u{1f0d2}\u{1f0c2}]|[\u{1f0a1}\u{1f0b1}\u{1f0d1}\u{1f0c1}][\u{1f0a5}\u{1f0b5}\u{1f0d5}\u{1f0c5}][\u{1f0a4}\u{1f0b4}\u{1f0d4}\u{1f0c4}][\u{1f0a3}\u{1f0b3}\u{1f0d3}\u{1f0c3}][\u{1f0a2}\u{1f0b2}\u{1f0d2}\u{1f0c2}]',
    'u'
  );
  private static ofAKindRegExp: RegExp = new RegExp(
    '(?:[\u{1f0a1}\u{1f0b1}\u{1f0d1}\u{1f0c1}]{2,4})|(?:[\u{1f0ae}\u{1f0be}\u{1f0de}\u{1f0ce}]{2,4})|(?:[\u{1f0ad}\u{1f0bd}\u{1f0dd}\u{1f0cd}]{2,4})|(?:[\u{1f0ab}\u{1f0bb}\u{1f0db}\u{1f0cb}]{2,4})|(?:[\u{1f0aa}\u{1f0ba}\u{1f0da}\u{1f0ca}]{2,4})|(?:[\u{1f0a9}\u{1f0b9}\u{1f0d9}\u{1f0c9}]{2,4})|(?:[\u{1f0a8}\u{1f0b8}\u{1f0d8}\u{1f0c8}]{2,4})|(?:[\u{1f0a7}\u{1f0b7}\u{1f0d7}\u{1f0c7}]{2,4})|(?:[\u{1f0a6}\u{1f0b6}\u{1f0d6}\u{1f0c6}]{2,4})|(?:[\u{1f0a5}\u{1f0b5}\u{1f0d5}\u{1f0c5}]{2,4})|(?:[\u{1f0a4}\u{1f0b4}\u{1f0d4}\u{1f0c4}]{2,4})|(?:[\u{1f0a3}\u{1f0b3}\u{1f0d3}\u{1f0c3}]{2,4})|(?:[\u{1f0a2}\u{1f0b2}\u{1f0d2}\u{1f0c2}]{2,4})',
    'u'
  );

  private static royalFlush: number = UNIPOKER_0X900000;
  private static straightFlush: number = UNIPOKER_0X800000;
  private static fourOfAKind: number = UNIPOKER_0X700000;
  private static fullHouse: number = UNIPOKER_0X600000;
  private static flush: number = UNIPOKER_0X500000;
  private static straight: number = UNIPOKER_0X400000;
  private static threeOfAKind: number = UNIPOKER_0X300000;
  private static twoPair: number = UNIPOKER_0X200000;
  private static pair: number = UNIPOKER_0X100000;
}

class Player extends Hand {
  private mName: string;
  private mWins: number;
  private mHandTypeCounts: number[];
  /*
   * @State
   */
  constructor(name: string) {
    super();
    this.mWins = 0;
    this.mName = name;
    this.mHandTypeCounts = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
  }
  // scoreHand
  scoreHand(): void {
    this.score();
    let handType = this.getRank >> UNIPOKER_20;
    this.handTypeCounts[handType] += 1;
  }

  wonHand(): void {
    this.mWins += 1;
  }
  // name
  get name(): string {
    return this.mName;
  }
  // hand
  get hand(): string {
    return super.toString();
  }
  // wins
  get wins(): number {
    return this.mWins;
  }
  // handTypeCounts
  get handTypeCounts(): number[] {
    return this.mHandTypeCounts;
  }
}

function playHands(players: Player[]): void {
  let cardDeck = new CardDeck();
  let handsPlayed = 0;
  let highestRank = 0;
  // begin do while
  do {
    cardDeck.shuffle();
    for (let player of players) {
      // clear
      player.clear();
    }
    for (let i = 0; i < UNIPOKER_5; i++) {
      for (let player of players) {
        player.takeCard(cardDeck.dealOneCard()!);
      }
    }

    for (let player of players) {
      player.scoreHand();
    }
    handsPlayed += 1;
    // reset highestRank
    highestRank = 0;
    // check highestRank
    for (let player of players) {
      if (player.getRank > highestRank) {
        highestRank = player.getRank;
      }
    }
    // check highestRank
    for (let player of players) {
      // We count ties as wins for each player.
      if (player.getRank === highestRank) {
        player.wonHand();
      }
    }
  } while (handsPlayed < UNIPOKER_2000);
}

function run(logsEnabled: boolean): void {
  let benchmark: Benchmark = new Benchmark();
  let startTime = ArkTools.timeInUs();
  let results: number[] = [];
  for (let i = 0; i < UNIPOKER_120; i++) {
    benchmark.runIteration();
  }
  let endTime = ArkTools.timeInUs();
  let time = (endTime - startTime) / UNIPOKER_1000;
  print('uni-poker: ms =', time);
  if (logsEnabled) {
    for (let index = 0; index < benchmark.players.length; index++) {
      print(`players ${index} wins:${benchmark._players[index].wins} handTypeCounts:${benchmark._players[index].handTypeCounts}`);
    }
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  run(false);
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

run(false);
