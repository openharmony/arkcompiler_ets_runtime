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
const AES_NUM_2: number = 2;
const AES_NUM_3: number = 3;
const AES_NUM_4: number = 4;
const AES_NUM_6: number = 6;
const AES_NUM_8: number = 8;
const AES_NUM_9: number = 9;
const AES_NUM_10: number = 10;
const AES_NUM_11: number = 11;
const AES_NUM_12: number = 12;
const AES_NUM_13: number = 13;
const AES_NUM_34: number = 34;
const AES_NUM_39: number = 39;
const AES_NUM_33: number = 33;
const AES_NUM_45: number = 45;
const AES_NUM_15: number = 15;
const AES_NUM_16: number = 16;
const AES_NUM_160: number = 160;
const AES_NUM_120: number = 20;

const AES_NUM_128: number = 128;
const AES_NUM_192: number = 192;
const AES_NUM_256: number = 256;

const AES_NUM_0X80: number = 0x80;
const AES_NUM_0X011B: number = 0x011b;
const AES_NUM_0XFF: number = 0xff;

const AES_NUM_0X100000000: number = 0x100000000;

const AES_NUM_INT32_MAX: number = 2147483647;
const AES_NUM_INT32_MIN: number = -2147483648;
const AES_NUM_UINT32_MAX: number = 4294967295;

let inDebug = false;
function log(str: string): void {
  if (inDebug) {
    print(str);
  }
}
function currentTimestamp13(): number {
  return ArkTools.timeInUs() / TIME_MULTIPLIER;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

/**
 ** AES Cipher function: encrypt 'input' with Rijndael algorithm
 *
 **   takes   byte-array 'input' (16 bytes)
 **           2D byte-array key schedule 'w' (Nr+1 x Nb bytes)
 *
 **   applies Nr rounds (10/12/14) using key schedule w for 'add round key' stage
 *
 **   returns byte-array encrypted value (16 bytes)
 **/
function cipher(input: Int16Array, w: Array<Int16Array>): Int16Array {
  // main Cipher function [§5.1]
  let nb = 4; // block size (in words): no of columns in state (fixed at 4 for AES)
  let nr = w.length / nb - 1; // no of rounds: 10/12/14 for 128/192/256-bit keys

  // initialise 4xNb byte-array 'state' with input [§3.4]
  let state = Array<Int16Array>(new Int16Array(nb), new Int16Array(nb), new Int16Array(nb), new Int16Array(nb));

  for (let i = 0; i < AES_NUM_4 * nb; i++) {
    state[i % AES_NUM_4][Math.floor(i / AES_NUM_4)] = input[i];
  }

  state = addRoundKey(state, w, 0, nb);
  for (let round = 1; round < nr; round++) {
    state = subBytes(state, nb);
    state = shiftRows(state, nb);
    state = mixColumns(state, nb);
    state = addRoundKey(state, w, round, nb);
  }

  state = subBytes(state, nb);
  state = shiftRows(state, nb);
  state = addRoundKey(state, w, nr, nb);

  let output = new Int16Array(AES_NUM_4 * nb); // convert state to 1-d array before returning [§3.4]
  for (let i = 0; i < AES_NUM_4 * nb; i++) {
    output[i] = state[i % AES_NUM_4][Math.floor(i / AES_NUM_4)];
  }
  return output;
}

function subBytes(s1: Array<Int16Array>, nb: number): Array<Int16Array> {
  // apply SBox to state S [§5.1.1]
  let s = s1;
  for (let r = 0; r < AES_NUM_4; r++) {
    for (let c = 0; c < nb; c++) {
      s[r][c] = SBOX[s[r][c]];
    }
  }
  return s;
}

function shiftRows(s1: Array<Int16Array>, nb: number): Array<Int16Array> {
  // shift row r of state S left by r bytes [§5.1.2]
  let s = s1;
  let t = new Int16Array(AES_NUM_4);
  for (let r = 1; r < AES_NUM_4; r++) {
    for (let c = 0; c < AES_NUM_4; c++) {
      t[c] = s[r][(c + r) % nb]; // shift into temp copy
    }
    for (let c = 0; c < AES_NUM_4; c++) {
      s[r][c] = t[c]; // and copy back
    }
  } // note that this will work for Nb=4,5,6, but not 7,8 (always 4 for AES):
  return s; // see fp.gladman.plus.com/cryptography_technology/rijndael/aes.spec.311.pdf
}

function mixColumns(s1: Array<Int16Array>, nb: number): Array<Int16Array> {
  // combine bytes of each col of state S [§5.1.3]
  let s = s1;
  for (let c = 0; c < AES_NUM_4; c++) {
    let a = Array.from({ length: 4 }, () => 0); // 'a' is a copy of the current column from 's'
    let b = Array.from({ length: 4 }, () => 0); // 'b' is a•{02} in GF(2^8)
    for (let i = 0; i < AES_NUM_4; i++) {
      a[i] = s[i][c];
      b[i] = (s[i][c] & AES_NUM_0X80) !== 0 ? (s[i][c] << 1) ^ AES_NUM_0X011B : s[i][c] << 1;
    }
    // a[n] ^ b[n] is a•{03} in GF(2^8)
    s[0][c] = b[0] ^ a[1] ^ b[1] ^ a[AES_NUM_2] ^ a[AES_NUM_3]; // 2*a0 + 3*a1 + a2 + a3
    s[1][c] = a[0] ^ b[1] ^ a[AES_NUM_2] ^ b[AES_NUM_2] ^ a[AES_NUM_3]; // a0 * 2*a1 + 3*a2 + a3
    s[AES_NUM_2][c] = a[0] ^ a[1] ^ b[AES_NUM_2] ^ a[AES_NUM_3] ^ b[AES_NUM_3]; // a0 + a1 + 2*a2 + 3*a3
    s[AES_NUM_3][c] = a[0] ^ b[0] ^ a[1] ^ a[AES_NUM_2] ^ b[AES_NUM_3]; // 3*a0 + a1 + a2 + 2*a3
  }
  return s;
}

function addRoundKey(state1: Array<Int16Array>, w: Array<Int16Array>, rnd: number, nb: number): Array<Int16Array> {
  // xor Round Key into state S [§5.1.4]
  let state = state1;
  for (let r = 0; r < AES_NUM_4; r++) {
    for (let c = 0; c < nb; c++) {
      state[r][c] ^= w[rnd * AES_NUM_4 + c][r];
    }
  }
  return state;
}

function keyExpansion(key1: Int16Array): Array<Int16Array> {
  // generate Key Schedule (byte-array Nr+1 x Nb) from Key [§5.2]
  let key = key1;
  let nb = 4; // block size (in words): no of columns in state (fixed at 4 for AES)
  let nk = key.length / AES_NUM_4; // key length (in words): 4/6/8 for 128/192/256-bit keys
  let nr = nk + AES_NUM_6; // no of rounds: 10/12/14 for 128/192/256-bit keys

  let w = new Array<Int16Array>(nb * (nr + 1));
  let temp = new Int16Array(AES_NUM_4);

  for (let i = 0; i < nk; i++) {
    let r = new Int16Array([key[AES_NUM_4 * i], key[AES_NUM_4 * i + 1], key[AES_NUM_4 * i + AES_NUM_2], key[AES_NUM_4 * i + AES_NUM_3]]);
    w[i] = r;
  }
  for (let i = nk; i < nb * (nr + 1); i++) {
    w[i] = new Int16Array(AES_NUM_4);
    for (let t = 0; t < AES_NUM_4; t++) {
      temp[t] = w[i - 1][t];
    }
    if (i % nk === 0) {
      temp = subWord(rotWord(temp));
      for (let t = 0; t < AES_NUM_4; t++) {
        temp[t] ^= RCON[i / nk][t];
      }
    } else if (nk > AES_NUM_6 && i % nk === AES_NUM_4) {
      temp = subWord(temp);
    }
    for (let t = 0; t < AES_NUM_4; t++) {
      w[i][t] = w[i - nk][t] ^ temp[t];
    }
  }
  return w;
}

function subWord(w1: Int16Array): Int16Array {
  // apply SBox to 4-byte word w
  let w = w1;
  for (let i = 0; i < AES_NUM_4; i++) {
    w[i] = SBOX[w[i]];
  }
  return w;
}

function rotWord(w1: Int16Array): Int16Array {
  // rotate 4-byte word w left by one byte
  let w = w1;
  let toMax = AES_NUM_4 - w.length;
  if (toMax >= 0) {
    w = new Int16Array([...w, ...Array.from({ length: toMax + 1 }, () => 0)]);
  }
  w[AES_NUM_4] = w[0];
  for (let i = 0; i < AES_NUM_4; i++) {
    w[i] = w[i + 1];
  }
  return w;
}

// Sbox is pre-computed multiplicative inverse in GF(2^8) used in SubBytes and KeyExpansion [§5.1.1]
const SBOX = [0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
  0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, //Sbox[16-1]
  0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
  0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, //Sbox[16-3]
  0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
  0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, //Sbox[16-5]
  0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
  0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, //Sbox[16-7]
  0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
  0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, //Sbox[16-9]
  0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
  0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08, //Sbox[16-11]
  0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
  0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e, //Sbox[16-13]
  0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
  0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16]; 

// RCON is Round Constant used for the Key Expansion [1st col is 2^(r-1) in GF(2^8)] [§5.2]
const RCON = [
  [0x00, 0x00, 0x00, 0x00],
  [0x01, 0x00, 0x00, 0x00], //RCON[1]
  [0x02, 0x00, 0x00, 0x00],
  [0x04, 0x00, 0x00, 0x00], //RCON[3]
  [0x08, 0x00, 0x00, 0x00],
  [0x10, 0x00, 0x00, 0x00], //RCON[5]
  [0x20, 0x00, 0x00, 0x00],
  [0x40, 0x00, 0x00, 0x00], //RCON[7]
  [0x80, 0x00, 0x00, 0x00],
  [0x1b, 0x00, 0x00, 0x00], //RCON[9]
  [0x36, 0x00, 0x00, 0x00]
];

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

/*
 * Use AES to encrypt 'plaintext' with 'password' using 'nBits' key, in 'Counter' mode of operation
 *                           - see http://csrc.nist.gov/publications/nistpubs/800-38a/sp800-38a.pdf;
 *   for each block;
 *   - outputblock = cipher(counter, key);
 *   - cipherblock = plaintext xor outputblock;
 */
function aesEncryptCtr(plaintext: string, password: string, nBits: number): string {
  if (!(nBits === AES_NUM_128 || nBits === AES_NUM_192 || nBits === AES_NUM_256)) {
    return '';
  } // standard allows 128/192/256 bit keys

  // for this example script, generate the key by applying Cipher to 1st 16/24/32 chars of password;
  // for real-world applications, a more secure approach would be to hash the password e.g. with SHA-1
  let nBytes = nBits / AES_NUM_8; // no bytes in key
  let pwBytes = new Int16Array(nBytes);
  for (let i = 0; i < nBytes; i++) {
    pwBytes[i] = getASCCode(password, i) & AES_NUM_0XFF;
  }

  let key = cipher(pwBytes, keyExpansion(pwBytes));
  key = new Int16Array([...key, ...key.slice(0, nBytes - AES_NUM_16)]); // key is now 16/24/32 bytes long

  // initialise counter block (NIST SP800-38A §B.2): millisecond time-stamp for nonce in 1st 8 bytes,
  // block counter in 2nd 8 bytes
  const blockSize = 16; // block size fixed at 16 bytes / 128 bits (Nb=4) for AES
  const counterBlock = new Int16Array(blockSize); //Array.from({length: blockSize}, ()=>0)  // block size fixed at 16 bytes / 128 bits (Nb=4) for AES
  const nonce = Math.round(currentTimestamp13()); // milliseconds since 1-Jan-1970

  // encode nonce in two stages to cater for JavaScript 32-bit limit on bitwise ops
  for (let i = 0; i < AES_NUM_4; i++) {
    counterBlock[i] = (transBigInt32(nonce) >>> (i * AES_NUM_8)) & AES_NUM_0XFF;
  }
  for (let i = 0; i < AES_NUM_4; i++) {
    counterBlock[i + AES_NUM_4] = ((transBigInt32(nonce) / AES_NUM_0X100000000) >>> (i * AES_NUM_8)) & AES_NUM_0XFF;
  }

  // generate key schedule - an expansion of the key into distinct Key Rounds for each round
  let keySchedule = keyExpansion(key);
  //log(`crypto-aes: keySchedule = ${JSON.stringify(keySchedule)}`)
  let blockCount = Math.ceil(plaintext.length / blockSize);
  let ciphertext = new Array<string>(blockCount); // ciphertext as array of strings
  for (let b = 0; b < blockCount; b++) {
    // set counter (block #) in last 8 bytes of counter block (leaving nonce in 1st 8 bytes)
    // again done in two stages for 32-bit ops
    for (let c = 0; c < AES_NUM_4; c++) {
      counterBlock[AES_NUM_15 - c] = (b >>> (c * AES_NUM_8)) & AES_NUM_0XFF;
    }
    for (let c = 0; c < AES_NUM_4; c++) {
      counterBlock[AES_NUM_15 - c - AES_NUM_4] = (b / AES_NUM_0X100000000) >>> (c * AES_NUM_8);
    }

    let cipherCntr = cipher(counterBlock, keySchedule); // -- encrypt counter block --

    // calculate length of final block:
    let blockLength = b < blockCount - 1 ? blockSize : ((plaintext.length - 1) % blockSize) + 1;

    let ct = '';
    for (let i = 0; i < blockLength; i++) {
      // -- xor plaintext with ciphered counter byte-by-byte --
      let plaintextByte = getASCCode(plaintext, b * blockSize + i);
      let cipherByte = plaintextByte ^ cipherCntr[i];
      ct += String.fromCharCode(cipherByte);
    }
    // ct is now ciphertext for this block
    ciphertext[b] = escCtrlChars(ct); // escape troublesome characters in ciphertext
  }

  // convert the nonce to a string to go on the front of the ciphertext
  let ctrTxt = '';
  for (let i = 0; i < AES_NUM_8; i++) {
    ctrTxt += String.fromCharCode(counterBlock[i]);
  }
  ctrTxt = escCtrlChars(ctrTxt);

  // use '-' to separate blocks, use Array.join to concatenate arrays of strings for efficiency;
  return ctrTxt + '-' + ciphertext.join('-');
}

/*
 * Use AES to decrypt 'ciphertext' with 'password' using 'nBits' key, in Counter mode of operation;
 *
 *   for each block;
 *   - outputblock = cipher(counter, key);
 *   - cipherblock = plaintext xor outputblock;
 */
function aesDecryptCtr(ciphertext: string, password: string, nBits: number): string {
  if (!(nBits === AES_NUM_128 || nBits === AES_NUM_192 || nBits === AES_NUM_256)) {
    return '';
  } // standard allows 128/192/256 bit keys
  let nBytes = nBits / AES_NUM_8; // no bytes in key
  let pwBytes = new Int16Array(nBytes);

  for (let i = 0; i < nBytes; i++) {
    pwBytes[i] = getASCCode(password, i) & AES_NUM_0XFF;
  }

  let pwKeySchedule = keyExpansion(pwBytes);
  let key = cipher(pwBytes, pwKeySchedule);
  key = new Int16Array([...key, ...key.slice(0, nBytes - AES_NUM_16)]); // key is now 16/24/32 bytes long
  let keySchedule = keyExpansion(key);
  let ciphertextArray: string[] = ciphertext.split('-'); // split ciphertext into array of block-length strings

  // recover nonce from 1st element of ciphertext
  let blockSize = 16; // block size fixed at 16 bytes / 128 bits (Nb=4) for AES
  let counterBlock = new Int16Array(blockSize); //Array.from({length: blockSize}, ()=>0)
  let ctrTxt = unescCtrlChars(ciphertextArray[0]);
  for (let i = 0; i < AES_NUM_8; i++) {
    counterBlock[i] = getASCCode(ctrTxt, i);
  }
  let plaintext = Array.from({ length: ciphertextArray.length - 1 }, () => '');

  for (let b = 1; b < ciphertextArray.length; b++) {
    // set counter (block #) in last 8 bytes of counter block (leaving nonce in 1st 8 bytes)
    for (let c = 0; c < AES_NUM_4; c++) {
      counterBlock[AES_NUM_15 - c] = ((b - 1) >>> (c * AES_NUM_8)) & AES_NUM_0XFF;
    }
    for (let c = 0; c < AES_NUM_4; c++) {
      counterBlock[AES_NUM_15 - c - AES_NUM_4] = ((b / AES_NUM_0X100000000 - 1) >>> (c * AES_NUM_8)) & AES_NUM_0XFF;
    }

    let cipherCntr = cipher(counterBlock, keySchedule); // encrypt counter block

    ciphertextArray[b] = unescCtrlChars(ciphertextArray[b]);

    let pt = '';
    for (let i = 0; i < ciphertextArray[b].length; i++) {
      // -- xor plaintext with ciphered counter byte-by-byte --
      let ciphertextByte = getASCCode(ciphertextArray[b], i);
      let plaintextByte = ciphertextByte ^ cipherCntr[i];
      pt += String.fromCharCode(plaintextByte);
    }
    // pt is now plaintext for this block

    plaintext[b - 1] = pt; // b-1 'cos no initial nonce block in plaintext
  }

  return plaintext.join('');
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  */

function escCtrlChars(str: string): string {
  // escape control chars which might cause problems handling ciphertext
  let tmpsArr = [0, AES_NUM_9, AES_NUM_10, AES_NUM_11, AES_NUM_12, AES_NUM_13, AES_NUM_160, AES_NUM_39, AES_NUM_34, AES_NUM_33, AES_NUM_45];
  return Array.from(str)
    .map(c => {
      let asciiCode = c.charCodeAt(0);
      if (tmpsArr.includes(asciiCode)) {
        return '!' + asciiCode + '!';
      }
      return c;
    })
    .reduce((a, b) => a + b, '');
} // \xa0 to cater for bug in Firefox; include '-' to leave it free for use as a block marker

function unescCtrlChars(str: string): string {
  // unescape potentially problematic control characters
  return str.replace(/!\d\d?\d?!/g, function (c) {
    return String.fromCharCode(parseInt(c.slice(1, -1)));
  });
}

function getASCCode(str: String, at: number): number {
  return str.charCodeAt(at);
}

function transBigInt32(bigInt32Num: number): number {
  let tmp = bigInt32Num;
  if (tmp > AES_NUM_INT32_MAX) {
    let max = AES_NUM_UINT32_MAX;
    tmp = tmp % (max + 1);
    if (tmp > AES_NUM_INT32_MAX) {
      tmp = tmp - max - 1;
    }
  } else if (tmp < AES_NUM_INT32_MIN) {
    let max = AES_NUM_UINT32_MAX;
    tmp = tmp % (max + 1);
    if (tmp < AES_NUM_INT32_MIN) {
      tmp = tmp + max + 1;
    }
  }
  return tmp;
}
function run(): void {
  let plainText =
    'ROMEO: But, soft! what light through yonder window breaks?\nIt \
  is the east, and Juliet is the sun.\n\
  Arise, fair sun, and kill the envious moon,\n\
  Who is already sick and pale with grief,\n\
  That thou her maid art far more fair than she:\nBe \
  not her maid, since she is envious;\n\
  Her vestal livery is but sick and green\n\
  And none but fools do wear it; cast it off.\nIt \
  is my lady, O, it is my love!\n\
  O, that she knew she were!\n\
  She speaks yet she says nothing: what of that?\nHer \
  eye discourses; I will answer it.\n\
  I am too bold, \'tis not to me she speaks:\n\
  Two of the fairest stars in all the heaven,\nHaving \
  some business, do entreat her eyes\n\
  To twinkle in their spheres till they return.\n\
  What if her eyes were there, they in her head?\nThe \
  brightness of her cheek would shame those stars,\n\
  As daylight doth a lamp; her eyes in heaven\n\
  Would through the airy region stream so bright\n\
  That birds would sing and think it were not night.\nSee, \
  how she leans her cheek upon her hand!\n\
  O, that I were a glove upon that hand,\n\
  That I might touch that cheek!\nJULIET: \
  Ay me!\n\
  ROMEO: She speaks:\n\
  O, speak again, bright angel! for thou art\n\
  As glorious to this night, being o\'er my head\n\
  As is a winged messenger of heaven\n\
  Unto the white-upturned wondering eyes\nOf \
  mortals that fall back to gaze on him\n\
  When he bestrides the lazy-pacing clouds\nAnd \
  sails upon the bosom of the air.';

  let password = 'O Romeo, Romeo! wherefore art thou Romeo?';

  let cipherText = aesEncryptCtr(plainText, password, AES_NUM_256);
  //log(`crypto-aes: cipherText = ${cipherText}`)
  let decryptedText = aesDecryptCtr(cipherText, password, AES_NUM_256);

  if (decryptedText !== plainText) {
    throw new Error(`ERROR: bad result: expected ${plainText} but got ${decryptedText}`);
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
    for (let i = 0; i < AES_NUM_8; ++i) {
      run();
    }
  }
}

function main() {
  let startTime = currentTimestamp13();
  let benchmark = new Benchmark();
  for (let i = 0; i < AES_NUM_120; i++) {
    let startTimeInLoop = currentTimestamp13();
    benchmark.runIteration();
    let endTimeInLoop = currentTimestamp13();
    // log(`crypto-aes: ms = ${endTimeInLoop - startTimeInLoop} i = ${i}`)
  }
  let endTime = currentTimestamp13();
  print(`crypto-aes: ms = ${endTime - startTime}`);
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
