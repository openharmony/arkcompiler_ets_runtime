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
const CRYPTO_NUMBER_0XFFFFFF: number = 0xffffff;
const CRYPTO_NUMBER_0XEFCAFE: number = 0xefcafe;
const CRYPTO_NUMBER_0: number = 0;
const CRYPTO_NUMBER_1: number = 1;
const CRYPTO_NUMBER_2: number = 2;
const CRYPTO_NUMBER_3: number = 3;
const CRYPTO_NUMBER_5: number = 5;
const CRYPTO_NUMBER_7: number = 7;
const CRYPTO_NUMBER_11: number = 11;
const CRYPTO_NUMBER_13: number = 13;
const CRYPTO_NUMBER_17: number = 17;
const CRYPTO_NUMBER_19: number = 19;
const CRYPTO_NUMBER_23: number = 23;
const CRYPTO_NUMBER_29: number = 29;
const CRYPTO_NUMBER_31: number = 31;
const CRYPTO_NUMBER_37: number = 37;
const CRYPTO_NUMBER_41: number = 41;
const CRYPTO_NUMBER_43: number = 43;
const CRYPTO_NUMBER_47: number = 47;
const CRYPTO_NUMBER_53: number = 53;
const CRYPTO_NUMBER_59: number = 59;
const CRYPTO_NUMBER_61: number = 61;
const CRYPTO_NUMBER_67: number = 67;
const CRYPTO_NUMBER_71: number = 71;
const CRYPTO_NUMBER_73: number = 73;
const CRYPTO_NUMBER_79: number = 79;
const CRYPTO_NUMBER_83: number = 83;
const CRYPTO_NUMBER_89: number = 89;
const CRYPTO_NUMBER_97: number = 97;
const CRYPTO_NUMBER_101: number = 101;
const CRYPTO_NUMBER_103: number = 103;
const CRYPTO_NUMBER_107: number = 107;
const CRYPTO_NUMBER_109: number = 109;
const CRYPTO_NUMBER_113: number = 113;
const CRYPTO_NUMBER_127: number = 127;
const CRYPTO_NUMBER_131: number = 131;
const CRYPTO_NUMBER_137: number = 137;
const CRYPTO_NUMBER_139: number = 139;
const CRYPTO_NUMBER_149: number = 149;
const CRYPTO_NUMBER_151: number = 151;
const CRYPTO_NUMBER_157: number = 157;
const CRYPTO_NUMBER_163: number = 163;
const CRYPTO_NUMBER_167: number = 167;
const CRYPTO_NUMBER_173: number = 173;
const CRYPTO_NUMBER_179: number = 179;
const CRYPTO_NUMBER_181: number = 181;
const CRYPTO_NUMBER_191: number = 191;
const CRYPTO_NUMBER_193: number = 193;
const CRYPTO_NUMBER_197: number = 197;
const CRYPTO_NUMBER_199: number = 199;
const CRYPTO_NUMBER_211: number = 211;
const CRYPTO_NUMBER_223: number = 223;
const CRYPTO_NUMBER_227: number = 227;
const CRYPTO_NUMBER_229: number = 229;
const CRYPTO_NUMBER_233: number = 233;
const CRYPTO_NUMBER_239: number = 239;
const CRYPTO_NUMBER_241: number = 241;
const CRYPTO_NUMBER_251: number = 251;
const CRYPTO_NUMBER_257: number = 257;
const CRYPTO_NUMBER_263: number = 263;
const CRYPTO_NUMBER_269: number = 269;
const CRYPTO_NUMBER_271: number = 271;
const CRYPTO_NUMBER_277: number = 277;
const CRYPTO_NUMBER_281: number = 281;
const CRYPTO_NUMBER_283: number = 283;
const CRYPTO_NUMBER_293: number = 293;
const CRYPTO_NUMBER_307: number = 307;
const CRYPTO_NUMBER_311: number = 311;
const CRYPTO_NUMBER_313: number = 313;
const CRYPTO_NUMBER_317: number = 317;
const CRYPTO_NUMBER_331: number = 331;
const CRYPTO_NUMBER_337: number = 337;
const CRYPTO_NUMBER_347: number = 347;
const CRYPTO_NUMBER_349: number = 349;
const CRYPTO_NUMBER_353: number = 353;
const CRYPTO_NUMBER_359: number = 359;
const CRYPTO_NUMBER_367: number = 367;
const CRYPTO_NUMBER_373: number = 373;
const CRYPTO_NUMBER_379: number = 379;
const CRYPTO_NUMBER_383: number = 383;
const CRYPTO_NUMBER_389: number = 389;
const CRYPTO_NUMBER_397: number = 397;
const CRYPTO_NUMBER_401: number = 401;
const CRYPTO_NUMBER_409: number = 409;
const CRYPTO_NUMBER_419: number = 419;
const CRYPTO_NUMBER_421: number = 421;
const CRYPTO_NUMBER_431: number = 431;
const CRYPTO_NUMBER_433: number = 433;
const CRYPTO_NUMBER_439: number = 439;
const CRYPTO_NUMBER_443: number = 443;
const CRYPTO_NUMBER_449: number = 449;
const CRYPTO_NUMBER_457: number = 457;
const CRYPTO_NUMBER_461: number = 461;
const CRYPTO_NUMBER_463: number = 463;
const CRYPTO_NUMBER_467: number = 467;
const CRYPTO_NUMBER_479: number = 479;
const CRYPTO_NUMBER_487: number = 487;
const CRYPTO_NUMBER_491: number = 491;
const CRYPTO_NUMBER_499: number = 499;
const CRYPTO_NUMBER_503: number = 503;
const CRYPTO_NUMBER_509: number = 509;
const CRYPTO_NUMBER_26: number = 26;
const CRYPTO_NUMBER_0X4000000: number = 0x4000000;
const CRYPTO_NUMBER_0X3FFFFFF: number = 0x3ffffff;
const CRYPTO_NUMBER_0X7FFF: number = 0x7fff;
const CRYPTO_NUMBER_15: number = 15;
const CRYPTO_NUMBER_0X3FFFFFFF: number = 0x3fffffff;
const CRYPTO_NUMBER_30: number = 30;
const CRYPTO_NUMBER_0X3FFF: number = 0x3fff;
const CRYPTO_NUMBER_14: number = 14;
const CRYPTO_NUMBER_28: number = 28;
const CRYPTO_NUMBER_0XFFFFFFF: number = 0xfffffff;
const CRYPTO_NUMBER_0X1FFF: number = 0x1fff;
const CRYPTO_NUMBER_52: number = 52;
const CRYPTO_NUMBER_9: number = 9;
const CRYPTO_NUMBER_36: number = 36;
const CRYPTO_NUMBER_256: number = 256;
const CRYPTO_NUMBER_16: number = 16;
const CRYPTO_NUMBER_4: number = 4;
const CRYPTO_NUMBER_8: number = 8;
const CRYPTO_NUMBER_32: number = 32;
const CRYPTO_NUMBER_0XFF: number = 0xff;
const CRYPTO_NUMBER_0X80: number = 0x80;
const CRYPTO_NUMBER_0XF: number = 0xf;
const CRYPTO_NUMBER_0XFFFF: number = 0xffff;
const CRYPTO_NUMBER_0XFFFFFFFF: number = 0xffffffff;
const CRYPTO_NUMBER_24: number = 24;
const CRYPTO_NUMBER_10: number = 10;
const CRYPTO_NUMBER_18: number = 18;
const CRYPTO_NUMBER_48: number = 48;
const CRYPTO_NUMBER_144: number = 144;
const CRYPTO_NUMBER_768: number = 768;
const CRYPTO_NUMBER_6: number = 6;
const CRYPTO_NUMBER_255: number = 255;
const CRYPTO_NUMBER_1122926989487: number = 1122926989487;
const CRYPTO_NUMBER_65536: number = 65536;
const CRYPTO_NUMBER_0X10: number = 0x10;
const CRYPTO_NUMBER_1000: number = 1000;
const CRYPTO_RUNCOUNT_120: number = 120;

// Bits per digit
let dbits: number;
let bIDB: number;
let bIDM: number;
let bIDV: number;

let bIFP: number;
let bIFV: number;
let bIF1: number;
let bIF2: number;

let dv: number = 0;

// JavaScript engine analysis
let canary: number = 0xdeadbeefcafe;
let jLm: boolean = (canary & CRYPTO_NUMBER_0XFFFFFF) === CRYPTO_NUMBER_0XEFCAFE;

let lowprimes = [
  CRYPTO_NUMBER_2, CRYPTO_NUMBER_3, CRYPTO_NUMBER_5, CRYPTO_NUMBER_7, CRYPTO_NUMBER_11, CRYPTO_NUMBER_13, CRYPTO_NUMBER_17, CRYPTO_NUMBER_19,
  CRYPTO_NUMBER_23, CRYPTO_NUMBER_29, CRYPTO_NUMBER_31, CRYPTO_NUMBER_37, CRYPTO_NUMBER_41, CRYPTO_NUMBER_43, CRYPTO_NUMBER_47, CRYPTO_NUMBER_53,
  CRYPTO_NUMBER_59, CRYPTO_NUMBER_61, CRYPTO_NUMBER_67, CRYPTO_NUMBER_71, CRYPTO_NUMBER_73, CRYPTO_NUMBER_79, CRYPTO_NUMBER_83, CRYPTO_NUMBER_89,
  CRYPTO_NUMBER_97, CRYPTO_NUMBER_101, CRYPTO_NUMBER_103, CRYPTO_NUMBER_107, CRYPTO_NUMBER_109, CRYPTO_NUMBER_113, CRYPTO_NUMBER_127, CRYPTO_NUMBER_131,
  CRYPTO_NUMBER_137, CRYPTO_NUMBER_139, CRYPTO_NUMBER_149, CRYPTO_NUMBER_151, CRYPTO_NUMBER_157, CRYPTO_NUMBER_163, CRYPTO_NUMBER_167, CRYPTO_NUMBER_173,
  CRYPTO_NUMBER_179, CRYPTO_NUMBER_181, CRYPTO_NUMBER_191, CRYPTO_NUMBER_193, CRYPTO_NUMBER_197, CRYPTO_NUMBER_199, CRYPTO_NUMBER_211, CRYPTO_NUMBER_223,
  CRYPTO_NUMBER_227, CRYPTO_NUMBER_229, CRYPTO_NUMBER_233, CRYPTO_NUMBER_239, CRYPTO_NUMBER_241, CRYPTO_NUMBER_251, CRYPTO_NUMBER_257, CRYPTO_NUMBER_263,
  CRYPTO_NUMBER_269, CRYPTO_NUMBER_271, CRYPTO_NUMBER_277, CRYPTO_NUMBER_281, CRYPTO_NUMBER_283, CRYPTO_NUMBER_293, CRYPTO_NUMBER_307, CRYPTO_NUMBER_311,
  CRYPTO_NUMBER_313, CRYPTO_NUMBER_317, CRYPTO_NUMBER_331, CRYPTO_NUMBER_337, CRYPTO_NUMBER_347, CRYPTO_NUMBER_349, CRYPTO_NUMBER_353, CRYPTO_NUMBER_359,
  CRYPTO_NUMBER_367, CRYPTO_NUMBER_373, CRYPTO_NUMBER_379, CRYPTO_NUMBER_383, CRYPTO_NUMBER_389, CRYPTO_NUMBER_397, CRYPTO_NUMBER_401, CRYPTO_NUMBER_409,
  CRYPTO_NUMBER_419, CRYPTO_NUMBER_421, CRYPTO_NUMBER_431, CRYPTO_NUMBER_433, CRYPTO_NUMBER_439, CRYPTO_NUMBER_443, CRYPTO_NUMBER_449, CRYPTO_NUMBER_457,
  CRYPTO_NUMBER_461, CRYPTO_NUMBER_463, CRYPTO_NUMBER_467, CRYPTO_NUMBER_479, CRYPTO_NUMBER_487, CRYPTO_NUMBER_491, CRYPTO_NUMBER_499, CRYPTO_NUMBER_503,
  CRYPTO_NUMBER_509
];
let lplim = (1 << CRYPTO_NUMBER_26) / lowprimes[lowprimes.length - 1];

// am: Compute w_j += (x*this_i), propagate carries,
// c is initial carry, returns final carry.
// c < 3*dvalue, x < 2*dvalue, this_i < dvalue
// We need to select the fastest one that works in this environment.

// am1: use a single mult and divide to get the high bits,
// max digit bits should be 26 because
// max internal value = 2*dvalue^2-2*dvalue (< 2^53)
let am1 = (array: (number | null)[], i: number, x: number, w: BigInteger, j: number, c: number, n: number): number => {
  let tempI = i;
  let tempJ = j;
  let tempC = c;
  let tempN = n - 1;
  while (tempN >= 0) {
    let v = x * array[tempI]! + w.array[tempJ]! + tempC;
    tempC = Math.floor(v / CRYPTO_NUMBER_0X4000000);
    w.array = insertValue(w.array, v & CRYPTO_NUMBER_0X3FFFFFF, tempJ);
    tempI += 1;
    tempJ += 1;
    tempN -= 1;
  }
  return tempC;
};

// am2 avoids a big mult-and-extract completely.
// Max digit bits should be <= 30 because we do bitwise ops
// on values up to 2*hdvalue^2-hdvalue-1 (< 2^31)
let am2 = (array: (number | null)[], i: number, x: number, w: BigInteger, j: number, c: number, n: number): number => {
  let xl = x & CRYPTO_NUMBER_0X7FFF;
  let xh = x >> CRYPTO_NUMBER_15;
  let tempI = i;
  let tempJ = j;
  let tempC = c;
  let tempN = n - 1;
  while (tempN >= 0) {
    let l = array[tempI]! & CRYPTO_NUMBER_0X7FFF;
    let h = array[tempI]! >> CRYPTO_NUMBER_15;
    let m = xh * l + h * xl;
    l = xl * l + ((m & CRYPTO_NUMBER_0X7FFF) << CRYPTO_NUMBER_15) + w.array[tempJ]! + (tempC & CRYPTO_NUMBER_0X3FFFFFFF);
    tempC = (l >>> CRYPTO_NUMBER_30) + (m >>> CRYPTO_NUMBER_15) + xh * h + (tempC >>> CRYPTO_NUMBER_30);
    w.array = insertValue(w.array, l & CRYPTO_NUMBER_0X3FFFFFFF, tempJ);
    tempI += 1;
    tempJ += 1;
    tempN -= 1;
  }
  return tempC;
};

// Alternately, set max digit bits to 28 since some
// browsers slow down when dealing with 32-bit numbers.
let am3 = (array: (number | null)[], i: number, x: number, w: BigInteger, j: number, c: number, n: number): number => {
  let xl = x & CRYPTO_NUMBER_0X3FFF;
  let xh = x >> CRYPTO_NUMBER_14;
  let tempI = i;
  let tempJ = j;
  let tempC = c;
  let tempN = n - 1;
  while (tempN >= 0) {
    let l = array[tempI]! & CRYPTO_NUMBER_0X3FFF;
    let h = array[tempI]! >> CRYPTO_NUMBER_14;
    let m = xh * l + h * xl;
    l = xl * l + ((m & CRYPTO_NUMBER_0X3FFF) << CRYPTO_NUMBER_14) + w.array[tempJ]! + tempC;
    tempC = (l >> CRYPTO_NUMBER_28) + (m >> CRYPTO_NUMBER_14) + xh * h;
    w.array = insertValue(w.array, l & CRYPTO_NUMBER_0XFFFFFFF, tempJ);
    tempI += 1;
    tempJ += 1;
    tempN -= 1;
  }
  return tempC;
};

// This is tailored to VMs with 2-bit tagging. It makes sure
// that all the computations stay within the 29 bits available.
let am4 = (array: (number | null)[], i: number, x: number, w: BigInteger, j: number, c: number, n: number): number => {
  let xl = x & CRYPTO_NUMBER_0X1FFF;
  let xh = x >> CRYPTO_NUMBER_13;
  let tempI = i;
  let tempJ = j;
  let tempC = c;
  let tempN = n - 1;
  while (tempN >= 0) {
    let l = array[tempI]! & CRYPTO_NUMBER_0X1FFF;
    let h = array[tempI]! >> CRYPTO_NUMBER_13;
    let m = xh * l + h * xl;
    l = xl * l + ((m & CRYPTO_NUMBER_0X1FFF) << CRYPTO_NUMBER_13) + w.array[tempJ]! + tempC;
    tempC = (l >> CRYPTO_NUMBER_26) + (m >> CRYPTO_NUMBER_13) + xh * h;
    w.array = insertValue(w.array, l & CRYPTO_NUMBER_0X3FFFFFF, tempJ);
    tempI += 1;
    tempJ += 1;
    tempN -= 1;
  }
  return tempC;
};

let aM: (array: (number | null)[], i: number, x: number, w: BigInteger, j: number, c: number, n: number) => number;

// am3/28 is best for SM, Rhino, but am4/26 is best for v8.
// Kestrel (Opera 9.5) gets its best result with am4/26.
// IE7 does 9% better with am3/28 than with am4/26.
// Firefox (SM) gets 10% faster with am3/28 than with am4/26.
function setupEngine(fn: (array: (number | null)[], i: number, x: number, w: BigInteger, j: number, c: number, n: number) => number, bits: number): void {
  aM = fn;
  dbits = bits;
  bIDB = dbits;
  bIDM = (1 << dbits) - 1;
  bIDV = 1 << dbits;

  bIFP = CRYPTO_NUMBER_52;
  bIFV = Math.pow(CRYPTO_NUMBER_2, bIFP);
  bIF1 = bIFP - dbits;
  bIF2 = CRYPTO_NUMBER_2 * dbits - bIFP;
}

// Digit conversions
let bIRM: string = '0123456789abcdefghijklmnopqrstuvwxyz';
let bIRC: (number | null)[] = [];

function initBIRC(): void {
  let rr: number;
  rr = '0'.charCodeAt(0);
  for (let vv = 0; vv <= CRYPTO_NUMBER_9; ++vv) {
    bIRC = insertValue(bIRC, vv, rr);
    rr += 1;
  }
  rr = 'a'.charCodeAt(0);
  for (let vv = CRYPTO_NUMBER_10; vv < CRYPTO_NUMBER_36; ++vv) {
    bIRC = insertValue(bIRC, vv, rr);
    rr += 1;
  }
  rr = 'A'.charCodeAt(0);
  for (let vv = CRYPTO_NUMBER_10; vv < CRYPTO_NUMBER_36; ++vv) {
    bIRC = insertValue(bIRC, vv, rr);
    rr += 1;
  }
}

class BigInteger {
  static one: BigInteger = BigInteger.nbv(1);
  static zero: BigInteger = BigInteger.nbv(0);
  array: (number | null)[] = [];
  t: number = 0;
  s: number = 0;

  // (public) Constructor
  constructor(a?: number | string | (number | null)[], b?: SecureRandom | number, c?: SecureRandom | number) {
    if (a != null) {
      if ('number' === typeof a) {
        this.fromNumber(a, b, c);
      } else if (b == null && 'string' !== typeof a) {
        this.fromString(a as string | number[], CRYPTO_NUMBER_256);
      } else {
        this.fromString(a as string | number[], b as number);
      }
    }
  }

  // return new, unset BigInteger
  static nbi(): BigInteger {
    return new BigInteger();
  }

  // return bigint initialized to value
  static nbv(i: number): BigInteger {
    let r = BigInteger.nbi();
    r.fromInt(i);
    return r;
  }

  int2char(n: number): string {
    return bIRM.charAt(n);
  }

  intAt(s: string, i: number): number {
    let c = bIRC[s.charCodeAt(i)];
    return c === null ? -1 : c;
  }

  // (protected) copy this to r
  copyTo(r: BigInteger): void {
    let i = this.t - 1;
    while (i >= 0) {
      r.array = insertValue(r.array, this.array[i]!, i);
      i -= 1;
    }
    r.t = this.t;
    r.s = this.s;
  }

  // (protected) set from integer value x, -DV <= x < DV
  fromInt(x: number): void {
    this.t = 1;
    this.s = x < 0 ? -1 : 0;
    if (x > 0) {
      this.array = insertValue(this.array, x, 0);
    } else if (x < -1) {
      this.array = insertValue(this.array, x + dv, 0);
    } else {
      this.t = 0;
    }
  }

  // (protected) set from string and radix
  fromString(s: string | (number | null)[], b: number): void {
    let k: number;
    if (b === CRYPTO_NUMBER_16) {
      k = CRYPTO_NUMBER_4;
    } else if (b === CRYPTO_NUMBER_8) {
      k = CRYPTO_NUMBER_3;
    } else if (b === CRYPTO_NUMBER_256) {
      k = CRYPTO_NUMBER_8; // byte array
    } else if (b === CRYPTO_NUMBER_2) {
      k = 1;
    } else if (b === CRYPTO_NUMBER_32) {
      k = CRYPTO_NUMBER_5;
    } else if (b === CRYPTO_NUMBER_4) {
      k = CRYPTO_NUMBER_2;
    } else {
      this.fromRadix(s as string, b);
      return;
    }
    this.t = 0;
    this.s = 0;
    let i: number;
    let mi: Boolean = false;
    let sh: number = 0;
    if (k === CRYPTO_NUMBER_8) {
      let tempS = s as number[];
      i = tempS.length;
    } else {
      i = (s as String).length;
    }
    i -= 1;
    while (i >= 0) {
      let x: number;
      if (k === CRYPTO_NUMBER_8) {
        x = (s as number[])[i] & CRYPTO_NUMBER_0XFF;
      } else {
        x = this.intAt(s as string, i);
      }
      if (x < 0) {
        if ((s as string)?.charAt(i) === '-') {
          mi = true;
        }
        continue;
      }
      mi = false;
      if (sh === 0) {
        this.array = insertValue(this.array, x, this.t);
        this.t += 1;
      } else if (sh + k > bIDB) {
        this.array = insertValue(this.array, this.array[this.t - 1]! | ((x & ((1 << (bIDB - sh)) - 1)) << sh), this.t - 1);
        this.array = insertValue(this.array, x >> (bIDB - sh), this.t);
        this.t += 1;
      } else {
        this.array = insertValue(this.array, this.array[this.t - 1]! | (x << sh), this.t - 1);
      }
      sh += k;
      if (sh >= bIDB) {
        sh -= bIDB;
      }
      i -= 1;
    }
    if (k === CRYPTO_NUMBER_8 && ((s as number[])[0] & CRYPTO_NUMBER_0X80) !== 0) {
      this.s = -1;
      if (sh > 0) {
        (this.array[this.t - 1] as number) |= ((1 << (bIDB - sh)) - 1) << sh;
      }
    }
    this.clamp();
    if (mi) {
      BigInteger.zero.subTo(this, this);
    }
  }
  
  // (protected) clamp off excess high words
  clamp(): void {
    let c = this.s & bIDM;
    while (this.t > 0 && this.array[this.t - 1] === c) {
      this.t -= 1;
    }
  }

  // (public) return string representation in given radix
  toString(b: number): string {
    if (this.s < 0) {
      return '-' + this.negate().toString(b);
    }
    let k: number;
    if (b === CRYPTO_NUMBER_16) {
      k = CRYPTO_NUMBER_4;
    } else if (b === CRYPTO_NUMBER_8) {
      k = CRYPTO_NUMBER_3;
    } else if (b === CRYPTO_NUMBER_2) {
      k = 1;
    } else if (b === CRYPTO_NUMBER_32) {
      k = CRYPTO_NUMBER_5;
    } else if (b === CRYPTO_NUMBER_4) {
      k = CRYPTO_NUMBER_2;
    } else {
      return this.toRadix(b);
    }
    let km = (1 << k) - 1;
    let d: number | null;
    let m = false;
    let r = '';
    let i = this.t;
    let p = bIDB - ((i * bIDB) % k);
    if (i > 0) {
      i -= 1;
      d = this.array[i]! >> p;
      if (p < bIDB && d > 0) {
        m = true;
        r = this.int2char(d);
      }
      while (i >= 0) {
        if (p < k) {
          d = (this.array[i]! & ((1 << p) - 1)) << (k - p);
          i -= 1;
          p += bIDB - k;
          d |= this.array[i]! >> p;
        } else {
          p -= k;
          d = (this.array[i]! >> p) & km;
          if (p <= 0) {
            p += bIDB;
            i -= 1;
          }
        }
        if (d > 0) {
          m = true;
        }
        if (m) {
          r += this.int2char(d);
        }
      }
    }
    return m ? r : '0';
  }

  // (public) -this
  negate(): BigInteger {
    let r = BigInteger.nbi();
    BigInteger.zero.subTo(this, r);
    return r;
  }

  // (public) |this|
  abs(): BigInteger {
    return this.s < 0 ? this.negate() : this;
  }

  // (public) return + if this > a, - if this < a, 0 if equal
  compareTo(a: BigInteger): number {
    let r = this.s - a.s;
    if (r !== 0) {
      return r;
    }
    let i = this.t;
    r = i - a.t;
    if (r !== 0) {
      return r;
    }
    i -= 1;
    while (i >= 0) {
      r = this.array[i]! - a.array[i]!;
      if (r !== 0) {
        return r;
      }
      i -= 1;
    }
    return 0;
  }

  // returns bit length of the integer x
  nbits(x: number): number {
    let tempX = x;
    let r = 1;
    let t: number = tempX >>> CRYPTO_NUMBER_16;
    if (t !== 0) {
      tempX = t;
      r += CRYPTO_NUMBER_16;
    }
    t = tempX >> CRYPTO_NUMBER_8;
    if (t !== 0) {
      tempX = t;
      r += CRYPTO_NUMBER_8;
    }
    t = tempX >> CRYPTO_NUMBER_4;
    if (t !== 0) {
      tempX = t;
      r += CRYPTO_NUMBER_4;
    }
    t = tempX >> CRYPTO_NUMBER_2;
    if (t !== 0) {
      tempX = t;
      r += CRYPTO_NUMBER_2;
    }
    t = tempX >> 1;
    if (t !== 0) {
      tempX = t;
      r += 1;
    }
    return r;
  }

  // (public) return the number of bits in "this"
  bitLength(): number {
    if (this.t <= 0) {
      return 0;
    }
    return bIDB * (this.t - 1) + this.nbits(this.array[this.t - 1]! ^ (this.s & bIDM));
  }

  // (protected) r = this << n*DB
  dlShiftTo(n: number, r: BigInteger): void {
    let i = this.t - 1;
    while (i >= 0) {
      r.array = insertValue(r.array, this.array[i]!, i + n);
      i -= 1;
    }
    i = n - 1;
    while (i >= 0) {
      r.array = insertValue(r.array, 0, i);
      i -= 1;
    }
    r.t = this.t + n;
    r.s = this.s;
  }

  // (protected) r = this >> n*DB
  drShiftTo(n: number, r: BigInteger): void {
    let i = n;
    while (i < this.t) {
      r.array = insertValue(r.array, this.array[i]!, i - n);
      i += 1;
    }
    r.t = Math.max(this.t - n, 0);
    r.s = this.s;
  }

  // (protected) r = this << n
  lShiftTo(n: number, r: BigInteger): void {
    let bs = n % bIDB;
    let cbs = bIDB - bs;
    let bm = (1 << cbs) - 1;
    let ds = Math.floor(n / bIDB);
    let c = (this.s << bs) & bIDM;
    let i = this.t - 1;
    while (i >= 0) {
      r.array = insertValue(r.array, (this.array[i]! >> cbs) | c, i + ds + 1);
      c = (this.array[i]! & bm) << bs;
      i -= 1;
    }
    i = ds - 1;
    while (i >= 0) {
      r.array = insertValue(r.array, 0, i);
      i -= 1;
    }
    r.array = insertValue(r.array, c, ds);
    r.t = this.t + ds + 1;
    r.s = this.s;
    r.clamp();
  }

  // (protected) r = this >> n
  rShiftTo(n: number, r: BigInteger): void {
    r.s = this.s;
    let ds = Math.floor(n / bIDB);
    if (ds >= this.t) {
      r.t = 0;
      return;
    }
    let bs = n % bIDB;
    let cbs = bIDB - bs;
    let bm = (1 << bs) - 1;
    r.array = insertValue(r.array, this.array[ds]! >> bs, 0);
    for (let i = ds + 1; i < this.t; ++i) {
      r.array = insertValue(r.array, r.array[i - ds - 1]! | ((this.array[i]! & bm) << cbs), i - ds - 1);
      r.array = insertValue(r.array, this.array[i]! >> bs, i - ds);
    }
    if (bs > 0) {
      r.array = insertValue(r.array, r.array[this.t - ds - 1]! | ((this.s & bm) << cbs), this.t - ds - 1);
    }
    r.t = this.t - ds;
    r.clamp();
  }

  // (protected) r = this - a
  subTo(a: BigInteger, r: BigInteger): void {
    let i = 0;
    let c = 0;
    let m = Math.min(a.t, this.t);
    while (i < m) {
      c += this.array[i]! - a.array[i]!;
      r.array = insertValue(r.array, c & bIDM, i);
      c >>= bIDB;
      i += 1;
    }
    if (a.t < this.t) {
      c -= a.s;
      while (i < this.t) {
        c += this.array[i]!;
        r.array = insertValue(r.array, c & bIDM, i);
        c >>= bIDB;
        i += 1;
      }
      c += this.s;
    } else {
      c += this.s;
      while (i < a.t) {
        c -= a.array[i]!;
        r.array = insertValue(r.array, c & bIDM, i);
        c >>= bIDB;
        i += 1;
      }
      c -= a.s;
    }
    r.s = c < 0 ? -1 : 0;
    if (c < -1) {
      r.array = insertValue(r.array, bIDV + c, i);
      i += 1;
    } else if (c > 0) {
      r.array = insertValue(r.array, c, i);
      i += 1;
    }
    r.t = i;
    r.clamp();
  }

  // (protected) r = this * a, r !== this,a (HAC 14.12)
  // "this" should be the larger one if appropriate.
  multiplyTo(a: BigInteger, r: BigInteger): void {
    let x = this.abs();
    let y = a.abs();
    let i = x.t;
    r.t = i + y.t;
    i -= 1;
    while (i >= 0) {
      r.array = insertValue(r.array, 0, i);
      i -= 1;
    }
    for (i = 0; i < y.t; ++i) {
      let value = x.am(0, y.array[i]!, r, i, 0, x.t);
      r.array = insertValue(r.array, value, i + x.t);
    }
    r.s = 0;
    r.clamp();
    if (this.s !== a.s) {
      BigInteger.zero.subTo(r, r);
    }
  }

  // (protected) r = this^2, r !== this (HAC 14.16)
  squareTo(r: BigInteger): void {
    let x = this.abs();
    r.t = CRYPTO_NUMBER_2 * x.t;
    let i = r.t - 1;
    while (i >= 0) {
      r.array = insertValue(r.array, 0, i);
      i -= 1;
    }
    i = 0;
    while (i < x.t - 1) {
      let c = x.am(i, x.array[i]!, r, CRYPTO_NUMBER_2 * i, 0, 1);
      let value = r.array[i + x.t]! + x.am(i + 1, CRYPTO_NUMBER_2 * x.array[i]!, r, CRYPTO_NUMBER_2 * i + 1, c, x.t - i - 1);
      r.array = insertValue(r.array, value, i + x.t);
      if (r.array[i + x.t]! >= bIDV) {
        r.array = insertValue(r.array, r.array[i + x.t]! - bIDV, i + x.t);
        r.array = insertValue(r.array, 1, i + x.t + 1);
      }
      i += 1;
    }
    if (r.t > 0) {
      let value = r.array[r.t - 1]! + x.am(i, x.array[i]!, r, CRYPTO_NUMBER_2 * i, 0, 1);
      r.array = insertValue(r.array, value, r.t - 1);
    }
    r.s = 0;
    r.clamp();
  }

  // (protected) divide this by m, quotient and remainder to q, r (HAC 14.20)
  // r !== q, this !== m.  q or r may be null.
  divRemTo(m: BigInteger, q: BigInteger | null, r: BigInteger | null): void {
    let pm: BigInteger = m.abs();
    if (pm.t <= 0) {
      return;
    }
    let pt = this.abs();
    if (pt.t < pm.t) {
      if (q !== null) {
        q.fromInt(0);
      }
      if (r !== null) {
        this.copyTo(r);
      }
      return;
    }
    if (r === null) {
      r = BigInteger.nbi();
    }
    let y: BigInteger = BigInteger.nbi();
    let ts = this.s;
    let ms: number = m.s;
    let nsh = bIDB - this.nbits(pm.array[pm.t - 1]!); // normalize modulus
    if (nsh > 0) {
      pm.lShiftTo(nsh, y);
      pt.lShiftTo(nsh, r);
    } else {
      pm.copyTo(y);
      pt.copyTo(r);
    }
    let ys: number = y.t;
    let y0: number = y.array[ys - 1]!;
    if (y0 === 0) {
      return;
    }
    let yt = y0 * (1 << bIF1) + (ys > 1 ? y.array[ys - CRYPTO_NUMBER_2]! >> bIF2 : 0);
    let d1 = bIFV / yt;
    let d2 = (1 << bIF1) / yt;
    let e = 1 << bIF2;
    let i: number = r.t;
    let j = i - ys;
    let t: BigInteger = q === null ? BigInteger.nbi() : q;
    y.dlShiftTo(j, t);
    if (r.compareTo(t) >= 0) {
      r.array = insertValue(r.array, 1, r.t);
      r.t += 1;
      r.subTo(t, r);
    }
    BigInteger.one.dlShiftTo(ys, t);
    t.subTo(y, y); // "negative" y so we can replace sub with am later
    while (y.t < ys) {
      y.array = insertValue(y.array, 0, y.t);
      y.t += 1;
    }
    j -= 1;
    while (j >= 0) {
      i -= 1; // Estimate quotient digit
      let qd = r.array[i]! === y0 ? bIDM : Math.floor(r.array[i]! * d1 + (r.array[i - 1]! + e) * d2);
      let value = r.array[i]! + y.am(0, qd, r, j, 0, ys);
      r.array = insertValue(r.array, value, i);
      if (r.array[i]! < qd) {
        y.dlShiftTo(j, t); // Try it out
        r.subTo(t, r);
        qd -= 1;
        while (r.array[i]! < qd) {
          r.subTo(t, r);
          qd -= 1;
        }
      }
      j -= 1;
    }
    if (q !== null) {
      r.drShiftTo(ys, q);
      if (ts !== ms) {
        BigInteger.zero.subTo(q, q);
      }
    }
    r.t = ys;
    r.clamp();
    if (nsh > 0) {
      r.rShiftTo(nsh, r);
    } // Denormalize remainder
    if (ts < 0) {
      BigInteger.zero.subTo(r, r);
    }
  }

  // (public) this mod a
  mod(a: BigInteger): BigInteger {
    let r: BigInteger | null = BigInteger.nbi();
    this.abs().divRemTo(a, null, r);
    if (this.s < 0 && r.compareTo(BigInteger.zero) > 0) {
      a.subTo(r, r);
    }
    return r;
  }

  invDigit(): number {
    if (this.t < 1) {
      return 0;
    }
    let x = this.array[0]!;
    if ((x & 1) === 0) {
      return 0;
    }
    let y = x & CRYPTO_NUMBER_3; // y === 1/x mod 2^2
    y = (y * (CRYPTO_NUMBER_2 - (x & CRYPTO_NUMBER_0XF) * y)) & CRYPTO_NUMBER_0XF; // y === 1/x mod 2^4
    y = (y * (CRYPTO_NUMBER_2 - (x & CRYPTO_NUMBER_0XFF) * y)) & CRYPTO_NUMBER_0XFF; // y === 1/x mod 2^8
    y = (y * (CRYPTO_NUMBER_2 - (((x & CRYPTO_NUMBER_0XFFFF) * y) & CRYPTO_NUMBER_0XFFFF))) & CRYPTO_NUMBER_0XFFFF; // y === 1/x mod 2^16
    // last step - calculate inverse mod DV directly;
    // assumes 16 < DB <= 32 and assumes ability to handle 48-bit ints
    y = (y * (CRYPTO_NUMBER_2 - ((x * y) % bIDV))) % bIDV; // y === 1/x mod 2^dbits
    // we really want the negative inverse, and -DV < y < DV
    return y > 0 ? bIDV - y : -y;
  }

  // (protected) true iff this is even
  isEven(): boolean {
    return (this.t > 0 ? this.array[0]! & 1 : this.s) === 0;
  }

  // (protected) this^e, e < 2^32, doing sqr and mul with "r" (HAC 14.79)
  exp(e: number, z: ConvertProtocol): BigInteger {
    if (e > CRYPTO_NUMBER_0XFFFFFFFF || e < 1) {
      return BigInteger.one;
    }
    let r: BigInteger | null = BigInteger.nbi();
    let r2: BigInteger | null = BigInteger.nbi();
    let g = z.convert(this);
    let i = this.nbits(e) - 1 - 1;
    g.copyTo(r);
    while (i >= 0) {
      z.sqrTo(r as BigInteger, r2);
      if ((e & (1 << i)) > 0) {
        z.mulTo(r2 as BigInteger, g, r);
      } else {
        let t: BigInteger | null = r;
        r = r2;
        r2 = t;
      }
      i -= 1;
    }
    return z.revert(r as BigInteger);
  }

  // (public) this^e % m, 0 <= e < 2^32
  modPowInt(e: number, m: BigInteger): BigInteger {
    let z: ConvertProtocol;
    if (e < CRYPTO_NUMBER_256 || m.isEven()) {
      z = new Classic(m);
    } else {
      z = new Montgomery(m);
    }
    return this.exp(e, z);
  }

  // (public)
  clone(): BigInteger {
    let r = BigInteger.nbi();
    this.copyTo(r);
    return r;
  }

  // (public) return value as integer
  intValue(): number {
    if (this.s < 0) {
      if (this.t === 1) {
        return this.array[0]! - bIDV;
      } else if (this.t === 0) {
        return -1;
      }
    } else if (this.t === 1) {
      return this.array[0]!;
    } else if (this.t === 0) {
      return 0;
    }
    // assumes 16 < DB < 32
    return ((this.array[1]! & ((1 << (CRYPTO_NUMBER_32 - bIDB)) - 1)) << bIDB) | this.array[0]!;
  }

  // (public) return value as byte
  byteValue(): number {
    return this.t === 0 ? this.s : (this.array[0]! << CRYPTO_NUMBER_24) >> CRYPTO_NUMBER_24;
  }

  // (public) return value as short (assumes DB>=16)
  shortValue(): number {
    return this.t === 0 ? this.s : (this.array[0]! << CRYPTO_NUMBER_16) >> CRYPTO_NUMBER_16;
  }

  // (protected) return x s.t. r^x < DV
  chunkSize(r: number): number {
    return Math.floor((Math.LN2 * bIDB) / Math.log(r));
  }

  // (public) 0 if this === 0, 1 if this > 0
  signum(): number {
    if (this.s < 0) {
      return -1;
    } else if (this.t <= 0 || (this.t === 1 && this.array[0]! <= 0)) {
      return 0;
    } else {
      return 1;
    }
  }

  // (protected) convert to radix string
  toRadix(b: number | null): string {
    let tempB = b == null ? CRYPTO_NUMBER_10 : b;
    if (this.signum() === 0 || tempB < CRYPTO_NUMBER_2 || tempB > CRYPTO_NUMBER_36) {
      return '0';
    }
    let cs = this.chunkSize(tempB);
    let a = Math.pow(tempB, cs);
    let d = BigInteger.nbv(a);
    let y = BigInteger.nbi();
    let z: BigInteger | null = BigInteger.nbi();
    let r = '';
    this.divRemTo(d, y, z);
    while (y.signum() > 0) {
      let str = (a + z.intValue()).toString(tempB);
      r = str.slice(1) + r;
      y.divRemTo(d, y, z);
    }
    return z.intValue().toString(tempB) + r;
  }

  // (protected) convert from radix string
  fromRadix(s: string, b: number | null): void {
    this.fromInt(0);
    let tempB = b == null ? CRYPTO_NUMBER_10 : b;
    let cs = this.chunkSize(tempB);
    let d = Math.pow(tempB, cs);
    let mi = false;
    let j = 0;
    let w = 0;
    for (let i = 0; i < s.length; ++i) {
      let x = this.intAt(s, i);
      if (x < 0) {
        if (s.charAt(i) === '-' && this.signum() === 0) {
          mi = true;
        }
        continue;
      }
      w = tempB * w + x;
      j += 1;
      if (j >= cs) {
        this.dMultiply(d);
        this.dAddOffset(w, 0);
        j = 0;
        w = 0;
      }
    }
    if (j > 0) {
      this.dMultiply(Math.pow(tempB, j));
      this.dAddOffset(w, 0);
    }
    if (mi) {
      BigInteger.zero.subTo(this, this);
    }
  }

  // (protected) alternate constructor
  fromNumber(a: number, b?: SecureRandom | number, c?: SecureRandom | number): void {
    if ('number' === typeof b) {
      // new BigInteger(int,int,RNG)
      if (a < CRYPTO_NUMBER_2) {
        this.fromInt(1);
      } else {
        this.fromNumber(a, c);
        if (!this.testBit(a - 1)) {
          // force MSB set
          this.bitwiseTo(BigInteger.one.shiftLeft(a - 1), this.opOr, this);
        }
        if (this.isEven()) {
          this.dAddOffset(1, 0); // force odd
        }
        while (!this.isProbablePrime(b)) {
          this.dAddOffset(CRYPTO_NUMBER_2, 0);
          if (this.bitLength() > a) {
            this.subTo(BigInteger.one.shiftLeft(a - 1), this);
          }
        }
      }
    } else {
      // new BigInteger(int,RNG)
      let x: (number | null)[] = [];
      let t = a & CRYPTO_NUMBER_7;
      x.length = (a >> CRYPTO_NUMBER_3) + 1;
      (b as SecureRandom).nextBytes(x);
      if (t > 0) {
        x = insertValue(x, x[0]! & ((1 << t) - 1), 0);
      } else {
        x = insertValue(x, 0, 0);
      }
      this.fromString(x, CRYPTO_NUMBER_256);
    }
  }

  // (public) convert to bigendian byte array
  toByteArray(): (number | null)[] {
    let i = this.t;
    let r: (number | null)[] = [];
    r = insertValue(r, this.s, 0);
    let p = bIDB - ((i * bIDB) % CRYPTO_NUMBER_8);
    let d: number;
    let k = 0;
    if (i > 0) {
      i -= 1;
      d = this.array[i]! >> p;
      if (p < bIDB && d !== (this.s & bIDM) >> p) {
        r = insertValue(r, d | (this.s << (bIDB - p)), k);
        k += 1;
      }
      while (i >= 0) {
        if (p < CRYPTO_NUMBER_8) {
          d = (this.array[i]! & ((1 << p) - 1)) << (CRYPTO_NUMBER_8 - p);
          i -= 1;
          p += bIDB - CRYPTO_NUMBER_8;
          d |= this.array[i]! >> p;
        } else {
          p -= CRYPTO_NUMBER_8;
          d = (this.array[i]! >> p) & CRYPTO_NUMBER_0XFF;
          if (p <= 0) {
            p += bIDB;
            i -= 1;
          }
        }
        if ((d & CRYPTO_NUMBER_0X80) !== 0) {
          d |= -CRYPTO_NUMBER_256;
        }
        if (k === 0 && (this.s & CRYPTO_NUMBER_0X80) !== (d & CRYPTO_NUMBER_0X80)) {
          k += 1;
        }
        if (k > 0 || d !== this.s) {
          r = insertValue(r, d, k);
          k += 1;
        }
      }
    }
    return r;
  }

  bnEquals(a: BigInteger): boolean {
    return this.compareTo(a) === 0;
  }

  bnMin(a: BigInteger): BigInteger {
    return this.compareTo(a) < 0 ? this : a;
  }

  bnMax(a: BigInteger): BigInteger {
    return this.compareTo(a) > 0 ? this : a;
  }

  // (protected) r = this op a (bitwise)
  bitwiseTo(a: BigInteger, op: (x: number, y: number) => number, r: BigInteger): void {
    let f: number;
    let m = Math.min(a.t, this.t);
    for (let i = 0; i < m; ++i) {
      r.array = insertValue(r.array, op(this.array[i]!, a.array[i]!), i);
    }
    if (a.t < this.t) {
      f = a.s & bIDM;
      for (let i = m; i < this.t; ++i) {
        r.array = insertValue(r.array, op(this.array[i]!, f), i);
      }
      r.t = this.t;
    } else {
      f = this.s & bIDM;
      for (let i = m; i < a.t; ++i) {
        r.array = insertValue(r.array, op(f, a.array[i]!), i);
      }
      r.t = a.t;
    }
    r.s = op(this.s, a.s);
    r.clamp();
  }

  // (public) this & a
  opAnd = (x: number, y: number): number => {
    return x & y;
  };
  bnAnd(a: BigInteger): BigInteger {
    let r = BigInteger.nbi();
    this.bitwiseTo(a, this.opAnd, r);
    return r;
  }

  // (public) this | a
  opOr = (x: number, y: number): number => {
    return x | y;
  };
  bnOr(a: BigInteger): BigInteger {
    let r = BigInteger.nbi();
    this.bitwiseTo(a, this.opOr, r);
    return r;
  }

  // (public) this ^ a
  opXor = (x: number, y: number): number => {
    return x ^ y;
  };
  bnXor(a: BigInteger): BigInteger {
    let r = BigInteger.nbi();
    this.bitwiseTo(a, this.opXor, r);
    return r;
  }

  // (public) this & ~a
  opAndnot = (x: number, y: number): number => {
    return x & ~y;
  };
  bnAndNot(a: BigInteger): BigInteger {
    let r = BigInteger.nbi();
    this.bitwiseTo(a, this.opAndnot, r);
    return r;
  }

  // (public) ~this
  bnNot(): BigInteger {
    let r = BigInteger.nbi();
    for (let i = 0; i < this.t; ++i) {
      r.array = insertValue(r.array, bIDM & ~this.array[i]!, i);
    }
    r.t = this.t;
    r.s = ~this.s;
    return r;
  }

  // (public) this << n
  shiftLeft(n: number): BigInteger {
    let r = BigInteger.nbi();
    if (n < 0) {
      this.rShiftTo(-n, r);
    } else {
      this.lShiftTo(n, r);
    }
    return r;
  }

  // (public) this >> n
  shiftRight(n: number): BigInteger {
    let r = BigInteger.nbi();
    if (n < 0) {
      this.lShiftTo(-n, r);
    } else {
      this.rShiftTo(n, r);
    }
    return r;
  }

  // return index of lowest 1-bit in x, x < 2^31
  lbit(x: number): number {
    let tempX = x;
    if (tempX === 0) {
      return -1;
    }
    let r = 0;
    if ((tempX & CRYPTO_NUMBER_0XFFFF) === 0) {
      tempX >>= CRYPTO_NUMBER_16;
      r += CRYPTO_NUMBER_16;
    }
    if ((tempX & CRYPTO_NUMBER_0XFF) === 0) {
      tempX >>= CRYPTO_NUMBER_8;
      r += CRYPTO_NUMBER_8;
    }
    if ((tempX & CRYPTO_NUMBER_0XF) === 0) {
      tempX >>= CRYPTO_NUMBER_4;
      r += CRYPTO_NUMBER_4;
    }
    if ((tempX & CRYPTO_NUMBER_3) === 0) {
      tempX >>= CRYPTO_NUMBER_2;
      r += CRYPTO_NUMBER_2;
    }
    if ((tempX & 1) === 0) {
      r += 1;
    }
    return r;
  }

  // (public) returns index of lowest 1-bit (or -1 if none)
  getLowestSetBit(): number {
    for (let i = 0; i < this.t; ++i) {
      if (this.array[i] !== 0) {
        return i * bIDB + this.lbit(this.array[i]!);
      }
    }
    if (this.s < 0) {
      return this.t * bIDB;
    }
    return -1;
  }

  // return number of 1 bits in x
  cbit(x: number): number {
    let r = 0;
    let tepmX = x;
    while (tepmX !== 0) {
      tepmX &= tepmX - 1;
      r += 1;
    }
    return r;
  }

  // (public) return number of set bits
  bitCount(): number {
    let r = 0;
    let x = this.s & bIDM;
    for (let i = 0; i < this.t; ++i) {
      r += this.cbit(this.array[i]! ^ x);
    }
    return r;
  }

  // (public) true iff nth bit is set
  testBit(n: number): boolean {
    let j = Math.floor(n / bIDB);
    if (j >= this.t) {
      return this.s !== 0;
    }
    return (this.array[j]! & (1 << n % bIDB)) !== 0;
  }

  // (protected) this op (1<<n)
  changeBit(n: number, op: (x: number, y: number) => number): BigInteger {
    let r = BigInteger.one.shiftLeft(n);
    this.bitwiseTo(r, op, r);
    return r;
  }

  // (public) this | (1<<n)
  setBit(n: number): BigInteger {
    return this.changeBit(n, this.opOr);
  }

  // (public) this & ~(1<<n)
  clearBit(n: number): BigInteger {
    return this.changeBit(n, this.opAndnot);
  }

  // (public) this ^ (1<<n)
  flipBit(n: number): BigInteger {
    return this.changeBit(n, this.opXor);
  }

  // (protected) r = this + a
  addTo(a: BigInteger, r: BigInteger): void {
    let i = 0;
    let c = 0;
    let m = Math.min(a.t, this.t);
    while (i < m) {
      c += this.array[i]! + a.array[i]!;
      r.array = insertValue(r.array, c & bIDM, i);
      c >>= bIDB;
      i += 1;
    }
    if (a.t < this.t) {
      c += a.s;
      while (i < this.t) {
        c += this.array[i]!;
        r.array = insertValue(r.array, c & bIDM, i);
        c >>= bIDB;
        i += 1;
      }
      c += this.s;
    } else {
      c += this.s;
      while (i < a.t) {
        c += a.array[i]!;
        r.array = insertValue(r.array, c & bIDM, i);
        c >>= bIDB;
        i += 1;
      }
      c += a.s;
    }
    r.s = c < 0 ? -1 : 0;
    if (c > 0) {
      r.array = insertValue(r.array, c, i);
      i += 1;
    } else if (c < -1) {
      r.array = insertValue(r.array, bIDV + c, i);
      i += 1;
    }
    r.t = i;
    r.clamp();
  }

  // (public) this + a
  add(a: BigInteger): BigInteger {
    let r = BigInteger.nbi();
    this.addTo(a, r);
    return r;
  }

  // (public) this - a
  subtract(a: BigInteger): BigInteger {
    let r = BigInteger.nbi();
    this.subTo(a, r);
    return r;
  }

  // (public) this * a
  multiply(a: BigInteger): BigInteger {
    let r = BigInteger.nbi();
    this.multiplyTo(a, r);
    return r;
  }

  // (public) this / a
  divide(a: BigInteger): BigInteger {
    let r = BigInteger.nbi();
    let temp: BigInteger | null = null;
    this.divRemTo(a, r, temp);
    return r;
  }

  // (public) this % a
  remainder(a: BigInteger): BigInteger {
    let r: BigInteger | null = BigInteger.nbi();
    this.divRemTo(a, null, r);
    return r;
  }

  // (public) [this/a,this%a]
  divideAndRemainder(a: BigInteger): BigInteger[] {
    let q = BigInteger.nbi();
    let r: BigInteger | null = BigInteger.nbi();
    this.divRemTo(a, q, r);
    return new Array(q, r);
  }

  // (protected) this *= n, this >= 0, 1 < n < DV
  dMultiply(n: number): void {
    let value = this.am(0, n - 1, this, 0, 0, this.t);
    this.array = insertValue(this.array, value, this.t);
    this.t += 1;
    this.clamp();
  }

  // (protected) this += n << w words, this >= 0
  dAddOffset(n: number, w: number): void {
    let tempW = w;
    while (this.t <= tempW) {
      this.array = insertValue(this.array, 0, this.t);
      this.t += 1;
    }
    this.array = insertValue(this.array, this.array[tempW]! + n, tempW);
    while (this.array[tempW]! >= bIDV) {
      this.array = insertValue(this.array, this.array[tempW]! - bIDV, tempW);
      tempW += 1;
      if (tempW >= this.t) {
        this.array = insertValue(this.array, 0, this.t);
        this.t += 1;
      }
      this.array = insertValue(this.array, this.array[tempW]! + 1, tempW);
    }
  }

  // (public) this^e
  bnPow(e: number): BigInteger {
    return this.exp(e, new NullExp());
  }

  // (protected) r = lower n words of "this * a", a.t <= n
  // "this" should be the larger one if appropriate.
  multiplyLowerTo(a: BigInteger, n: number, r: BigInteger): void {
    let i = Math.min(this.t + a.t, n);
    r.s = 0; // assumes a,this >= 0
    r.t = i;
    while (i > 0) {
      i -= 1;
      r.array = insertValue(r.array, 0, i);
    }
    while (i < r.t - this.t) {
      let value = this.am(0, a.array[i]!, r, i, 0, this.t);
      r.array = insertValue(r.array, value, i + this.t);
      i += 1;
    }
    for (let j = Math.min(a.t, n); i < j; ++i) {
      this.am(0, a.array[i]!, r, i, 0, n - i);
    }
    r.clamp();
  }

  // (protected) r = "this * a" without lower n words, n > 0
  // "this" should be the larger one if appropriate.
  multiplyUpperTo(a: BigInteger, n: number, r: BigInteger): void {
    let tempN = n - 1;
    r.t = this.t + a.t - tempN;
    let i = r.t - 1;
    r.s = 0; // assumes a,this >= 0
    while (i >= 0) {
      r.array = insertValue(r.array, 0, i);
      i -= 1;
    }
    for (i = Math.max(tempN - this.t, 0); i < a.t; ++i) {
      let value = this.am(tempN - i, a.array[i]!, r, 0, 0, this.t + i - tempN);
      r.array = insertValue(r.array, value, this.t + i - tempN);
    }
    r.clamp();
    r.drShiftTo(1, r);
  }

  // (public) this^e % m (HAC 14.85)
  modPow(e: BigInteger, m: BigInteger): BigInteger {
    let i = e.bitLength();
    let k: number;
    let r: BigInteger | null = BigInteger.nbv(1);
    let z: ConvertProtocol;
    if (i <= 0) {
      return r;
    } else if (i < CRYPTO_NUMBER_18) {
      k = 1;
    } else if (i < CRYPTO_NUMBER_48) {
      k = CRYPTO_NUMBER_3;
    } else if (i < CRYPTO_NUMBER_144) {
      k = CRYPTO_NUMBER_4;
    } else if (i < CRYPTO_NUMBER_768) {
      k = CRYPTO_NUMBER_5;
    } else {
      k = CRYPTO_NUMBER_6;
    }
    if (i < CRYPTO_NUMBER_8) {
      z = new Classic(m);
    } else if (m.isEven()) {
      z = new Barrett(m);
    } else {
      z = new Montgomery(m);
    }
    let g: (BigInteger | null)[] = [];
    let n = CRYPTO_NUMBER_3;
    let k1 = k - 1;
    let km = (1 << k) - 1;
    g = insertBigIntegerValue(g, z.convert(this), 1);
    if (k > 1) {
      let g2: BigInteger | null = BigInteger.nbi();
      z.sqrTo(g[1]!, g2);
      while (n <= km) {
        g = insertBigIntegerValue(g, BigInteger.nbi(), n);
        z.mulTo(g2, g[n - CRYPTO_NUMBER_2]!, g[n]!);
        n += CRYPTO_NUMBER_2;
      }
    }
    let j = e.t - 1;
    let w: number;
    let is1: boolean = true;
    let r2: BigInteger | null = BigInteger.nbi();
    let t: BigInteger | null;
    i = this.nbits(e.array[j]!) - 1;
    while (j >= 0) {
      if (i >= k1) {
        w = (e.array[j]! >> (i - k1)) & km;
      } else {
        w = (e.array[j]! & ((1 << (i + 1)) - 1)) << (k1 - i);
        if (j > 0) {
          w |= e.array[j - 1]! >> (bIDB + i - k1);
        }
      }
      n = k;
      while ((w & 1) === 0) {
        w >>= 1;
        n -= 1;
      }
      i -= n;
      if (i < 0) {
        i += bIDB;
        j -= 1;
      }
      if (is1) {
        // ret == 1, don't bother squaring or multiplying it
        g[w]!.copyTo(r);
        is1 = false;
      } else {
        while (n > 1) {
          z.sqrTo(r, r2);
          z.sqrTo(r2, r);
          n -= CRYPTO_NUMBER_2;
        }
        if (n > 0) {
          z.sqrTo(r, r2);
        } else {
          t = r;
          r = r2;
          r2 = t;
        }
        z.mulTo(r2, g[w]!, r);
      }
      while (j >= 0 && (e.array[j]! & (1 << i)) === 0) {
        z.sqrTo(r, r2);
        t = r;
        r = r2;
        r2 = t;
        i -= 1;
        if (i < 0) {
          i = bIDB - 1;
          j -= 1;
        }
      }
    }
    return z.revert(r);
  }
  // (public) gcd(this,a) (HAC 14.54)
  gcd(a: BigInteger): BigInteger {
    let x = this.s < 0 ? this.negate() : this.clone();
    let y = a.s < 0 ? a.negate() : a.clone();
    if (x.compareTo(y) < 0) {
      let t = x;
      x = y;
      y = t;
    }
    let i = x.getLowestSetBit();
    let g = y.getLowestSetBit();
    if (g < 0) {
      return x;
    }
    if (i < g) {
      g = i;
    }
    if (g > 0) {
      x.rShiftTo(g, x);
      y.rShiftTo(g, y);
    }
    while (x.signum() > 0) {
      i = x.getLowestSetBit();
      if (i > 0) {
        x.rShiftTo(i, x);
      }
      i = y.getLowestSetBit();
      if (i > 0) {
        y.rShiftTo(i, y);
      }
      if (x.compareTo(y) >= 0) {
        x.subTo(y, x);
        x.rShiftTo(CRYPTO_NUMBER_1, x);
      } else {
        y.subTo(x, y);
        y.rShiftTo(CRYPTO_NUMBER_1, y);
      }
    }
    if (g > 0) {
      y.lShiftTo(g, y);
    }
    return y;
  }

  // (protected) this % n, n < 2^26
  modInt(n: number): number {
    if (n <= 0) {
      return 0;
    }
    let d = bIDV % n;
    let r = this.s < 0 ? n - 1 : 0;
    if (this.t > 0) {
      if (d === 0) {
        r = this.array[0]! % n;
      } else {
        let i = this.t - 1;
        while (i >= 0) {
          r = (d * r + this.array[i]!) % n;
          i -= 1;
        }
      }
    }
    return r;
  }

  // (public) 1/this % m (HAC 14.61)
  modInverse(m: BigInteger): BigInteger {
    let ac = m.isEven();
    if ((this.isEven() && ac) || m.signum() === 0) {
      return BigInteger.zero;
    }
    let u = m.clone();
    let v = this.clone();
    let a: BigInteger = BigInteger.nbv(1);
    let b: BigInteger = BigInteger.nbv(0);
    let c: BigInteger = BigInteger.nbv(0);
    let d: BigInteger = BigInteger.nbv(1);
    while (u.signum() !== 0) {
      while (u.isEven()) {
        u.rShiftTo(1, u);
        if (ac) {
          if (!a.isEven() || !b.isEven()) {
            a.addTo(this, a);
            b.subTo(m, b);
          }
          a.rShiftTo(1, a);
        } else if (!b.isEven()) {
          b.subTo(m, b);
        }
        b.rShiftTo(1, b);
      }
      while (v.isEven()) {
        v.rShiftTo(1, v);
        if (ac) {
          if (!c.isEven() || !d.isEven()) {
            c.addTo(this, c);
            d.subTo(m, d);
          }
          c.rShiftTo(1, c);
        } else if (!d.isEven()) {
          d.subTo(m, d);
        }
        d.rShiftTo(1, d);
      }
      if (u.compareTo(v) >= 0) {
        u.subTo(v, u);
        if (ac) {
          a.subTo(c, a);
        }
        b.subTo(d, b);
      } else {
        v.subTo(u, v);
        if (ac) {
          c.subTo(a, c);
        }
        d.subTo(b, d);
      }
    }
    if (v.compareTo(BigInteger.one) !== 0) {
      return BigInteger.zero;
    }
    if (d.compareTo(m) >= 0) {
      return d.subtract(m);
    }
    if (d.signum() < 0) {
      d.addTo(m, d);
    } else {
      return d;
    }
    if (d.signum() < 0) {
      return d.add(m);
    } else {
      return d;
    }
  }

  // (public) test primality with certainty >= 1-.5^t
  isProbablePrime(t: number): boolean {
    let x = this.abs();
    if (x.t === 1 && x.array[0]! <= lowprimes[lowprimes.length - 1]) {
      for (let i = 0; i < lowprimes.length; ++i) {
        if (x.array[0] === lowprimes[i]) {
          return true;
        }
      }
      return false;
    }
    if (x.isEven()) {
      return false;
    }
    let i = 1;
    while (i < lowprimes.length) {
      let m = lowprimes[i];
      let j = i + 1;
      while (j < lowprimes.length && m < lplim) {
        m *= lowprimes[j];
        j += 1;
      }
      m = x.modInt(m);
      while (i < j) {
        if (m % lowprimes[i] === 0) {
          return false;
        }
        i += 1;
      }
    }
    return x.millerRabin(t);
  }

  // (protected) true if probably prime (HAC 4.24, Miller-Rabin)
  millerRabin(t: number): boolean {
    let tempT = t;
    let n1 = this.subtract(BigInteger.one);
    let k = n1.getLowestSetBit();
    if (k <= 0) {
      return false;
    }
    let r = n1.shiftRight(k);
    tempT = (tempT + 1) >> 1;
    if (tempT > lowprimes.length) {
      tempT = lowprimes.length;
    }
    let a = BigInteger.nbi();
    for (let i = 0; i < tempT; ++i) {
      a.fromInt(lowprimes[i]);
      let y = a.modPow(r, this);
      if (y.compareTo(BigInteger.one) !== 0 && y.compareTo(n1) !== 0) {
        let j = 1;
        while (j < k && y.compareTo(n1) !== 0) {
          j += 1;
          y = y.modPowInt(CRYPTO_NUMBER_2, this);
          if (y.compareTo(BigInteger.one) === 0) {
            return false;
          }
        }
        if (y.compareTo(n1) !== 0) {
          return false;
        }
      }
    }
    return true;
  }

  am(i: number, x: number, w: BigInteger, j: number, c: number, n: number): number {
    return aM(this.array, i, x, w, j, c, n);
  }
}

interface ConvertProtocol {
  convert(x: BigInteger): BigInteger;
  revert(x: BigInteger): BigInteger;
  reduce(x: BigInteger): void;
  sqrTo(x: BigInteger, r: BigInteger): void;
  mulTo(x: BigInteger, y: BigInteger, r: BigInteger): void;
}

// Modular reduction using "classic" algorithm
class Classic implements ConvertProtocol {
  m: BigInteger;

  constructor(m: BigInteger) {
    this.m = m;
  }

  convert(x: BigInteger): BigInteger {
    if (x.s < 0 || x.compareTo(this.m) >= 0) {
      return x.mod(this.m);
    } else {
      return x;
    }
  }

  revert(x: BigInteger): BigInteger {
    return x;
  }

  reduce(x: BigInteger): void {
    x.divRemTo(this.m, null, x);
  }

  sqrTo(x: BigInteger, r: BigInteger): void {
    x.squareTo(r);
    this.reduce(r);
  }

  mulTo(x: BigInteger, y: BigInteger, r: BigInteger): void {
    x.multiplyTo(y, r);
    this.reduce(r);
  }
}

// Montgomery reduction
class Montgomery implements ConvertProtocol {
  m: BigInteger;
  mp: number;
  mpl: number;
  mph: number;
  um: number;
  mt2: number;

  constructor(m: BigInteger) {
    this.m = m;
    this.mp = m.invDigit();
    this.mpl = this.mp & CRYPTO_NUMBER_0X7FFF;
    this.mph = this.mp >> CRYPTO_NUMBER_15;
    this.um = (1 << (bIDB - CRYPTO_NUMBER_15)) - 1;
    this.mt2 = CRYPTO_NUMBER_2 * m.t;
  }

  // xR mod m
  convert(x: BigInteger): BigInteger {
    let r: BigInteger | null = BigInteger.nbi();
    x.abs().dlShiftTo(this.m.t, r);
    r.divRemTo(this.m, null, r);
    if (x.s < 0 && r.compareTo(BigInteger.zero) > 0) {
      this.m.subTo(r, r);
    }
    return r;
  }

  // x/R mod m
  revert(x: BigInteger): BigInteger {
    let r: BigInteger | null = BigInteger.nbi();
    x.copyTo(r);
    this.reduce(r);
    return r;
  }

  // x = x/R mod m (HAC 14.32)
  reduce(x: BigInteger): void {
    while (x.t <= this.mt2) {
      // pad x so am has enough room later
      x.array = insertValue(x.array, 0, x.t);
      x.t += 1;
    }
    for (let i = 0; i < this.m.t; ++i) {
      // faster way of calculating u0 = x[i]*mp mod DV
      let j = x.array[i]! & CRYPTO_NUMBER_0X7FFF;
      let u0 = (j * this.mpl + (((j * this.mph + (x.array[i]! >> CRYPTO_NUMBER_15) * this.mpl) & this.um) << CRYPTO_NUMBER_15)) & bIDM;
      // use am to combine the multiply-shift-add into one call
      j = i + this.m.t;
      let value = x.array[j]! + this.m.am(0, u0, x, i, 0, this.m.t);
      x.array = insertValue(x.array, value, j);
      // propagate carry
      while (x.array[j]! >= bIDV) {
        x.array = insertValue(x.array, x.array[j]! - bIDV, j);
        j += 1;
        x.array = insertValue(x.array, x.array[j]! + 1, j);
      }
    }
    x.clamp();
    x.drShiftTo(this.m.t, x);
    if (x.compareTo(this.m) >= 0) {
      x.subTo(this.m, x);
    }
  }

  // r = "x^2/R mod m"; x !== r
  sqrTo(x: BigInteger, r: BigInteger): void {
    x.squareTo(r);
    this.reduce(r);
  }

  // r = "xy/R mod m"; x,y !== r
  mulTo(x: BigInteger, y: BigInteger, r: BigInteger): void {
    x.multiplyTo(y, r);
    this.reduce(r);
  }
}

class NullExp implements ConvertProtocol {
  convert(x: BigInteger): BigInteger {
    return x;
  }

  revert(x: BigInteger): BigInteger {
    return x;
  }

  reduce(x: BigInteger): void {
    //debugLog('Method not implemented.')
  }

  sqrTo(x: BigInteger, r: BigInteger): void {
    x.squareTo(r);
  }

  mulTo(x: BigInteger, y: BigInteger, r: BigInteger): void {
    x.multiplyTo(y, r);
  }
}

// Barrett modular reduction
class Barrett implements ConvertProtocol {
  r2: BigInteger;
  q3: BigInteger;
  mu: BigInteger;
  m: BigInteger;

  constructor(m: BigInteger) {
    // setup Barrett
    this.r2 = BigInteger.nbi();
    this.q3 = BigInteger.nbi();
    BigInteger.one.dlShiftTo(CRYPTO_NUMBER_2 * m.t, this.r2);
    this.mu = this.r2.divide(m);
    this.m = m;
  }

  convert(x: BigInteger): BigInteger {
    if (x.s < 0 || x.t > CRYPTO_NUMBER_2 * this.m.t) {
      return x.mod(this.m);
    } else if (x.compareTo(this.m) < 0) {
      return x;
    } else {
      let r: BigInteger | null = BigInteger.nbi();
      x.copyTo(r);
      this.reduce(r);
      return r;
    }
  }

  revert(x: BigInteger): BigInteger {
    return x;
  }

  // x = x mod m (HAC 14.42)
  reduce(x: BigInteger): void {
    x.drShiftTo(this.m.t - 1, this.r2);
    if (x.t > this.m.t + 1) {
      x.t = this.m.t + 1;
      x.clamp();
    }
    this.mu.multiplyUpperTo(this.r2, this.m.t + 1, this.q3);
    this.m.multiplyLowerTo(this.q3, this.m.t + 1, this.r2);
    while (x.compareTo(this.r2) < 0) {
      x.dAddOffset(1, this.m.t + 1);
    }
    x.subTo(this.r2, x);
    while (x.compareTo(this.m) >= 0) {
      x.subTo(this.m, x);
    }
  }

  // r = x^2 mod m; x !== r
  sqrTo(x: BigInteger, r: BigInteger): void {
    x.squareTo(r);
    this.reduce(r);
  }

  // r = x*y mod m; x,y !== r
  mulTo(x: BigInteger, y: BigInteger, r: BigInteger): void {
    x.multiplyTo(y, r);
    this.reduce(r);
  }
}

class Arcfour {
  i: number = 0;
  j: number = 0;
  s: (number | null)[] = [];

  // Initialize arcfour context from key, an array of ints, each from [0..255]
  arc4Init(key: (number | null)[]): void {
    let j: number = 0;
    let t = 0;
    for (let i = 0; i < CRYPTO_NUMBER_256; ++i) {
      this.s = insertValue(this.s, i, i);
    }
    for (let i = 0; i < CRYPTO_NUMBER_256; ++i) {
      j = (j + this.s[i]! + key[i % key.length]!) & CRYPTO_NUMBER_255;
      t = this.s[i]!;
      this.s = insertValue(this.s, this.s[j]!, i);
      this.s = insertValue(this.s, t, j);
    }
    this.i = 0;
    this.j = 0;
  }

  next(): number {
    let t: number;
    this.i = (this.i + 1) & CRYPTO_NUMBER_255;
    this.j = (this.j + this.s[this.i]!) & CRYPTO_NUMBER_255;
    t = this.s[this.i]!;
    this.s = insertValue(this.s, this.s[this.j]!, this.i);
    this.s = insertValue(this.s, t, this.j);
    return this.s[(t + this.s[this.i]!) & CRYPTO_NUMBER_255]!;
  }
}

// Plug in your RNG constructor here
function prngNewstate(): Arcfour {
  return new Arcfour();
}

// Pool size must be a multiple of 4 and greater than 32.
// An array of bytes the size of the pool will be passed to init()
let rngPsize = CRYPTO_NUMBER_256;

// For best results, put code like
// <body onClick='rngSeedTime();' onKeyPress='rngSeedTime();'>
// in your main HTML document.

let rngState: Arcfour | null;
let rngPool: (number | null)[] | null;
let rngPptr: number = 0;

// Mix in a 32-bit integer into the pool
function rngSeedInt(x: number): void {
  (rngPool![rngPptr] as number) ^= x & CRYPTO_NUMBER_255;
  rngPptr += 1;
  (rngPool![rngPptr] as number) ^= (x >> CRYPTO_NUMBER_8) & CRYPTO_NUMBER_255;
  rngPptr += 1;
  (rngPool![rngPptr] as number) ^= (x >> CRYPTO_NUMBER_16) & CRYPTO_NUMBER_255;
  rngPptr += 1;
  (rngPool![rngPptr] as number) ^= (x >> CRYPTO_NUMBER_24) & CRYPTO_NUMBER_255;
  rngPptr += 1;
  if (rngPptr >= rngPsize) {
    rngPptr -= rngPsize;
  }
}

// Mix in the current time (w/milliseconds) into the pool
function rngSeedTime(): void {
  // Use pre-computed date to avoid making the benchmark
  // results dependent on the current date.
  rngSeedInt(CRYPTO_NUMBER_1122926989487);
}

// Initialize the pool with junk if needed.
function initRngPool(): void {
  if (rngPool == null) {
    rngPool = [];
    rngPptr = 0;
    let t: number;
    while (rngPptr < rngPsize) {
      // extract some randomness from Math.random()
      t = Math.floor(CRYPTO_NUMBER_65536 * Math.random());
      rngPool = insertValue(rngPool, t >>> CRYPTO_NUMBER_8, rngPptr);
      rngPptr += 1;
      rngPool = insertValue(rngPool, t & CRYPTO_NUMBER_255, rngPptr);
      rngPptr += 1;
    }
    rngPptr = 0;
    rngSeedTime();
  }
}

class SecureRandom {
  nextBytes(ba: (number | null)[]): void {
    for (let i = 0; i < ba.length; ++i) {
      ba = insertValue(ba, this.rngGetByte(), i);
    }
  }

  rngGetByte(): number {
    if (rngState == null) {
      rngSeedTime();
      rngState = prngNewstate();
      rngState.arc4Init(rngPool!);
      rngPptr = 0;
      while (rngPptr < rngPool!.length) {
        rngPool = insertValue(rngPool!, 0, rngPptr);
        rngPptr += 1;
      }
      rngPptr = 0;
    }
    // TODO: allow reseeding after first request
    return rngState.next();
  }
}

// "empty" RSA key constructor
class RsaKey {
  n: BigInteger | null;
  d: BigInteger | null;
  p: BigInteger | null;
  q: BigInteger | null;
  e: number;
  dmp1: BigInteger | null;
  dmq1: BigInteger | null;
  coeff: BigInteger | null;

  constructor() {
    this.n = null; //default
    this.e = CRYPTO_NUMBER_0;
    this.d = null; //default
    this.p = null; //default
    this.q = null; //default
    this.dmp1 = null; //default
    this.dmq1 = null; //default
    this.coeff = null; //default
  }

  // convert a (hex) string to a bignum object
  parseBigInt(str: string, r: number): BigInteger {
    return new BigInteger(str, r);
  }

  linebrk(s: string, n: number): string {
    let ret = '';
    let i: number = 0;
    while (i + n < s.length) {
      ret += s.substring(i, i + n) + '\n';
      i += n;
    }
    return ret + s.substring(i, s.length);
  }

  byte2Hex(b: number): string {
    if (b < CRYPTO_NUMBER_0X10) {
      return '0' + b.toString(CRYPTO_NUMBER_16);
    } else {
      return b.toString(CRYPTO_NUMBER_16);
    }
  }

  // PKCS#1 (type 2, random) pad input string s to n bytes, and return a bigint
  pkcs1pad2(s: string, n: number): BigInteger | null {
    if (n < s.length + CRYPTO_NUMBER_11) {
      //debugLog('Message too long for RSA');
      return null;
    }
    let tempN = n;
    let ba: (number | null)[] = [];
    let i = s.length - 1;
    while (i >= 0 && tempN > 0) {
      tempN -= 1;
      ba = insertValue(ba, s.charCodeAt(i), tempN);
      i -= 1;
    }
    tempN -= 1;
    ba = insertValue(ba, 0, tempN);
    let rng = new SecureRandom();
    let x: (number | null)[] = [];
    while (tempN > CRYPTO_NUMBER_2) {
      // random non-zero pad
      x = insertValue(x, 0, 0);
      while (x[0] === 0) {
        rng.nextBytes(x);
      }
      tempN -= 1;
      ba = insertValue(ba, x[0]!, tempN);
    }
    tempN -= 1;
    ba = insertValue(ba, CRYPTO_NUMBER_2, tempN);
    tempN -= 1;
    ba = insertValue(ba, 0, tempN);
    return new BigInteger(ba!);
  }

  // Set the public key fields N and e from hex strings
  setPublic(n: string, e: string): void {
    if (n.length > 0 && e.length > 0) {
      this.n = this.parseBigInt(n, CRYPTO_NUMBER_16);
      this.e = Number.parseInt(e, CRYPTO_NUMBER_16);
    } else {
      //debugLog("Invalid RSA public key");
    }
  }

  // Perform raw public operation on "x": return x^e (mod n)
  doPublic(x: BigInteger): BigInteger {
    return x.modPowInt(this.e, this.n as BigInteger);
  }

  // Return the PKCS#1 RSA encryption of "text" as an even-length hex string
  encrypt(text: string): string | null {
    let m = this.pkcs1pad2(text, ((this.n as BigInteger).bitLength() + CRYPTO_NUMBER_7) >> CRYPTO_NUMBER_3);
    if (m === null) {
      return null;
    }
    let c = this.doPublic(m);
    if (c === null) {
      return null;
    }
    let h = c.toString(CRYPTO_NUMBER_16);
    if ((h.length & 1) === 0) {
      return h;
    } else {
      return '0' + h;
    }
  }

  // Undo PKCS#1 (type 2, random) padding and, if valid, return the plaintext
  pkcs1unpad2(d: BigInteger, n: number): string | null {
    let b = d.toByteArray();
    let i = 0;
    while (i < b.length && b[i] === 0) {
      i += 1;
    }
    if (b.length - i !== n - 1 || b[i] !== CRYPTO_NUMBER_2) {
      return null;
    }
    i += 1;
    while (b[i] !== 0) {
      i += 1;
      if (i >= b.length) {
        return null;
      }
    }
    let ret = '';
    i += 1;
    while (i < b.length) {
      ret += String.fromCharCode(b[i]!)!;
      i += 1;
    }
    return ret;
  }

  // Set the private key fields N, e, and d from hex strings
  setPrivate(n: string, e: string, d: string): void {
    if (n.length > 0 && e.length > 0) {
      this.n = this.parseBigInt(n, CRYPTO_NUMBER_16);
      this.e = Number.parseInt(e, CRYPTO_NUMBER_16);
      this.d = this.parseBigInt(d, CRYPTO_NUMBER_16);
    } else {
      //debugLog("Invalid RSA private key");
    }
  }

  // Set the private key fields N, e, d and CRT params from hex strings
  setPrivateEx(n: string, e: string, d: string, p: string, q: string, dp: string, dq: string, c: string): void {
    if (n.length > 0 && e.length > 0) {
      this.n = this.parseBigInt(n, CRYPTO_NUMBER_16);
      this.e = Number.parseInt(e, CRYPTO_NUMBER_16);
      this.d = this.parseBigInt(d, CRYPTO_NUMBER_16);
      this.p = this.parseBigInt(p, CRYPTO_NUMBER_16);
      this.q = this.parseBigInt(q, CRYPTO_NUMBER_16);
      this.dmp1 = this.parseBigInt(dp, CRYPTO_NUMBER_16);
      this.dmq1 = this.parseBigInt(dq, CRYPTO_NUMBER_16);
      this.coeff = this.parseBigInt(c, CRYPTO_NUMBER_16);
    } else {
      //debugLog("Invalid RSA private key");
    }
  }

  // Generate a new random private key B bits long, using public expt E
  generate(b: number, e: string): void {
    let rng = new SecureRandom();
    let qs = b >> 1;
    this.e = Number.parseInt(e, CRYPTO_NUMBER_16);
    let ee = new BigInteger(e, CRYPTO_NUMBER_16);
    while (true) {
      while (true) {
        this.p = new BigInteger(b - qs, 1, rng);
        if (this.p.subtract(BigInteger.one).gcd(ee).compareTo(BigInteger.one) === 0 && this.p.isProbablePrime(CRYPTO_NUMBER_10)) {
          break;
        }
      }
      while (true) {
        this.q = new BigInteger(qs, 1, rng);
        if (this.q.subtract(BigInteger.one).gcd(ee).compareTo(BigInteger.one) === 0 && this.q.isProbablePrime(CRYPTO_NUMBER_10)) {
          break;
        }
      }
      if (this.p.compareTo(this.q) <= 0) {
        let t = this.p;
        this.p = this.q;
        this.q = t;
      }
      let p1 = this.p.subtract(BigInteger.one);
      let q1 = this.q.subtract(BigInteger.one);
      let phi = p1.multiply(q1);
      if (phi.gcd(ee).compareTo(BigInteger.one) === 0) {
        this.dmp1 = this.d!.mod(p1);
        this.dmq1 = this.d!.mod(q1);
        this.n = this.p.multiply(this.q);
        this.d = ee.modInverse(phi);
        this.coeff = this.q.modInverse(this.p);
        break;
      }
    }
  }

  // Perform raw private operation on "x": return x^d (mod n)
  doPrivate(x: BigInteger): BigInteger | null {
    if (this.p === null || this.q === null) {
      return x.modPow(this.d as BigInteger, this.n as BigInteger);
    }
    // TODO: re-calculate any missing CRT params
    let xp = x.mod(this.p as BigInteger).modPow(this.dmp1 as BigInteger, this.p as BigInteger);
    let xq = x.mod(this.q as BigInteger).modPow(this.dmq1 as BigInteger, this.q as BigInteger);
    while (xp.compareTo(xq) < 0) {
      xp = xp.add(this.p as BigInteger);
    }
    return xp
      .subtract(xq)
      .multiply(this.coeff as BigInteger)
      .mod(this.p as BigInteger)
      .multiply(this.q as BigInteger)
      .add(xq);
  }

  // Return the PKCS#1 RSA decryption of "ctext".
  // "ctext" is an even-length hex string and the output is a plain string.
  decrypt(ctext: string): string | null {
    let c = this.parseBigInt(ctext, CRYPTO_NUMBER_16);
    let m = this.doPrivate(c);
    if (m === null) {
      return null;
    }
    return this.pkcs1unpad2(m, ((this.n as BigInteger).bitLength() + CRYPTO_NUMBER_7) >> CRYPTO_NUMBER_3);
  }
}

let nValue =
  'a5261939975948bb7a58dffe5ff54e65f0498f9175f5a09288810b8975871e99af3b5dd94057b0fc07535f5f97444504fa35169d461d0d30cf0192e307727c065168c788771c561a9400fb49175e9e6aa4e23fe11af69e9412dd23b0cb6684c4c2429bce139e848ab26d0829073351f4acd36074eafd036a5eb83359d2a698d3';
let eValue = '10001';
let dValue =
  '8e9912f6d3645894e8d38cb58c0db81ff516cf4c7e5a14c7f1eddb1459d2cded4d8d293fc97aee6aefb861859c8b6a3d1dfe710463e1f9ddc72048c09751971c4a580aa51eb523357a3cc48d31cfad1d4a165066ed92d4748fb6571211da5cb14bc11b6e2df7c1a559e6d5ac1cd5c94703a22891464fba23d0d965086277a161';
let pValue = 'd090ce58a92c75233a6486cb0a9209bf3583b64f540c76f5294bb97d285eed33aec220bde14b2417951178ac152ceab6da7090905b478195498b352048f15e7d';
let qValue = 'cab575dc652bb66df15a0359609d51d1db184750c00c6698b90ef3465c99655103edbf0d54c56aec0ce3c4d22592338092a126a0cc49f65a4a30d222b411e58f';
let dmp1Value = '1a24bca8e273df2f0e47c199bbf678604e7df7215480c77c8db39f49b000ce2cf7500038acfff5433b7d582a01f1826e6f4d42e1c57f5e1fef7b12aabc59fd25';
let dmq1Value = '3d06982efbbe47339e1f6d36b1216b8a741d410b0c662f54f7118b27b9a4ec9d914337eb39841d8666f3034408cf94f5b62f11c402fc994fe15a05493150d9fd';
let coeffValue = '3a3e731acd8960b7ff9eb81a7ff93bd1cfa74cbd56987db58b4594fb09c09084db1734c8143f98b602b981aaa9243ca28deb69b5b280ee8dcee0fd2625e53250';

let text = 'The quick brown fox jumped over the extremely lazy frog! ' + 'Now is the time for all good men to come to the party.';
let encrypted: string | null;

function encrypt(): void {
  let RSA = new RsaKey();
  RSA.setPublic(nValue, eValue);
  RSA.setPrivateEx(nValue, eValue, dValue, pValue, qValue, dmp1Value, dmq1Value, coeffValue);
  encrypted = RSA.encrypt(text);
}

function decrypt(): void {
  let RSA = new RsaKey();
  RSA.setPublic(nValue, eValue);
  RSA.setPrivateEx(nValue, eValue, dValue, pValue, qValue, dmp1Value, dmq1Value, coeffValue);
  let decrypted = RSA.decrypt(encrypted as string);
  if (decrypted !== text) {
    // debugLog("Crypto operation failed");
  }
}

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  /*
   * @Setup
   */
  cryptoSetup(): void {
    initBIRC();
    initRngPool();
    setupEngine(am3, CRYPTO_NUMBER_28);
  }

  /*
   * @Benchmark
   */
  runIteration(): void {
    encrypt();
    decrypt();
  }
}

function cryptoRunIteration(): void {
  let benchmark = new Benchmark();
  let setupStart = ArkTools.timeInUs() / CRYPTO_NUMBER_1000;
  benchmark.cryptoSetup();
  let setupEnd = ArkTools.timeInUs() / CRYPTO_NUMBER_1000;
  //debugLog("setupTime: " + String(setupEnd - setupStart) + "ms");
  let runTimes: number[] = [];
  let runStart = ArkTools.timeInUs() / CRYPTO_NUMBER_1000;
  for (let index = 0; index < CRYPTO_RUNCOUNT_120; index++) {
    let start = ArkTools.timeInUs() / CRYPTO_NUMBER_1000;
    benchmark.runIteration();
    let end = ArkTools.timeInUs() / CRYPTO_NUMBER_1000;
    let runtime = end - start;
    runTimes.push(runtime);
    // debugLog("onceRunTime: " + String(runtime) + "ms");
  }
  let endTime = ArkTools.timeInUs() / CRYPTO_NUMBER_1000;
  // debugLog("runTime: " + String(endTime - runStart) + "ms");
  print('crypto: ms = ' + String(endTime - setupStart));
}

function debugLog(str: string): void {
  let isLog = false;
  if (isLog) {
    print(str);
  }
}

function insertBigIntegerValue(array: (BigInteger | null)[], value: BigInteger, i: number): (BigInteger | null)[] {
  let arr = array;
  if (i < arr.length) {
    arr[i] = value;
  } else {
    const surplus = i - arr.length;
    const surplusArr: (BigInteger | null)[] = new Array(surplus + 1).fill(null);
    arr = arr.concat(surplusArr);
    arr[i] = value;
  }
  return arr;
}

function insertValue(array: (number | null)[], value: number, i: number): (number | null)[] {
  let arr = array;
  if (i < arr.length) {
    arr[i] = value;
  } else {
    const surplus = i - arr.length;
    const surplusArr: (number | null)[] = new Array(surplus + 1).fill(null);
    arr = arr.concat(surplusArr);
    arr[i] = value;
  }
  return arr;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  cryptoRunIteration()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

cryptoRunIteration();




