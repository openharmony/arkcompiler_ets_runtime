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

const TIME_MULTIPLIER: number = 1000;
const MD5_NUM_2: number = 2;
const MD5_NUM_3: number = 3;
const MD5_NUM_4: number = 4;
const MD5_NUM_5: number = 5;
const MD5_NUM_6: number = 6;
const MD5_NUM_7: number = 7;
const MD5_NUM_8: number = 8;
const MD5_NUM_9: number = 9;
const MD5_NUM_10: number = 10;
const MD5_NUM_11: number = 11;
const MD5_NUM_12: number = 12;
const MD5_NUM_13: number = 13;
const MD5_NUM_14: number = 14;
const MD5_NUM_15: number = 15;
const MD5_NUM_16: number = 16;
const MD5_NUM_17: number = 17;
const MD5_NUM_20: number = 20;
const MD5_NUM_21: number = 21;
const MD5_NUM_22: number = 22;
const MD5_NUM_23: number = 23;
const MD5_NUM_32: number = 32;
const MD5_NUM_64: number = 64;
const MD5_NUM_120: number = 20;
const MD5_NUM_128: number = 128;
const MD5_NUM_512: number = 512;

const MD5_NUM_38016083: number = 38016083;
const MD5_NUM_568446438: number = 568446438;
const MD5_NUM_606105819: number = 606105819;
const MD5_NUM_643717713: number = 643717713;
const MD5_NUM_681279174: number = 681279174;
const MD5_NUM_1200080426: number = 1200080426;
const MD5_NUM_1770035416: number = 1770035416;
const MD5_NUM_1804603682: number = 1804603682;
const MD5_NUM_1163531501: number = 1163531501;
const MD5_NUM_1735328473: number = 1735328473;
const MD5_NUM_1839030562: number = 1839030562;
const MD5_NUM_1272893353: number = 1272893353;

const MD5_NUM_76029189: number = 76029189;
const MD5_NUM_1126891415: number = 1126891415;
const MD5_NUM_530742520: number = 530742520;
const MD5_NUM_1700485571: number = 1700485571;

const MD5_NUM_1873313359: number = 1873313359;
const MD5_NUM_1309151649: number = 1309151649;
const MD5_NUM_718787259: number = 718787259;
const MD5_NUM_1236535329: number = 1236535329;

const MD5_NUM_NEGATIVE_343485551: number = -343485551;
const MD5_NUM_NEGATIVE_1120210379: number = -1120210379;
const MD5_NUM_NEGATIVE_145523070: number = -145523070;
const MD5_NUM_NEGATIVE_1560198380: number = -1560198380;

const MD5_NUM_NEGATIVE_30611744: number = -30611744;
const MD5_NUM_NEGATIVE_2054922799: number = -2054922799;
const MD5_NUM_NEGATIVE_1051523: number = -1051523;
const MD5_NUM_NEGATIVE_1894986606: number = -1894986606;

const MD5_NUM_NEGATIVE_57434055: number = -57434055;
const MD5_NUM_NEGATIVE_1416354905: number = -1416354905;
const MD5_NUM_NEGATIVE_198630844: number = -198630844;
const MD5_NUM_NEGATIVE_995338651: number = -995338651;

const MD5_NUM_NEGATIVE_421815835: number = -421815835;
const MD5_NUM_NEGATIVE_640364487: number = -640364487;
const MD5_NUM_NEGATIVE_722521979: number = -722521979;
const MD5_NUM_NEGATIVE_358537222: number = -358537222;

const MD5_NUM_NEGATIVE_1094730640: number = -1094730640;
const MD5_NUM_NEGATIVE_155497632: number = -155497632;
const MD5_NUM_NEGATIVE_1530992060: number = -1530992060;
const MD5_NUM_NEGATIVE_35309556: number = -35309556;

const MD5_NUM_NEGATIVE_2022574463: number = -2022574463;
const MD5_NUM_NEGATIVE_378558: number = -378558;
const MD5_NUM_NEGATIVE_1926607734: number = -1926607734;
const MD5_NUM_NEGATIVE_51403784: number = -51403784;

const MD5_NUM_NEGATIVE_1444681467: number = -1444681467;
const MD5_NUM_NEGATIVE_187363961: number = -187363961;
const MD5_NUM_NEGATIVE_1019803690: number = -1019803690;
const MD5_NUM_NEGATIVE_405537848: number = -405537848;
const MD5_NUM_NEGATIVE_660478335: number = -660478335;
const MD5_NUM_NEGATIVE_701558691: number = -701558691;
const MD5_NUM_NEGATIVE_373897302: number = -373897302;
const MD5_NUM_NEGATIVE_1069501632: number = -1069501632;

const MD5_NUM_NEGATIVE_165796510: number = -165796510;
const MD5_NUM_NEGATIVE_1502002290: number = -1502002290;
const MD5_NUM_NEGATIVE_40341101: number = -40341101;
const MD5_NUM_NEGATIVE_1990404162: number = -1990404162;
const MD5_NUM_NEGATIVE_42063: number = -42063;
const MD5_NUM_NEGATIVE_1958414417: number = -1958414417;
const MD5_NUM_NEGATIVE_45705983: number = -45705983;
const MD5_NUM_NEGATIVE_1473231341: number = -1473231341;
const MD5_NUM_NEGATIVE_176418897: number = -176418897;
const MD5_NUM_NEGATIVE_1044525330: number = -1044525330;
const MD5_NUM_NEGATIVE_389564586: number = -389564586;
const MD5_NUM_NEGATIVE_680876936: number = -680876936;

const MD5_NUM_0X80: number = 0x80;
const MD5_NUM_0X3F: number = 0x3f;
const MD5_NUM_0XF: number = 0xf;
const MD5_NUM_0XFF: number = 0xff;
const MD5_NUM_0XFFFF: number = 0xffff;
const MD5_NUM_0X36363636: number = 0x36363636;
const MD5_NUM_0X5C5C5C5C: number = 0x5c5c5c5c;

let inDebug = false;
function log(str: string): void {
  if (inDebug) {
    print(str);
  }
}
function currentTimestamp13(): number {
  return ArkTools.timeInUs() / TIME_MULTIPLIER;
}

/*
 * Configurable variables. You may need to tweak these to be compatible with
 * the server-side, but the defaults work in most cases.
 */
let hexcase = 0; /* hex output format. 0 - lowercase; 1 - uppercase        */
let b64pad = ''; /* base-64 pad character. "=" for strict RFC compliance   */
let chrsz = 8; /* bits per input character. 8 - ASCII; 16 - Unicode      */

/*
 * These are the funcs you'll usually want to call
 * They take string arguments and return either hex or base-64 encoded strings
 */
function hexMd5(s: string): string {
  return binl2hex(coreMd5(str2binl(s), s.length * chrsz));
}
function b64Md5(s: string): string {
  return binl2b64(coreMd5(str2binl(s), s.length * chrsz));
}
function strMd5(s: string): string {
  return binl2str(coreMd5(str2binl(s), s.length * chrsz));
}
function hexHmacMd5(key: string, data: string): string {
  return binl2hex(coreHmacMd5(key, data));
}
function b64HmacMd5(key: string, data: string): string {
  return binl2b64(coreHmacMd5(key, data));
}
function strHmacMd5(key: string, data: string): string {
  return binl2str(coreHmacMd5(key, data));
}

/*
 * Perform a simple self-test to see if the VM is working
 */
function md5VmTest(): boolean {
  return hexMd5('abc') === '900150983cd24fb0d6963f7d28e17f72';
}

/*
 * Calculate the MD5 of an array of little-endian words, and a bit length
 */
function coreMd5(x1: number[], len: number): number[] {
  let x = x1;
  const toMax = (len >> MD5_NUM_5) - x.length;
  if (toMax >= 0) {
    x = x.concat(Array<number>(toMax + 1));
  }
  /* append padding */
  x[len >> MD5_NUM_5] |= MD5_NUM_0X80 << len % MD5_NUM_32;

  const toMax1 = (((len + MD5_NUM_64) >>> MD5_NUM_9) << MD5_NUM_4) + MD5_NUM_14 - x.length;
  if (toMax1 >= 0) {
    x = x.concat(Array<number>(toMax1 + 1));
  }
  x[(((len + MD5_NUM_64) >>> MD5_NUM_9) << MD5_NUM_4) + MD5_NUM_14] = len;

  let a = 1732584193;
  let b = -271733879;
  let c = -1732584194;
  let d = 271733878;
  let j = 0;
  while (j < x.length) {
    const toMax2 = j + MD5_NUM_15 - x.length;
    if (toMax2 >= 0) {
      x = x.concat(Array<number>(toMax2 + 1));
    }
    let olda = a;
    let oldb = b;
    let oldc = c;
    let oldd = d;

    a = md5Ff(a, b, c, d, x[j + 0], MD5_NUM_7, MD5_NUM_NEGATIVE_680876936);
    d = md5Ff(d, a, b, c, x[j + 1], MD5_NUM_12, MD5_NUM_NEGATIVE_389564586);
    c = md5Ff(c, d, a, b, x[j + MD5_NUM_2], MD5_NUM_17, MD5_NUM_606105819);
    b = md5Ff(b, c, d, a, x[j + MD5_NUM_3], MD5_NUM_22, MD5_NUM_NEGATIVE_1044525330);

    a = md5Ff(a, b, c, d, x[j + MD5_NUM_4], MD5_NUM_7, MD5_NUM_NEGATIVE_176418897);
    d = md5Ff(d, a, b, c, x[j + MD5_NUM_5], MD5_NUM_12, MD5_NUM_1200080426);
    c = md5Ff(c, d, a, b, x[j + MD5_NUM_6], MD5_NUM_17, MD5_NUM_NEGATIVE_1473231341);
    b = md5Ff(b, c, d, a, x[j + MD5_NUM_7], MD5_NUM_22, MD5_NUM_NEGATIVE_45705983);

    a = md5Ff(a, b, c, d, x[j + MD5_NUM_8], MD5_NUM_7, MD5_NUM_1770035416);
    d = md5Ff(d, a, b, c, x[j + MD5_NUM_9], MD5_NUM_12, MD5_NUM_NEGATIVE_1958414417);
    c = md5Ff(c, d, a, b, x[j + MD5_NUM_10], MD5_NUM_17, MD5_NUM_NEGATIVE_42063);
    b = md5Ff(b, c, d, a, x[j + MD5_NUM_11], MD5_NUM_22, MD5_NUM_NEGATIVE_1990404162);

    a = md5Ff(a, b, c, d, x[j + MD5_NUM_12], MD5_NUM_7, MD5_NUM_1804603682);
    d = md5Ff(d, a, b, c, x[j + MD5_NUM_13], MD5_NUM_12, MD5_NUM_NEGATIVE_40341101);
    c = md5Ff(c, d, a, b, x[j + MD5_NUM_14], MD5_NUM_17, MD5_NUM_NEGATIVE_1502002290);
    b = md5Ff(b, c, d, a, x[j + MD5_NUM_15], MD5_NUM_22, MD5_NUM_1236535329);

    a = md5Gg(a, b, c, d, x[j + 1], MD5_NUM_5, MD5_NUM_NEGATIVE_165796510);
    d = md5Gg(d, a, b, c, x[j + MD5_NUM_6], MD5_NUM_9, MD5_NUM_NEGATIVE_1069501632);
    c = md5Gg(c, d, a, b, x[j + MD5_NUM_11], MD5_NUM_14, MD5_NUM_643717713);
    b = md5Gg(b, c, d, a, x[j + 0], MD5_NUM_20, MD5_NUM_NEGATIVE_373897302);

    a = md5Gg(a, b, c, d, x[j + MD5_NUM_5], MD5_NUM_5, MD5_NUM_NEGATIVE_701558691);
    d = md5Gg(d, a, b, c, x[j + MD5_NUM_10], MD5_NUM_9, MD5_NUM_38016083);
    c = md5Gg(c, d, a, b, x[j + MD5_NUM_15], MD5_NUM_14, MD5_NUM_NEGATIVE_660478335);
    b = md5Gg(b, c, d, a, x[j + MD5_NUM_4], MD5_NUM_20, MD5_NUM_NEGATIVE_405537848);

    a = md5Gg(a, b, c, d, x[j + MD5_NUM_9], MD5_NUM_5, MD5_NUM_568446438);
    d = md5Gg(d, a, b, c, x[j + MD5_NUM_14], MD5_NUM_9, MD5_NUM_NEGATIVE_1019803690);
    c = md5Gg(c, d, a, b, x[j + MD5_NUM_3], MD5_NUM_14, MD5_NUM_NEGATIVE_187363961);
    b = md5Gg(b, c, d, a, x[j + MD5_NUM_8], MD5_NUM_20, MD5_NUM_1163531501);

    a = md5Gg(a, b, c, d, x[j + MD5_NUM_13], MD5_NUM_5, MD5_NUM_NEGATIVE_1444681467);
    d = md5Gg(d, a, b, c, x[j + MD5_NUM_2], MD5_NUM_9, MD5_NUM_NEGATIVE_51403784);
    c = md5Gg(c, d, a, b, x[j + MD5_NUM_7], MD5_NUM_14, MD5_NUM_1735328473);
    b = md5Gg(b, c, d, a, x[j + MD5_NUM_12], MD5_NUM_20, MD5_NUM_NEGATIVE_1926607734);

    a = md5Hh(a, b, c, d, x[j + MD5_NUM_5], MD5_NUM_4, MD5_NUM_NEGATIVE_378558);
    d = md5Hh(d, a, b, c, x[j + MD5_NUM_8], MD5_NUM_11, MD5_NUM_NEGATIVE_2022574463);
    c = md5Hh(c, d, a, b, x[j + MD5_NUM_11], MD5_NUM_16, MD5_NUM_1839030562);
    b = md5Hh(b, c, d, a, x[j + MD5_NUM_14], MD5_NUM_23, MD5_NUM_NEGATIVE_35309556);

    a = md5Hh(a, b, c, d, x[j + 1], MD5_NUM_4, MD5_NUM_NEGATIVE_1530992060);
    d = md5Hh(d, a, b, c, x[j + MD5_NUM_4], MD5_NUM_11, MD5_NUM_1272893353);
    c = md5Hh(c, d, a, b, x[j + MD5_NUM_7], MD5_NUM_16, MD5_NUM_NEGATIVE_155497632);
    b = md5Hh(b, c, d, a, x[j + MD5_NUM_10], MD5_NUM_23, MD5_NUM_NEGATIVE_1094730640);

    a = md5Hh(a, b, c, d, x[j + MD5_NUM_13], MD5_NUM_4, MD5_NUM_681279174);
    d = md5Hh(d, a, b, c, x[j + 0], MD5_NUM_11, MD5_NUM_NEGATIVE_358537222);
    c = md5Hh(c, d, a, b, x[j + MD5_NUM_3], MD5_NUM_16, MD5_NUM_NEGATIVE_722521979);
    b = md5Hh(b, c, d, a, x[j + MD5_NUM_6], MD5_NUM_23, MD5_NUM_76029189);

    a = md5Hh(a, b, c, d, x[j + MD5_NUM_9], MD5_NUM_4, MD5_NUM_NEGATIVE_640364487);
    d = md5Hh(d, a, b, c, x[j + MD5_NUM_12], MD5_NUM_11, MD5_NUM_NEGATIVE_421815835);
    c = md5Hh(c, d, a, b, x[j + MD5_NUM_15], MD5_NUM_16, MD5_NUM_530742520);
    b = md5Hh(b, c, d, a, x[j + MD5_NUM_2], MD5_NUM_23, MD5_NUM_NEGATIVE_995338651);

    a = md5Ii(a, b, c, d, x[j + 0], MD5_NUM_6, MD5_NUM_NEGATIVE_198630844);
    d = md5Ii(d, a, b, c, x[j + MD5_NUM_7], MD5_NUM_10, MD5_NUM_1126891415);
    c = md5Ii(c, d, a, b, x[j + MD5_NUM_14], MD5_NUM_15, MD5_NUM_NEGATIVE_1416354905);
    b = md5Ii(b, c, d, a, x[j + MD5_NUM_5], MD5_NUM_21, MD5_NUM_NEGATIVE_57434055);

    a = md5Ii(a, b, c, d, x[j + MD5_NUM_12], MD5_NUM_6, MD5_NUM_1700485571);
    d = md5Ii(d, a, b, c, x[j + MD5_NUM_3], MD5_NUM_10, MD5_NUM_NEGATIVE_1894986606);
    c = md5Ii(c, d, a, b, x[j + MD5_NUM_10], MD5_NUM_15, MD5_NUM_NEGATIVE_1051523);
    b = md5Ii(b, c, d, a, x[j + 1], MD5_NUM_21, MD5_NUM_NEGATIVE_2054922799);

    a = md5Ii(a, b, c, d, x[j + MD5_NUM_8], MD5_NUM_6, MD5_NUM_1873313359);
    d = md5Ii(d, a, b, c, x[j + MD5_NUM_15], MD5_NUM_10, MD5_NUM_NEGATIVE_30611744);
    c = md5Ii(c, d, a, b, x[j + MD5_NUM_6], MD5_NUM_15, MD5_NUM_NEGATIVE_1560198380);
    b = md5Ii(b, c, d, a, x[j + MD5_NUM_13], MD5_NUM_21, MD5_NUM_1309151649);

    a = md5Ii(a, b, c, d, x[j + MD5_NUM_4], MD5_NUM_6, MD5_NUM_NEGATIVE_145523070);
    d = md5Ii(d, a, b, c, x[j + MD5_NUM_11], MD5_NUM_10, MD5_NUM_NEGATIVE_1120210379);
    c = md5Ii(c, d, a, b, x[j + MD5_NUM_2], MD5_NUM_15, MD5_NUM_718787259);
    b = md5Ii(b, c, d, a, x[j + MD5_NUM_9], MD5_NUM_21, MD5_NUM_NEGATIVE_343485551);

    a = safeAdd(a, olda);
    b = safeAdd(b, oldb);
    c = safeAdd(c, oldc);
    d = safeAdd(d, oldd);

    j += MD5_NUM_16;
  }
  return [a, b, c, d];
}

/*
 * These funcs implement the four basic operations the algorithm uses.
 */
function md5Cmn(q: number, a: number, b: number, x: number, s: number, t: number): number {
  return safeAdd(bitRol(safeAdd(safeAdd(a, q), safeAdd(x, t)), s), b);
}
function md5Ff(a: number, b: number, c: number, d: number, x: number, s: number, t: number): number {
  return md5Cmn((b & c) | (~b & d), a, b, x, s, t);
}
function md5Gg(a: number, b: number, c: number, d: number, x: number, s: number, t: number): number {
  return md5Cmn((b & d) | (c & ~d), a, b, x, s, t);
}
function md5Hh(a: number, b: number, c: number, d: number, x: number, s: number, t: number): number {
  return md5Cmn(b ^ c ^ d, a, b, x, s, t);
}
function md5Ii(a: number, b: number, c: number, d: number, x: number, s: number, t: number): number {
  return md5Cmn(c ^ (b | ~d), a, b, x, s, t);
}

/*
 * Calculate the HMAC-MD5, of a key and some data
 */
function coreHmacMd5(key: string, data: string): Array<number> {
  let bkey = str2binl(key);
  if (bkey.length > MD5_NUM_16) {
    bkey = coreMd5(bkey, key.length * chrsz);
  }
  let toMax = MD5_NUM_15 - bkey.length;
  if (toMax >= 0) {
    bkey = bkey.concat(Array<number>(toMax + 1));
  }
  let ipad = Array<number>(MD5_NUM_16);
  let opad = Array<number>(MD5_NUM_16);
  for (let i = 0; i < MD5_NUM_16; i++) {
    ipad[i] = bkey[i] ^ MD5_NUM_0X36363636;
    opad[i] = bkey[i] ^ MD5_NUM_0X5C5C5C5C;
  }
  let hash = coreMd5(ipad.concat(str2binl(data)), MD5_NUM_512 + data.length * chrsz);
  return coreMd5(opad.concat(hash), MD5_NUM_512 + MD5_NUM_128);
}

/*
 * Add integers, wrapping at 2^32. This uses 16-bit operations internally
 * to work around bugs in some JS interpreters.
 */
function safeAdd(x: number, y: number): number {
  let lsw = (x & MD5_NUM_0XFFFF) + (y & MD5_NUM_0XFFFF);
  let msw = (x >> MD5_NUM_16) + (y >> MD5_NUM_16) + (lsw >> MD5_NUM_16);
  return (msw << MD5_NUM_16) | (lsw & MD5_NUM_0XFFFF);
}

/*
 * Bitwise rotate a 32-bit number to the left.
 */
function bitRol(num: number, cnt: number): number {
  return (num << cnt) | (num >>> (MD5_NUM_32 - cnt));
}

/*
 * Convert a string to an array of little-endian words
 * If chrsz is ASCII, characters >255 have their hi-byte silently ignored.
 */
function str2binl(str: string): Array<number> {
  let bin = Array<number>();
  let mask = (1 << chrsz) - 1;
  let toMax = ((str.length * chrsz - 1) >> MD5_NUM_5) - bin.length;
  if (toMax >= 0) {
    bin = bin.concat(Array<number>(toMax + 1));
  }

  let i = 0;
  while (i < str.length * chrsz) {
    bin[i >> MD5_NUM_5] |= (str.charCodeAt(i / chrsz) & mask) << i % MD5_NUM_32;
    i += chrsz;
  }
  log(`crypto-md5: str2binl = ${JSON.stringify(bin)}`);
  return bin;
}

/*
 * Convert an array of little-endian words to a string
 */
function binl2str(bin: number[]): string {
  let str = '';
  let mask = (1 << chrsz) - 1;
  for (let i = 0; i < bin.length * MD5_NUM_32; i += chrsz) {
    str += String.fromCharCode((bin[i >> MD5_NUM_5] >>> i % MD5_NUM_32) & mask);
  }
  return str;
}

/*
 * Convert an array of little-endian words to a hex string.
 */
function binl2hex(binarray: number[]): string {
  let hexTab = hexcase !== 0 ? '0123456789ABCDEF' : '0123456789abcdef';
  let str = '';

  for (let i = 0; i < binarray.length * MD5_NUM_4; i++) {
    str +=
      hexTab.charAt((binarray[i >> MD5_NUM_2] >> ((i % MD5_NUM_4) * MD5_NUM_8 + MD5_NUM_4)) & MD5_NUM_0XF) +
      hexTab.charAt((binarray[i >> MD5_NUM_2] >> ((i % MD5_NUM_4) * MD5_NUM_8)) & MD5_NUM_0XF);
  }
  return str;
}

/*
 * Convert an array of little-endian words to a base-64 string
 */
function binl2b64(binarray: number[]): string {
  let tab = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
  let str = '';
  let i = 0;

  while (i < binarray.length * MD5_NUM_4) {
    let triplet =
      (((getData(binarray, i >> MD5_NUM_2) >> (MD5_NUM_8 * (i % MD5_NUM_4))) & MD5_NUM_0XFF) << MD5_NUM_16) |
      (((getData(binarray, (i + 1) >> MD5_NUM_2) >> (MD5_NUM_8 * ((i + 1) % MD5_NUM_4))) & MD5_NUM_0XFF) << MD5_NUM_8) |
      ((getData(binarray, (i + MD5_NUM_2) >> MD5_NUM_2) >> (MD5_NUM_8 * ((i + MD5_NUM_2) % MD5_NUM_4))) & MD5_NUM_0XFF);
    for (let j = 0; j < MD5_NUM_4; j++) {
      if (i * MD5_NUM_8 + j * MD5_NUM_6 > binarray.length * MD5_NUM_32) {
        str += b64pad;
      } else {
        str += tab.charAt((triplet >> (MD5_NUM_6 * (MD5_NUM_3 - j))) & MD5_NUM_0X3F);
      }
    }
    i += MD5_NUM_3;
  }

  return str;
}
function getData(array: number[], index: number): number {
  if (index >= array.length) {
    return 0;
  }
  return array[index];
}

function run(): void {
  let plainText =
    "Rebellious subjects, enemies to peace,\nProfaners \
of this neighbour-stained steel,--\n\
Will they not hear? What, ho! you men, you beasts,\n\
That quench the fire of your pernicious rage\n\
With purple fountains issuing from your veins,\nOn \
pain of torture, from those bloody hands\n\
Throw your mistemper'd weapons to the ground,\n\
And hear the sentence of your moved prince.\n\
Three civil brawls, bred of an airy word,\nBy thee, \
old Capulet, and Montague,\n\
Have thrice disturb'd the quiet of our streets,\n\
And made Verona's ancient citizens\n\
Cast by their grave beseeming ornaments,\n\
To wield old partisans, in hands as old,\nCanker'd \
with peace, to part your canker'd hate:\n\
If ever you disturb our streets again,\n\
Your lives shall pay the forfeit of the peace.\n\
For this time, all the rest depart away:\nYou Capulet; \
shall go along with me:\n\
And, Montague, come you this afternoon,\nTo \
know our further pleasure in this case,\n\
To old Free-town, our common judgment-place.\nOnce more, \
on pain of death, all men depart.";

  for (let i = 0; i < MD5_NUM_4; i++) {
    plainText += plainText;
  }

  let md5Output = hexMd5(plainText);
  log(`crypto-md5: md5Output = ${md5Output}`);
  let expected = 'a831e91e0f70eddcb70dc61c6f82f6cd';
  if (md5Output !== expected) {
    throw new Error('ERROR: bad result: expected ' + expected + ' but got ' + md5Output);
  }
}
/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  /*
   * @Benchmark
   */
  runIteration(): void {
    for (let i = 0; i < MD5_NUM_22; ++i) {
      run();
    }
  }
}

function main() {
  let startTime = currentTimestamp13();
  let benchmark = new Benchmark();
  for (let i = 0; i < MD5_NUM_120; i++) {
    let startTimeInLoop = currentTimestamp13();
    benchmark.runIteration();
    let endTimeInLoop = currentTimestamp13();
  //log('crypto_md5: ms = ' +  (endTimeInLoop - startTimeInLoop) + ' i = ' + i)
  }
  let endTime = currentTimestamp13();
  print(`crypto-md5: ms = ${endTime - startTime}`);
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  main()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

main()

declare interface ArkTools {
  timeInUs(args: number): number;
}
