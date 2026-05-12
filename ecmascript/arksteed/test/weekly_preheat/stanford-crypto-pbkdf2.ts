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

const PBKDF2_NUM_2: number = 2;
const PBKDF2_NUM_3: number = 3;
const PBKDF2_NUM_4: number = 4;
const PBKDF2_NUM_5: number = 5;
const PBKDF2_NUM_6: number = 6;
const PBKDF2_NUM_7: number = 7;
const PBKDF2_NUM_8: number = 8;
const PBKDF2_NUM_9: number = 9;
const PBKDF2_NUM_10: number = 10;
const PBKDF2_NUM_11: number = 11;
const PBKDF2_NUM_12: number = 12;
const PBKDF2_NUM_13: number = 13;
const PBKDF2_NUM_14: number = 14;
const PBKDF2_NUM_15: number = 15;
const PBKDF2_NUM_16: number = 16;
const PBKDF2_NUM_17: number = 17;
const PBKDF2_NUM_18: number = 18;
const PBKDF2_NUM_19: number = 19;
const PBKDF2_NUM_25: number = 25;
const PBKDF2_NUM_26: number = 26;
const PBKDF2_NUM_21: number = 21;
const PBKDF2_NUM_22: number = 22;
const PBKDF2_NUM_30: number = 30;
const PBKDF2_NUM_31: number = 31;
const PBKDF2_NUM_32: number = 32;
const PBKDF2_NUM_64: number = 64;
const PBKDF2_NUM_120: number = 20;
const PBKDF2_NUM_511: number = 511;
const PBKDF2_NUM_512: number = 512;
const PBKDF2_NUM_1000: number = 1000;
const PBKDF2_NUM_10000: number = 10000;
const PBKDF2_NUM_2147483647: number = 2147483647;
const PBKDF2_NUM_NEGATIVE_2147483648: number = 2147483648;
const PBKDF2_NUM_9007199254740991: number = 9007199254740991;
const PBKDF2_HEX_10000000000: number = 0x10000000000;
const PBKDF2_HEX_80000000: number = 0x80000000;
const PBKDF2_HEX_FFFFFFFF: number = 0xffffffff;
const PBKDF2_HEX_F00000000000: number = 0xf00000000000;
const PBKDF2_HEX_100000000: number = 0x100000000;
const PBKDF2_HEX_36363636: number = 0x36363636;
const PBKDF2_HEX_5C5C5C5C: number = 0x5c5c5c5c;

class BitArray {
  /**
   * Make a partial word for a bit array.
   * @param {Number} len The number of bits in the word.
   * @param {Number} x The bits.
   * @param {Number} [end=0] Pass 1 if x has already been shifted to the high side.
   * @return {Number} The partial word.
   */
  static partial(len: number, x: number, end: Boolean = false): number {
    if (len === PBKDF2_NUM_32) {
      return x;
    } else {
      let result = (end ? x | 0 : initUInt32(x << (PBKDF2_NUM_32 - len))) + len * PBKDF2_HEX_10000000000;
      return result;
    }
  }

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
    if (shift === PBKDF2_NUM_32) {
      return a1.concat(a2);
    } else {
      return BitArray.shiftRight(a2, shift, last | 0, a1.slice(0, a1.length - 1));
    }
  }
  /**
   * Truncate an array.
   * @param {bitArray} aa The array.
   * @param {Number} len1 The length to truncate to, in bits.
   * @return {bitArray} A new array, truncated to len bits.
   */
  static clamp(aa: number[], len1: number): number[] {
    let a = aa;
    let len = len1;
    if (a.length * PBKDF2_NUM_32 < len) {
      return a;
    }

    let sliceLength = Math.ceil(len / PBKDF2_NUM_32);
    a = a.slice(0, sliceLength);

    let l = a.length;
    len = len & PBKDF2_NUM_31;

    if (l > 0 && len !== 0) {
      a[l - 1] = BitArray.partial(len, a[l - 1] & (PBKDF2_HEX_80000000 >> (len - 1)), true);
    }

    return a;
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
    return (l - 1) * PBKDF2_NUM_32 + BitArray.getPartial(x!);
  }

  /**
   * Get the number of bits used by a partial word.
   * @param {Number} x The partial word.
   * @return {Number} The number of bits used by the partial word.
   */
  static getPartial(x: number): number {
    return Math.round(x / PBKDF2_HEX_10000000000) || PBKDF2_NUM_32;
  }

  /** Shift an array right.
   * @param {bitArray} a The array to shift.
   * @param {Number} shift1 The number of bits to shift.
   * @param {Number} [carry1=0] A byte to carry in
   * @param {bitArray} [out1=[]] An array to prepend to the output.
   * @private
   */
  static shiftRight(a: number[], shift1: number, carry1: number, out1: number[]): number[] {
    let last2 = 0;
    let shift2: number;
    let out = out1;
    let carry = initUInt32(carry1);
    let shift = shift1;
    if (out.length === 0) {
      out = [];
    }

    while (shift >= PBKDF2_NUM_32) {
      out.push(carry);
      carry = 0;
      shift -= PBKDF2_NUM_32;
    }

    if (shift === 0) {
      return out.concat(a);
    }

    for (let i = 0; i < a.length; i++) {
      out.push(carry | (initUInt32(a[i]) >>> shift));
      carry = (a[i] << (PBKDF2_NUM_32 - shift)) & PBKDF2_HEX_FFFFFFFF;
    }

    last2 = a.length === 0 ? 0 : a[a.length - 1];
    shift2 = BitArray.getPartial(last2) !== 0 ? 1 : 0;
    out.push(BitArray.partial((shift + shift2) & PBKDF2_NUM_31, shift + shift2 > PBKDF2_NUM_32 ? carry : out.pop(), true));
    return out;
  }
}
/**
 * UTF-8 strings
 * @namespace
 */
class Utf8String {
  static toBits(str: string): number[] {
    let out: number[] = [];
    let i: number = 0;
    let tmp: number = 0;
    while (i < str.length) {
      tmp = (tmp << PBKDF2_NUM_8) | str.charCodeAt(i);
      if ((i & PBKDF2_NUM_3) === PBKDF2_NUM_3) {
        out.push(tmp);
        tmp = 0;
      }
      i += 1;
    }
    if ((i & PBKDF2_NUM_3) !== 0) {
      out.push(BitArray.partial(PBKDF2_NUM_8 * (i & PBKDF2_NUM_3), tmp));
    }
    return out;
  }
}

/**
 * Hexadecimal
 * @namespace
 */
class SjclCodecHex {
  /** Convert from a bitArray to a hex string. */
  static fromBits(arr: number[]): String {
    let out = '';
    for (let i = 0; i < arr.length; i++) {
      let hexValue = (arr[i] | 0) + PBKDF2_HEX_F00000000000;
      out += hexValue.toString(PBKDF2_NUM_16).substring(PBKDF2_NUM_4);
    }
    let str = out.substring(0, BitArray.bitLength(arr) / PBKDF2_NUM_4);
    return `${str}`;
  }
  /** Convert from a hex string to a bitArray. */
  static toBits(str: string): number[] {
    let out: number[] = [];
    let strHex = str.replace(new RegExp('\\s|0x', 'g'), '');
    let len = strHex.length;
    strHex = strHex + '00000000';

    let i = 0;
    while (i < strHex.length) {
      let hexValue = hexSwitch(strHex.slice(i, i + PBKDF2_NUM_8));
      out.push(initUInt32(hexValue) ^ 0);
      i += PBKDF2_NUM_8;
    }

    return BitArray.clamp(out, len * PBKDF2_NUM_4);
  }
}

function hexSwitch(hexStr: string): number {
  let i = 0;
  let sum = 0;
  while (i < hexStr.length) {
    let s = hexStr.charAt(i);
    let n = 0;
    switch (s) {
      case 'A':
        n = PBKDF2_NUM_10;
        break;
      case 'B':
        n = PBKDF2_NUM_11;
        break;
      case 'C':
        n = PBKDF2_NUM_12;
        break;
      case 'D':
        n = PBKDF2_NUM_13;
        break;
      case 'E':
        n = PBKDF2_NUM_14;
        break;
      case 'F':
        n = PBKDF2_NUM_15;
        break;
      default:
        n = n as number;
        break;
    }
    sum = sum + n * Math.pow(PBKDF2_NUM_16, hexStr.length - i - 1);
    i += 1;
  }
  return sum;
}

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
  key: number[] = [];
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
  blockSize = PBKDF2_NUM_512;

  h: number[] = [];

  buffer: number[] = [];

  length: number = 0;

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
    if (this.key.length === 0) {
      this.precompute();
    }
    if (hash !== undefined) {
      this.h = hash!.h.slice(0);
      this.buffer = hash!.buffer.slice(0);
      this.length = hash!.length;
    } else {
      this.reset();
    }
  }
  frac(x: number): number {
    let a = ((x - Math.floor(x)) * PBKDF2_HEX_100000000) | 0;
    a = initUInt32(a);
    return a;
  }
  /**
   * Function to precompute _init and _key.
   * @private
   */
  private precompute(): void {
    let i = 0;
    let prime = 2;
    let isPrime: Boolean;
    this.init = Array.from({ length: PBKDF2_NUM_8 }, (a: number) => 0);
    this.key = Array.from({ length: PBKDF2_NUM_64 }, (a: number) => 0);
    while (i < PBKDF2_NUM_64) {
      isPrime = true;
      let factor = 2;
      while (factor * factor <= prime) {
        if (prime % factor === 0) {
          isPrime = false;
          break;
        }
        factor += 1;
      }
      if (isPrime) {
        if (i < PBKDF2_NUM_8) {
          this.init[i] = this.frac(Math.pow(prime, 1 / PBKDF2_NUM_2));
        }
        this.key[i] = this.frac(Math.pow(prime, 1 / PBKDF2_NUM_3));
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
    this.h = this.init;
    this.buffer = [];
    this.length = 0;
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
    this.buffer = BitArray.concat(this.buffer, data);
    let b = this.buffer;
    let ol = this.length;
    this.length = this.length + BitArray.bitLength(data);
    let nl = this.length;
    if (nl > PBKDF2_NUM_9007199254740991) {
      throw new Error('INVALID Cannot hash more than 2^53 - 1 bits');
    }
    let c = b.map(item => initUInt32(item));
    let j = 0;
    i = PBKDF2_NUM_512 + ol - ((PBKDF2_NUM_512 + ol) & PBKDF2_NUM_511);
    while (i <= nl) {
      this.block(c.slice(PBKDF2_NUM_16 * j, PBKDF2_NUM_16 * (j + 1)));
      j += 1;
      i += PBKDF2_NUM_512;
    }
    b.splice(0, PBKDF2_NUM_16 * j);
    return this;
  }
  /**
   * Complete hashing and output the hash value.
   * @return {bitArray} The hash value, an array of 8 big-endian words.
   */
  finalize(): number[] {
    // Round out and push the buffer
    this.buffer = BitArray.concat(this.buffer, [BitArray.partial(1, 1)]);

    // Round out the buffer to a multiple of 16 words, less the 2 length words.
    let i = this.buffer.length + PBKDF2_NUM_2;
    while ((i & PBKDF2_NUM_15) !== 0) {
      this.buffer.push(0);
      i += 1;
    }
    // append the length
    let a = (this.length / PBKDF2_HEX_100000000) | 0;
    this.buffer.push(a);
    this.buffer.push(this.length);

    while (!(this.buffer.length === 0)) {
      let chunk = this.buffer.slice(0, PBKDF2_NUM_16);
      this.buffer.splice(0, PBKDF2_NUM_16);
      this.block(chunk);
    }
    this.reset();
    return this.h;
  }

  /**
   * Perform one cycle of SHA-256.
   * @param {Uint32Array|bitArray} w one block of words.
   * @private
   */
  private block(input: number[]): void {
    let w = input;
    let tmp: number;
    let a: number;
    let b: number;
    let k = this.key;
    let h0 = this.h[0];
    let h1 = this.h[1];
    let h2 = this.h[PBKDF2_NUM_2];
    let h3 = this.h[PBKDF2_NUM_3];
    let h4 = this.h[PBKDF2_NUM_4];
    let h5 = this.h[PBKDF2_NUM_5];
    let h6 = this.h[PBKDF2_NUM_6];
    let h7 = this.h[PBKDF2_NUM_7];

    /* Rationale for placement of |0 :
     * If a value can overflow is original 32 bits by a factor of more than a few
     * million (2^23 ish), there is a possibility that it might overflow the
     * 53-bit mantissa and lose precision.
     *
     * To avoid this, we clamp back to 32 bits by |'ing with 0 on any value that
     * propagates around the loop, and on the hash state h[].  I don't believe
     * that the clamps on h4 and on h0 are strictly necessary, but it's close
     * (for h4 anyway), and better safe than sorry.
     *
     * The clamps on h[] are necessary for the output to be correct even in the
     * common case and for short inputs.
     */
    for (let i = 0; i < PBKDF2_NUM_64; i++) {
      // load up the input word for this round
      if (i < PBKDF2_NUM_16) {
        tmp = w[i];
      } else {
        a = initUInt32(w[(i + 1) & PBKDF2_NUM_15]);
        b = initUInt32(w[(i + PBKDF2_NUM_14) & PBKDF2_NUM_15]);

        let value1 = (a >>> PBKDF2_NUM_7) ^ (a >>> PBKDF2_NUM_18) ^ (a >>> PBKDF2_NUM_3) ^ (a << PBKDF2_NUM_25) ^ (a << PBKDF2_NUM_14);
        let value2 = (b >>> PBKDF2_NUM_17) ^ (b >>> PBKDF2_NUM_19) ^ (b >>> PBKDF2_NUM_10) ^ (b << PBKDF2_NUM_15) ^ (b << PBKDF2_NUM_13);
        let value3 = w[i & PBKDF2_NUM_15] + w[(i + PBKDF2_NUM_9) & PBKDF2_NUM_15];

        w[i & PBKDF2_NUM_15] = initUInt32(value1 + value2 + value3);
        tmp = w[i & PBKDF2_NUM_15];
      }
      h4 = initUInt32(h4);
      let value =
        (h4 >>> PBKDF2_NUM_6) ^ (h4 >>> PBKDF2_NUM_11) ^ (h4 >>> PBKDF2_NUM_25) ^ (h4 << PBKDF2_NUM_26) ^ (h4 << PBKDF2_NUM_21) ^ (h4 << PBKDF2_NUM_7);
      tmp = initUInt32(tmp + h7 + value + (h6 ^ (h4 & (h5 ^ h6))) + k[i]);
      // shift register
      h7 = initUInt32(h6);
      h6 = initUInt32(h5);
      h5 = initUInt32(h4);
      h4 = initUInt32(h3 + tmp);
      h3 = initUInt32(h2);
      h2 = initUInt32(h1);
      h1 = initUInt32(h0);

      h0 = initUInt32(
        tmp +
          ((h1 & h2) ^ (h3 & (h1 ^ h2))) +
          ((h1 >>> PBKDF2_NUM_2) ^ (h1 >>> PBKDF2_NUM_13) ^ (h1 >>> PBKDF2_NUM_22) ^ (h1 << PBKDF2_NUM_30) ^ (h1 << PBKDF2_NUM_19) ^ (h1 << PBKDF2_NUM_10))
      );
    }

    this.h[0] = initUInt32(this.h[0] + h0);
    this.h[1] = initUInt32(this.h[1] + h1);
    this.h[PBKDF2_NUM_2] = initUInt32(this.h[PBKDF2_NUM_2] + h2);
    this.h[PBKDF2_NUM_3] = initUInt32(this.h[PBKDF2_NUM_3] + h3);
    this.h[PBKDF2_NUM_4] = initUInt32(this.h[PBKDF2_NUM_4] + h4);
    this.h[PBKDF2_NUM_5] = initUInt32(this.h[PBKDF2_NUM_5] + h5);
    this.h[PBKDF2_NUM_6] = initUInt32(this.h[PBKDF2_NUM_6] + h6);
    this.h[PBKDF2_NUM_7] = initUInt32(this.h[PBKDF2_NUM_7] + h7);
    this.init = this.h;
  }
}

/** Hmac with the specified hash function.
 * @constructor
 * @param {bitArray} key the key for Hmac.
 * @param {Object} [Hash=sjcl.hash.sha256] The hash function to use.
 */

class Hmac {
  hash: Sha256;
  baseHash: [Sha256, Sha256];
  resultHash: Sha256;
  exKey: number[][] = [[], []];
  updated: Boolean = false;

  constructor(key: number[] = [], hash: Sha256 = new Sha256()) {
    this.hash = hash;
    this.baseHash = [new Sha256(), new Sha256()];
    let bs = hash.blockSize / PBKDF2_NUM_32;
    this.exKey[0] = Array.from({ length: bs }, (a: number) => 0);
    this.exKey[1] = Array.from({ length: bs }, (a: number) => 0);
    let _key = key;
    if (_key.length > bs) {
      _key = Sha256.hash(_key);
    }
    for (let i = 0; i < bs; i++) {
      if (i < _key.length) {
        this.exKey[0][i] = _key[i] ^ PBKDF2_HEX_36363636;
        this.exKey[1][i] = _key[i] ^ PBKDF2_HEX_5C5C5C5C;
      } else {
        this.exKey[0][i] = 0 ^ PBKDF2_HEX_36363636;
        this.exKey[1][i] = 0 ^ PBKDF2_HEX_5C5C5C5C;
      }
    }

    this.baseHash[0].update(this.exKey[0]);
    this.baseHash[1].update(this.exKey[1]);
    this.resultHash = this.baseHash[0];
  }

  encrypt(data: number[]): number[] {
    if (!this.updated) {
      this.update(data);
      return this.digest();
    } else {
      throw new Error('encrypt on already updated Hmac called!');
    }
  }

  reset(): void {
    this.resultHash = new Sha256(this.baseHash[0]);
    this.updated = false;
  }

  update(data: number[]): void {
    this.updated = true;
    this.resultHash.buffer = [];
    this.resultHash.update(data);
  }

  digest(): number[] {
    let w = this.resultHash.finalize();

    let b = new Sha256(this.baseHash[1]);
    b.buffer = [];
    let c = b.update(w);

    let finalResult = c.finalize();
    this.reset();
    return finalResult;
  }
}

class Pbkdf2 {
  static pbkdf2(password: string | number[], salt: string | number[], count: number = PBKDF2_NUM_10000, length: number = 0, rrff?: Hmac): number[] {
    if (length === undefined || count < 0) {
      throw new Error('nvalid params to pbkdf2');
    }
    let passwordArr: number[] = [];
    let saltArr: number[] = [];
    if (typeof password === 'string') {
      let arr = Utf8String.toBits(password);
      let i = 0;
      while (i < arr.length) {
        let n = arr[i];
        passwordArr.push(initUInt32(n));
        i += 1;
      }
    } else {
      let arr = password;
      let i = 0;
      while (i < arr.length) {
        let n = arr[i];
        passwordArr.push(initUInt32(n));
        i += 1;
      }
    }
    if (typeof salt === 'string') {
      let arr = Utf8String.toBits(salt);
      let i = 0;
      while (i < arr.length) {
        let n = arr[i];
        saltArr.push(initUInt32(n));
        i += 1;
      }
    } else {
      let arr = salt;
      let i = 0;
      while (i < arr.length) {
        let n = arr[i];
        saltArr.push(initUInt32(n));
        i += 1;
      }
    }

    let u: number[];
    let ui: number[];
    let out: number[] = [];

    let k = 1;
    while (PBKDF2_NUM_32 * out.length < (length === 0 ? 1 : length)!) {
      u = new Hmac(passwordArr).encrypt(BitArray.concat(saltArr, [k]));
      ui = new Hmac(passwordArr).encrypt(BitArray.concat(saltArr, [k]));

      for (let i = 1; i < count; i++) {
        ui = new Hmac(passwordArr).encrypt(ui);
        for (let j = 0; j < ui.length; j++) {
          u[j] ^= ui[j];
        }
      }

      out = out.concat(u);
      k += 1;
    }
    if (length !== 0) {
      out = BitArray.clamp(out, length!);
    }
    //debugLog("pbkdf2-out", out)
    return out;
  }
}
/*
 *  @State
 *  @Tags Jetstream2
 */
class Benchmark {
  /*
   * @Benchmark
   */
  runIteration(): void {
    let startTime = ArkTools.timeInUs() / PBKDF2_NUM_1000;
    for (let i = 0; i < PBKDF2_NUM_120; i++) {
      let password = 'b4db33f';
      let salt = 'BA DB EE FB AD BE EF 00';
      let count = 2048;
      let strArr = SjclCodecHex.toBits(salt);
      //debugLog("strArr", strArr)
      let output = SjclCodecHex.fromBits(Pbkdf2.pbkdf2(password, strArr, count));
      //debugLog("output", output)
      if (output !== '5436c4c0d73e425a521f4a5a13e0fffe5cd957a823f435385fa0a75068a26803') {
        throw new Error('Bad output');
      }
    }
    let endTime = ArkTools.timeInUs() / PBKDF2_NUM_1000;
    print(`stanford-crypto-pbkdf2: ms = ${endTime - startTime}`);
  }
}
declare interface ArkTools {
  timeInUs(args: number): number;
}

function debugLog(str: String, object: String | number[]): void {
  let isLog = true;
  if (isLog) {
    print(`${str}: ${object}`);
  }
}

function initUInt32(a: number): number {
  let tmp = a;
  if (tmp > PBKDF2_NUM_2147483647) {
    let max = 4294967295;
    tmp = tmp % (max + 1);
    if (tmp > PBKDF2_NUM_2147483647) {
      tmp = tmp - max - 1;
    }
  } else if (tmp < -PBKDF2_NUM_NEGATIVE_2147483648) {
    let max = 4294967295;
    tmp = tmp % (max + 1);
    if (tmp < -PBKDF2_NUM_NEGATIVE_2147483648) {
      tmp = tmp + max + 1;
    }
  }
  return tmp;
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
