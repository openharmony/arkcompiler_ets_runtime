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

const NUM_512: number = 512;
const HEX_100000000: number = 0x100000000;
const HEX_10000000000: number = 0x10000000000;
const NUM_8: number = 8;
const NUM_64: number = 64;
const NUM_2: number = 2;
const NUM_3: number = 3;
const NUM_9007199254740991: number = 9007199254740991;
const NUM_511: number = 511;
const NUM_16: number = 16;
const NUM_15: number = 15;
const NUM_4: number = 4;
const NUM_5: number = 5;
const NUM_6: number = 6;
const NUM_7: number = 7;
const NUM_14: number = 14;
const NUM_18: number = 18;
const NUM_25: number = 25;
const NUM_9: number = 9;
const NUM_17: number = 17;
const NUM_19: number = 19;
const NUM_10: number = 10;
const NUM_13: number = 13;
const NUM_11: number = 11;
const NUM_26: number = 26;
const NUM_21: number = 21;
const NUM_22: number = 22;
const NUM_30: number = 30;
const NUM_32: number = 32;
const HEX_80000000: number = 0x80000000;
const HEX_FFFFFFFF: number = 0xffffffff;
const NUM_31: number = 31;
const HEX_F00000000000: number = 0xf00000000000;
const INT32MAX: number = 2147483647;
const INT32MIN: number = -2147483648;
const UINT32MAX: number = 4294967295;
const NUM_1000: number = 1000;
const NUM_120: number = 20;
const NUM_4500: number = 4500;
const NUM_500: number = 500;

/** @fileOverview Javascript SHA-256 implementation.
 *
 * An older version of this implementation is available in the public
 * domain, but this one is (c) Emily Stark, Mike Hamburg, Dan Boneh,
 * Stanford University 2008-2010 and BSD-licensed for liability
 * reasons.
 *
 * Special thanks to Aldo Cortesi for pointing out several bugs in
 * this code.
 *
 * @author Emily Stark
 * @author Mike Hamburg
 * @author Dan Boneh
 */
class Sha256 {
  /**
   * The SHA-256 initialization vector, to be precomputed.
   * @private
   */
  init: number[] = [];
  /*
       init:[0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19],
   */

  /**
   * The SHA-256 hash key, to be precomputed.
   * @private
   */
  _key: Int32Array = new Int32Array();
  /*
     _key:
     [0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
     0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
     0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
     0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
     0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
     0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
     0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
     0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2],
     */

  /**
   * The hash's block size, in bits.
   * @constant
   */
  blockSize = NUM_512;

  _h: Int32Array = new Int32Array();

  _buffer: number[] = [];

  _length = 0;

  /**
   * Hash a string or an array of words.
   * @static
   * @param {bitArray|String} data the data to hash.
   * @return {bitArray} The hash value, an array of 16 big-endian words.
   */
  static hash(data: number[] | String): number[] {
    return new Sha256(undefined).update(data).finalize();
  }
  /**
   * Context for a SHA-256 operation in progress.
   * @constructor
   */
  constructor(hash?: Sha256) {
    if (this._key.length === 0) {
      this._precompute();
    }
    if (hash !== undefined) {
      this._h = hash!._h.slice(0);
      this._buffer = hash!._buffer.slice(0);
      this._length = hash!._length;
    } else {
      this.reset();
    }
  }
  frac(x: number): number {
    let a = ((x - Math.floor(x)) * HEX_100000000) | 0;
    a = initUInt32(a);
    return a;
  }
  /**
   * Function to precompute _init and _key.
   * @private
   */
  private _precompute(): void {
    let i = 0;
    let prime = 2;
    let isPrime: Boolean;
    this._init = new Int32Array(NUM_8);
    this._key = new Int32Array(NUM_64);
    while (i < NUM_64) {
      isPrime = true;
      let factor = NUM_2;
      while (factor * factor <= prime) {
        if (prime % factor === 0) {
          isPrime = false;
          break;
        }
        factor += 1;
      }
      if (isPrime) {
        if (i < NUM_8) {
          this._init[i] = this.frac(Math.pow(prime, 1 / NUM_2));
        }
        this._key[i] = this.frac(Math.pow(prime, 1 / NUM_3));
        i += 1;
      }
      prime += 1;
    }
  }
  /**
   * Reset the hash state.
   * @return this
   */
  reset(): Sha256 {
    this._h = this._init;
    this._buffer = [];
    this._length = 0;
    return this;
  }
  /**
   * Input several words to the hash.
   * @param {bitArray|String} data the data to hash.
   * @return this
   */
  update(datas: number[] | String): Sha256 {
    let data: number[] = [];
    if (typeof datas === 'string') {
      data = Utf8String.toBits(datas);
    } else {
      data = datas as number[];
    }
    let i: number = 0;
    this._buffer = BitArray.concat(this._buffer, data);
    let b = this._buffer;
    let ol = this._length;
    this._length = this._length + BitArray.bitLength(data);
    let nl = this._length;
    if (nl > NUM_9007199254740991) {
      throw new Error('INVALID Cannot hash more than 2^53 - 1 bits');
    }
    let c = b.map(item => initUInt32(item));
    let j = 0;
    i = NUM_512 + ol - ((NUM_512 + ol) & NUM_511);
    while (i <= nl) {
      this._block(c.slice(NUM_16 * j, NUM_16 * (j + 1)));
      j += 1;
      i += NUM_512;
    }
    b.splice(0, NUM_16 * j);
    return this;
  }
  /**
   * Complete hashing and output the hash value.
   * @return {bitArray} The hash value, an array of 8 big-endian words.
   */
  finalize(): number[] {
    // Round out and push the buffer
    this._buffer = BitArray.concat(this._buffer, [BitArray.partial(1, 1)]);

    // Round out the buffer to a multiple of 16 words, less the 2 length words.
    let i = this._buffer.length + NUM_2;
    while ((i & NUM_15) !== 0) {
      this._buffer.push(0);
      i += 1;
    }
    // append the length
    let a = (this._length / HEX_100000000) | 0;
    this._buffer.push(a);
    this._buffer.push(this._length);

    while (!(this._buffer.length === 0)) {
      let chunk = this._buffer.slice(0, NUM_16);
      this._buffer.splice(0, NUM_16);
      this._block(chunk);
    }
    this.reset();

    return [...this._h];
  }
  /**
   * Perform one cycle of SHA-256.
   * @param {Uint32Array|bitArray} w one block of words.
   * @private
   */
  private _block(input: number[]): void {
    let w = input;
    let tmp: number;
    let a: number;
    let b: number;
    let k = this._key;
    let h0 = this._h[0];
    let h1 = this._h[1];
    let h2 = this._h[NUM_2];
    let h3 = this._h[NUM_3];
    let h4 = this._h[NUM_4];
    let h5 = this._h[NUM_5];
    let h6 = this._h[NUM_6];
    let h7 = this._h[NUM_7];
    for (let i = 0; i < NUM_64; i++) {
      if (i < NUM_16) {
        tmp = w[i];
      } else {
        a = initUInt32(w[(i + 1) & NUM_15]);
        b = initUInt32(w[(i + NUM_14) & NUM_15]);
        let value1 = (a >>> NUM_7) ^ (a >>> NUM_18) ^ (a >>> NUM_3) ^ (a << NUM_25) ^ (a << NUM_14);
        let value2 = (b >>> NUM_17) ^ (b >>> NUM_19) ^ (b >>> NUM_10) ^ (b << NUM_15) ^ (b << NUM_13);
        let value3 = w[i & NUM_15] + w[(i + NUM_9) & NUM_15];
        w[i & NUM_15] = initUInt32(value1 + value2 + value3);
        tmp = w[i & NUM_15];
      }
      h4 = initUInt32(h4);
      let value = (h4 >>> NUM_6) ^ (h4 >>> NUM_11) ^ (h4 >>> NUM_25) ^ (h4 << NUM_26) ^ (h4 << NUM_21) ^ (h4 << NUM_7);
      tmp = initUInt32(tmp + h7 + value + (h6 ^ (h4 & (h5 ^ h6))) + k[i]);
      h7 = initUInt32(h6);
      h6 = initUInt32(h5);
      h5 = initUInt32(h4);
      h4 = initUInt32(h3 + tmp);
      h3 = initUInt32(h2);
      h2 = initUInt32(h1);
      h1 = initUInt32(h0);
      h0 = initUInt32(
        tmp + ((h1 & h2) ^ (h3 & (h1 ^ h2))) + ((h1 >>> NUM_2) ^ (h1 >>> NUM_13) ^ (h1 >>> NUM_22) ^ (h1 << NUM_30) ^ (h1 << NUM_19) ^ (h1 << NUM_10))
      );
    }
    this._h[0] = initUInt32(this._h[0] + h0);
    this._h[1] = initUInt32(this._h[1] + h1);
    this._h[NUM_2] = initUInt32(this._h[NUM_2] + h2);
    this._h[NUM_3] = initUInt32(this._h[NUM_3] + h3);
    this._h[NUM_4] = initUInt32(this._h[NUM_4] + h4);
    this._h[NUM_5] = initUInt32(this._h[NUM_5] + h5);
    this._h[NUM_6] = initUInt32(this._h[NUM_6] + h6);
    this._h[NUM_7] = initUInt32(this._h[NUM_7] + h7);
    this._init = this._h;
  }
}

class Utf8String {
  static toBits(str: string): number[] {
    let out: number[] = [];
    let i: number = 0;
    let tmp: number = 0;
    while (i < str.length) {
      tmp = (tmp << NUM_8) | str.charCodeAt(i);
      if ((i & NUM_3) === NUM_3) {
        out.push(tmp);
        tmp = 0;
      }
      i += 1;
    }
    if ((i & NUM_3) !== 0) {
      out.push(BitArray.partial(NUM_8 * (i & NUM_3), tmp));
    }
    return out;
  }
}
class BitArray {
  /**
   * Concatenate two bit arrays.
   * @param {bitArray} a1 The first array.
   * @param {bitArray} a2 The second array.
   * @return {bitArray} The concatenation of a1 and a2.
   */
  static concat(a1: number[], a2: number[]): number[] {
    if (a1.length === 0 || a2.length === 0) {
      return a1.concat(a2);
    }

    let last = a1[a1.length - 1];
    let shift = BitArray.getPartial(last);
    if (shift === NUM_32) {
      return a1.concat(a2);
    } else {
      return BitArray.shiftRight(a2, shift, last | 0, a1.slice(0, a1.length - 1));
    }
  }
  /**
   * Find the length of an array of bits.
   * @param {bitArray} a The array.
   * @return {Number} The length of a, in bits.
   */
  static bitLength(a: number[]): number {
    let l = a.length;
    let x: number | null;
    if (l === 0) {
      return 0;
    }
    x = a[l - 1];
    return (l - 1) * NUM_32 + BitArray.getPartial(x!);
  }
  /**
   * Truncate an array.
   * @param {bitArray} a The array.
   * @param {Number} len The length to truncate to, in bits.
   * @return {bitArray} A new array, truncated to len bits.
   */
  static clamp(a: number[], len: number): number[] {
    let _a = a;
    let _len = len;
    if (_a.length * NUM_32 < _len) {
      return _a;
    }

    let sliceLength = Math.ceil(_len / NUM_32);
    _a = _a.slice(0, sliceLength);

    let l = _a.length;
    _len = _len & NUM_32;

    if (l > 0 && _len !== 0) {
      _a[l - 1] = BitArray.partial(_len, _a[l - 1] & (HEX_80000000 >> (_len - 1)), true);
    }

    return _a;
  }
  /**
   * Make a partial word for a bit array.
   * @param {Number} len The number of bits in the word.
   * @param {Number} x The bits.
   * @param {Number} [_end=0] Pass 1 if x has already been shifted to the high side.
   * @return {Number} The partial word.
   */
  static partial(len: number, x: number, end: Boolean = false): number {
    if (len === NUM_32) {
      return x;
    } else {
      let result = (end ? x | 0 : initUInt32(x << (NUM_32 - len))) + len * HEX_10000000000;
      return result;
    }
  }
  /**
   * Get the number of bits used by a partial word.
   * @param {Number} x The partial word.
   * @return {Number} The number of bits used by the partial word.
   */
  static getPartial(x: number): number {
    return Math.round(x / HEX_10000000000) !== 0 ? Math.round(x / HEX_10000000000) : NUM_32;
  }
  /** Shift an array right.
   * @param {bitArray} a The array to shift.
   * @param {Number} shift The number of bits to shift.
   * @param {Number} [carry=0] A byte to carry in
   * @param {bitArray} [out=[]] An array to prepend to the output.
   * @private
   */
  static shiftRight(a: number[], shift: number, carry: number, out: number[]): number[] {
    let last2 = 0;
    let shift2: number;
    let _out = out;
    let _carry = initUInt32(carry);
    let _shift = shift;
    if (_out.length === 0) {
      _out = [];
    }

    while (_shift >= NUM_32) {
      _out.push(_carry);
      _carry = 0;
      _shift -= NUM_32;
    }

    if (_shift === 0) {
      return _out.concat(a);
    }

    for (let i = 0; i < a.length; i++) {
      _out.push(_carry | (initUInt32(a[i]) >>> _shift));
      _carry = (a[i] << (NUM_32 - _shift)) & HEX_FFFFFFFF;
    }

    last2 = a.length === 0 ? 0 : a[a.length - 1];
    shift2 = BitArray.getPartial(last2) !== 0 ? 1 : 0;
    _out.push(BitArray.partial((_shift + shift2) & NUM_31, _shift + shift2 > NUM_32 ? _carry : _out.pop(), true));
    return _out;
  }
}
class Hex {
  /** Convert from a bitArray to a hex string. */
  static fromBits(arr: number[]): String {
    let out = '';
    for (let i = 0; i < arr.length; i++) {
      let hexValue = (arr[i] | 0) + HEX_F00000000000;
      out += hexValue.toString(NUM_16).substring(NUM_4);
    }
    let str = out.substring(0, BitArray.bitLength(arr) / NUM_4);
    return `${str}`;
  }
}

function initUInt32(a: number): number {
  let tmp = a;
  if (tmp > INT32MAX) {
    let max = UINT32MAX;
    tmp = tmp % (max + 1);
    if (tmp > INT32MAX) {
      tmp = tmp - max - 1;
    }
  } else if (tmp < INT32MIN) {
    let max = UINT32MAX;
    tmp = tmp % (max + 1);
    if (tmp < INT32MIN) {
      tmp = tmp + max + 1;
    }
  }
  return tmp;
}

/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  @State
 *  @Tags Jetstream2
 */
class Benchmark {
  /*
   *  @Benchmark
   */
  runIteration(): void {
    let startTime = ArkTools.timeInUs() / NUM_1000;
    for (let index = 0; index < NUM_120; index++) {
      let hash = Sha256.hash('b4d');
      for (let i = 0; i < NUM_4500; i++) {
        hash = Sha256.hash(hash);
        if (i % NUM_500 === 0) {
          //deBugLog(`第${i + 1}条的hash：${hash}`)
        }
      }
      if (Hex.fromBits(hash) !== '719043495be84b97fe4f5d7e61c99d6d1ba0cd6974a6b10c684c25a44ddd0c03') {
        throw new Error('Bad result');
      }
    }
    let endTime = ArkTools.timeInUs() / NUM_1000;
    print('stanford-crypto-sha256: ms = ' + `${endTime - startTime}`);
  }
}
declare interface ArkTools {
  timeInUs(args: number): number;
}
let isDebug: boolean = false;
function deBugLog(msg: string): void {
  if (isDebug) {
    print(msg);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().runIteration();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().runIteration();