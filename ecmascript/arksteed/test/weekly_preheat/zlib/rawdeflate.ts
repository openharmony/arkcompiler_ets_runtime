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

import {
  BitStream,
  replaceNumberCount,
  replace8NumberCount,
  replace16NumberCount,
  replace32NumberCount
} from './bitstream';
import { Heap, PopObject } from './heap';

const Number_2 = 2;
const Number_3 = 3;
const Number_4 = 4;
const Number_5 = 5;
const Number_6 = 6;
const Number_7 = 7;
const Number_8 = 8;
const Number_9 = 9;
const Number_10 = 10;
const Number_11 = 11;
const Number_12 = 12;
const Number_14 = 14;
const Number_15 = 15;
const Number_16 = 16;
const Number_17 = 17;
const Number_18 = 18;
const Number_19 = 19;
const Number_22 = 22;
const Number_24 = 24;
const Number_26 = 26;
const Number_30 = 30;
const Number_32 = 32;
const Number_34 = 34;
const Number_42 = 42;
const Number_48 = 48;
const Number_50 = 50;
const Number_58 = 58;
const Number_64 = 64;
const Number_66 = 66;
const Number_82 = 82;
const Number_96 = 96;
const Number_98 = 98;
const Number_114 = 114;
const Number_128 = 128;
const Number_130 = 130;
const Number_138 = 138;
const Number_143 = 143;
const Number_162 = 162;
const Number_192 = 192;
const Number_194 = 194;
const Number_226 = 226;
const Number_255 = 255;
const Number_256 = 256;
const Number_257 = 257;
const Number_258 = 258;
const Number_279 = 279;
const Number_286 = 286;
const Number_287 = 287;
const Number_288 = 288;
const Number_384 = 384;
const Number_512 = 512;
const Number_768 = 768;
const Number_1024 = 1024;
const Number_1536 = 1536;
const Number_2048 = 2048;
const Number_3072 = 3072;
const Number_4096 = 4096;
const Number_6144 = 6144;
const Number_8192 = 8192;
const Number_12288 = 12288;
const Number_16384 = 16384;
const Number_24576 = 24576;
const Number_32768 = 32768;

export function replacePopObjectArray(count: number, element: PopObject): Array<PopObject> {
  let result: PopObject[] = new Array<PopObject>();
  for (let i = 0; i < count; i++) {
    result.push(element);
  }
  return result;
}

export class OptionParams {
  lazy?: number;
  compressionType?: CompressionType;
  outputBuffer?: Uint8Array;
  outputIndex?: number;
}

interface TreeSymbols {
  codes: Uint32Array;
  freqs: Uint8Array;
}

export enum CompressionType {
  NONE = 0,
  FIXED = 1,
  DYNAMIC = Number_2,
  RESERVED = Number_3
}

export class Lz77Match {
  public length: number;
  public backwardDistance: number;

  constructor(length: number, backwardDistance: number) {
    this.length = length;
    this.backwardDistance = backwardDistance;
  }

  static get LengthCodeTable(): Uint32Array {
    const code = (length: number): number[] => {
      switch (true) {
        case length === Number_3:
          return [257, length - 3, 0];
        case length === Number_4:
          return [258, length - 4, 0];
        case length === Number_5:
          return [259, length - 5, 0];
        case length === Number_6:
          return [260, length - 6, 0];
        case length === Number_7:
          return [261, length - 7, 0];
        case length === Number_8:
          return [262, length - 8, 0];
        case length === Number_9:
          return [263, length - 9, 0];
        case length === Number_10:
          return [264, length - 10, 0];
        case length <= Number_12:
          return [265, length - 11, 1];
        case length <= Number_14:
          return [266, length - 13, 1];
        case length <= Number_16:
          return [267, length - 15, 1];
        case length <= Number_18:
          return [268, length - 17, 1];
        case length <= Number_22:
          return [269, length - 19, 2];
        case length <= Number_26:
          return [270, length - 23, 2];
        case length <= Number_30:
          return [271, length - 27, 2];
        case length <= Number_34:
          return [272, length - 31, 2];
        case length <= Number_42:
          return [273, length - 35, 3];
        case length <= Number_50:
          return [274, length - 43, 3];
        case length <= Number_58:
          return [275, length - 51, 3];
        case length <= Number_66:
          return [276, length - 59, 3];
        case length <= Number_82:
          return [277, length - 67, 4];
        case length <= Number_98:
          return [278, length - 83, 4];
        case length <= Number_114:
          return [279, length - 99, 4];
        case length <= Number_130:
          return [280, length - 115, 4];
        case length <= Number_162:
          return [281, length - 131, 5];
        case length <= Number_194:
          return [282, length - 163, 5];
        case length <= Number_226:
          return [283, length - 195, 5];
        case length <= Number_257:
          return [284, length - 227, 5];
        case length === Number_258:
          return [285, length - 258, 0];
        default:
          throw new Error(`invalid length: ${length}`);
      }
    };

    let table: Uint32Array = replace32NumberCount(Number_258);
    let c: number[] = [];

    for (let i = 3; i <= Number_258; i++) {
      c = code(i);
      table[i] = (c[Number_2] << Number_24) | (c[1] << Number_16) | c[0];
    }

    return table;
  }

  public getDistanceCode_(dist: number): number[] {
    /** @type {!Array.<number>} distance code table. */
    let r: number[];
    switch (true) {
      case dist === 1:
        r = [0, dist - 1, 0];
        break;
      case dist === Number_2:
        r = [1, dist - 2, 0];
        break;
      case dist === Number_3:
        r = [2, dist - 3, 0];
        break;
      case dist === Number_4:
        r = [3, dist - 4, 0];
        break;
      case dist <= Number_6:
        r = [4, dist - 5, 1];
        break;
      case dist <= Number_8:
        r = [5, dist - 7, 1];
        break;
      case dist <= Number_12:
        r = [6, dist - 9, 2];
        break;
      case dist <= Number_16:
        r = [7, dist - 13, 2];
        break;
      case dist <= Number_24:
        r = [8, dist - 17, 3];
        break;
      case dist <= Number_32:
        r = [9, dist - 25, 3];
        break;
      case dist <= Number_48:
        r = [10, dist - 33, 4];
        break;
      case dist <= Number_64:
        r = [11, dist - 49, 4];
        break;
      case dist <= Number_96:
        r = [12, dist - 65, 5];
        break;
      case dist <= Number_128:
        r = [13, dist - 97, 5];
        break;
      case dist <= Number_192:
        r = [14, dist - 129, 6];
        break;
      case dist <= Number_256:
        r = [15, dist - 193, 6];
        break;
      case dist <= Number_384:
        r = [16, dist - 257, 7];
        break;
      case dist <= Number_512:
        r = [17, dist - 385, 7];
        break;
      case dist <= Number_768:
        r = [18, dist - 513, 8];
        break;
      case dist <= Number_1024:
        r = [19, dist - 769, 8];
        break;
      case dist <= Number_1536:
        r = [20, dist - 1025, 9];
        break;
      case dist <= Number_2048:
        r = [21, dist - 1537, 9];
        break;
      case dist <= Number_3072:
        r = [22, dist - 2049, 10];
        break;
      case dist <= Number_4096:
        r = [23, dist - 3073, 10];
        break;
      case dist <= Number_6144:
        r = [24, dist - 4097, 11];
        break;
      case dist <= Number_8192:
        r = [25, dist - 6145, 11];
        break;
      case dist <= Number_12288:
        r = [26, dist - 8193, 12];
        break;
      case dist <= Number_16384:
        r = [27, dist - 12289, 12];
        break;
      case dist <= Number_24576:
        r = [28, dist - 16385, 13];
        break;
      case dist <= Number_32768:
        r = [29, dist - 24577, 13];
        break;
      default:
        throw new Error('invalid distance');
    }
    return r;
  }

  public toLz77Array(): number[] {
    let length: number = this.length;
    let dist: number = this.backwardDistance;
    let codeArray: number[] = [];
    let pos: number = 0;
    let code: number;

    // length
    code = Lz77Match.LengthCodeTable[length];
    codeArray[pos] = code & 0xffff;
    codeArray[pos + 1] = (code >> Number_16) & 0xff;
    codeArray[pos + Number_2] = code >> Number_24;
    pos += Number_3;
    // distance
    let distCode = this.getDistanceCode_(dist);
    codeArray[pos] = distCode[0];
    codeArray[pos + 1] = distCode[1];
    codeArray[pos + Number_2] = distCode[Number_2];

    return codeArray;
  }
}

/**
 * @Generator
 */
export class RawDeflate {
  public compressionType: CompressionType;
  public lazy: number = 0;
  public freqsLitLen: Uint32Array = new Uint32Array(0);
  public freqsDist: Uint32Array = new Uint32Array(0);
  public input: Uint8Array = new Uint8Array(0);
  public output: Uint8Array = new Uint8Array(0);
  public op: number = 0;
  public length: number = 0;
  public backwardDistance: number;
  public static Lz77MaxLength: number = Number_258;
  public static WindowSize: number = 0x8000;
  public static MaxCodeLength: number = Number_16;
  public static HUFMAX: number = Number_286;
  public static Lz77MinLength: number = Number_3;

  constructor(input: Uint8Array, opt_params?: OptionParams) {
    this.compressionType = CompressionType.DYNAMIC;
    this.lazy = 0;
    this.length = 0;
    this.backwardDistance = 0;
    this.input = input;
    this.op = 0;
    // option parameters
    if (opt_params != null) {
      if (opt_params.lazy != null) {
        this.lazy = opt_params.lazy;
      }
      if (typeof opt_params.compressionType === 'number') {
        this.compressionType = opt_params.compressionType;
      }
      if (opt_params.outputBuffer != null) {
        this.output = opt_params.outputBuffer;
      }
      if (typeof opt_params.outputIndex === 'number') {
        this.op = opt_params.outputIndex;
      }
    }

    this.output = replace8NumberCount(0x8000);
  }

  public static get FixedHuffmanTable(): number[][] {
    let table: number[][] = [];

    for (let i = 0; i < Number_288; i++) {
      switch (true) {
        case i <= Number_143:
          table.push([i + 0x030, 8]);
          break;
        case i <= Number_255:
          table.push([i - 144 + 0x190, 9]);
          break;
        case i <= Number_279:
          table.push([i - 256 + 0x000, 7]);
          break;
        case i <= Number_287:
          table.push([i - 280 + 0x0c0, 8]);
          break;
        default:
          throw new Error(`invalid literal: ${i}`);
      }
    }
    return table;
  }

  public compress(): Uint8Array {
    let blockArray: Uint8Array;
    let input: Uint8Array = this.input;

    switch (this.compressionType) {
      case CompressionType.NONE:
        // each 65535-Byte (length header: 16-bit)
        let position: number = 0;
        let length: number = input.length;
        while (position < length) {
          blockArray = input.subarray(position, position + 0xffff);
          position += blockArray.length;
          this.makeNocompressBlock(blockArray, position === length);
        }
        break;
      case CompressionType.FIXED:
        this.output = this.makeFixedHuffmanBlock(input, true);
        this.op = this.output.length;
        break;
      case CompressionType.DYNAMIC:
        this.output = this.makeDynamicHuffmanBlock(input, true);
        this.op = this.output.length;
        break;
      default:
        throw new Error('invalid compression type');
    }
    return this.output;
  }

  public makeNocompressBlock(blockArray: Uint8Array, isFinalBlock: boolean): Uint8Array {
    let bfinal: number;
    let btype: CompressionType;
    let len: number;
    let nlen: number;

    let output: Uint8Array = this.output;
    let op: number = this.op;

    // expand buffer

    output = new Uint8Array(this.output.buffer);
    while (output.length <= op + blockArray.length + Number_5) {
      output = replace8NumberCount(output.length << 1);
    }
    output.set(this.output);

    // header
    bfinal = isFinalBlock ? 1 : 0;
    btype = CompressionType.NONE;
    output[op] = bfinal | (btype << 1);

    // length
    len = blockArray.length;
    nlen = (~len + 0x10000) & 0xffff;
    output[op + 1] = len & 0xff;
    output[op + Number_2] = (len >>> Number_8) & 0xff;
    output[op + Number_3] = nlen & 0xff;
    output[op + Number_4] = (nlen >>> Number_8) & 0xff;
    op += Number_5;
    // copy buffer

    output.set(blockArray, op);
    op += blockArray.length;
    output = output.subarray(0, op);

    this.op = op;
    this.output = output;

    return output;
  }

  public makeFixedHuffmanBlock(blockArray: Uint8Array, isFinalBlock: boolean): Uint8Array {
    let stream: BitStream = new BitStream(new Uint8Array(this.output.buffer), this.op);
    let bfinal: number = 0;
    let btype: CompressionType = CompressionType.FIXED;
    let data: Uint16Array;

    // header
    bfinal = isFinalBlock ? 1 : 0;

    stream.writeBits(bfinal, 1, true);
    stream.writeBits(btype, Number_2, true);

    data = this.lz77(blockArray);
    this.fixedHuffman(data, stream);

    return stream.finish();
  }

  public makeDynamicHuffmanBlock(blockArray: Uint8Array, isFinalBlock: boolean): Uint8Array {
    let stream = new BitStream(this.output, this.op);
    let bfinal: number;
    let btype: CompressionType;
    let data: Uint16Array;
    let hlit: number = 0;
    let hdist: number = 0;
    let hclen: number = 0;
    let hclenOrder = [16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15];
    let litLenLengths: Uint8Array;
    let litLenCodes: Uint16Array;
    let distLengths: Uint8Array;
    let distCodes: Uint16Array;
    let treeSymbols: TreeSymbols;
    let treeLengths: Uint8Array;
    let transLengths: number[] = replaceNumberCount(Number_19);
    let treeCodes: Uint16Array;
    let code: number;
    let bitlen: number;

    // header
    bfinal = isFinalBlock ? 1 : 0;
    btype = CompressionType.DYNAMIC;

    stream.writeBits(bfinal, 1, true);
    stream.writeBits(btype, Number_2, true);

    data = this.lz77(blockArray);

    litLenLengths = this.getLengths_(this.freqsLitLen, Number_15);
    litLenCodes = this.getCodesFromLengths_(litLenLengths);
    distLengths = this.getLengths_(this.freqsDist, Number_7);
    distCodes = this.getCodesFromLengths_(distLengths);

    for (hlit = Number_286; hlit > Number_257; hlit--) {
      if (litLenLengths[hlit - 1] === 0) {
        break;
      }
    }
    for (hdist = Number_30; hdist > 1; hdist--) {
      if (distLengths[hdist - 1] === 0) {
        break;
      }
    }

    // HCLEN
    treeSymbols = this.getTreeSymbols_(hlit, litLenLengths, hdist, distLengths);
    treeLengths = this.getLengths_(treeSymbols.freqs, Number_7);
    for (let i = 0; i < Number_19; i++) {
      transLengths[i] = treeLengths[hclenOrder[i]];
    }
    for (hclen = Number_19; hclen > Number_4; hclen--) {
      if (transLengths[hclen - 1] === 0) {
        break;
      }
    }

    treeCodes = this.getCodesFromLengths_(treeLengths);

    stream.writeBits(hlit - Number_257, Number_5, true);
    stream.writeBits(hdist - 1, Number_5, true);
    stream.writeBits(hclen - Number_4, Number_4, true);
    for (let i = 0; i < hclen; i++) {
      stream.writeBits(transLengths[i], Number_3, true);
    }

    let i: number = 0;
    let il: number = treeSymbols.codes.length;
    while (i < il) {
      code = treeSymbols.codes[i];

      stream.writeBits(treeCodes[code], treeLengths[code], true);
      // extra bits
      if (code >= Number_16) {
        i += 1;
        switch (code) {
          case Number_16:
            bitlen = Number_2;
            break;
          case Number_17:
            bitlen = Number_3;
            break;
          case Number_18:
            bitlen = Number_7;
            break;
          default:
            throw new Error(`invalid code: ${code}`);
        }

        stream.writeBits(treeSymbols.codes[i], bitlen, true);
      }

      i += 1;
    }

    this.dynamicHuffman(data, [litLenCodes, litLenLengths], [distCodes, distLengths], stream);

    return stream.finish();
  }

  public dynamicHuffman(
    dataArray: Uint16Array,
    litLen: [Uint16Array, Uint8Array],
    dist: [Uint16Array, Uint8Array],
    stream: BitStream
  ): BitStream {
    let index: number = 0;
    let literal: number;
    let code: number;
    let litLenCodes: Uint16Array = litLen[0];
    let litLenLengths: Uint8Array = litLen[1];
    let distCodes: Uint16Array = dist[0];
    let distLengths: Uint8Array = dist[1];

    //  write symbols to BitStream
    while (index < dataArray.length) {
      literal = dataArray[index];

      // literal or length
      stream.writeBits(litLenCodes[literal], litLenLengths[literal], true);

      // length/distance symbol
      if (literal > Number_256) {
        // length extra
        stream.writeBits(dataArray[index + 1], dataArray[index + Number_2], true);
        // distance
        code = dataArray[index + Number_3];
        stream.writeBits(distCodes[code], distLengths[code], true);
        // distance extra
        stream.writeBits(dataArray[index + Number_4], dataArray[index + Number_5], true);
        // end marker
        index += Number_5;
      } else if (literal === Number_256) {
        break;
      }
      index += 1;
    }

    return stream;
  }

  public fixedHuffman(dataArray: Uint16Array, stream: BitStream): BitStream {
    let index: number = 0;
    let literal: number;
    while (index < dataArray.length) {
      literal = dataArray[index];
      stream.writeBits(RawDeflate.FixedHuffmanTable[literal][0], RawDeflate.FixedHuffmanTable[literal][1]);
      if (literal > 0x100) {
        stream.writeBits(dataArray[index + 1], dataArray[index + Number_2], true);
        stream.writeBits(dataArray[index + Number_3], Number_5);
        stream.writeBits(dataArray[index + Number_4], dataArray[index + Number_5], true);
        index += Number_5;
      } else if (literal === 0x100) {
        break;
      }
    }
    return stream;
  }

  public lz77(dataArray: Uint8Array): Uint16Array {
    let matchKey: number = 0;
    let table: Record<number, number[]> = {};
    let windowSize: number = RawDeflate.WindowSize;
    let matchsList: number[] = [];
    let longestMatch: Lz77Match;
    let prevMatch: Lz77Match | null = null;
    let lz77buf: Uint16Array = replace16NumberCount(dataArray.length * Number_2);
    let pos: number = 0;
    let skipLength: number = 0;
    let freqsLitLen: Uint32Array = replace32NumberCount(Number_286);
    let freqsDist: Uint32Array = replace32NumberCount(Number_30);
    let lazy: number = this.lazy;
    let tmp: number;

    freqsLitLen[Number_256] = 1;
    const writeMatch = (match: Lz77Match, offset: number): void => {
      let lz77Array = match.toLz77Array();

      for (let i = 0; i < lz77Array.length; ++i) {
        lz77buf[pos] = lz77Array[i];
        pos += 1;
      }

      freqsLitLen[lz77Array[0]] += 1;
      freqsDist[lz77Array[Number_3]] += 1;
      skipLength = match.length + offset - 1;
      prevMatch = null;
    };

    for (let position = 0; position < dataArray.length; ++position) {
      for (let i = 0; i < RawDeflate.Lz77MinLength; ++i) {
        if (position + i === dataArray.length) {
          break;
        }
        matchKey = (matchKey << Number_8) | dataArray[position + i];
      }

      if (table[matchKey] != null) {
        matchsList = table[matchKey];
      } else {
        matchsList = [];
        table[matchKey] = matchsList;
      }

      if (skipLength > 0) {
        skipLength -= 1;
        matchsList.push(position);
        continue;
      }

      while (matchsList.length > 0 && position - matchsList[0] > windowSize) {
        matchsList.shift();
      }

      if (position + RawDeflate.Lz77MinLength >= dataArray.length) {
        if (prevMatch != null) {
          writeMatch(prevMatch, -1);
        }

        for (let i = position; i < dataArray.length; ++i) {
          tmp = dataArray[i];
          lz77buf[pos] = tmp;
          pos += 1;
          freqsLitLen[tmp] += 1;
        }

        break;
      }

      if (matchsList.length > 0) {
        longestMatch = this.searchLongestMatch_(dataArray, position, matchsList);
        if (prevMatch != null) {
          if (prevMatch.length < longestMatch.length) {
            // write previous literal
            tmp = dataArray[position - 1];
            lz77buf[pos] = tmp;
            pos += 1;
            freqsLitLen[tmp] += 1;

            // write current match
            writeMatch(longestMatch, 0);
          } else {
            // write previous match
            writeMatch(prevMatch, -1);
          }
        } else if (longestMatch.length < lazy) {
          prevMatch = longestMatch;
        } else {
          writeMatch(longestMatch, 0);
        }
      } else if (prevMatch != null) {
        writeMatch(prevMatch, -1);
      } else {
        tmp = dataArray[position];
        lz77buf[pos] = tmp;
        pos += 1;
        freqsLitLen[tmp] += 1;
      }

      matchsList.push(position);
    }

    lz77buf[pos] = 256;
    pos += 1;
    freqsLitLen[Number_256] += 1;
    this.freqsLitLen = freqsLitLen;
    this.freqsDist = freqsDist;
    /** @type {!(Uint16Array|Array.<number>)} */
    return lz77buf.subarray(0, pos);
  }

  public searchLongestMatch_(data: Uint8Array, position: number, matchList: number[]): Lz77Match {
    let match: number;
    let currentMatch: number | null = null;
    let matchMax: number = 0;
    let matchLength: number;
    let dl: number = data.length;

    permatch: for (let i = 0; i < matchList.length; i++) {
      match = matchList[matchList.length - i - 1];
      matchLength = RawDeflate.Lz77MinLength;

      if (matchMax > RawDeflate.Lz77MinLength) {
        for (let j = matchMax; j > RawDeflate.Lz77MinLength; j--) {
          if (data[match + j - 1] !== data[position + j - 1]) {
            continue permatch;
          }
        }
        matchLength = matchMax;
      }

      while (
        matchLength < RawDeflate.Lz77MaxLength &&
        position + matchLength < dl &&
        data[match + matchLength] === data[position + matchLength]
      ) {
        matchLength += 1;
      }

      if (matchLength > matchMax) {
        currentMatch = match;
        matchMax = matchLength;
      }

      if (matchLength === RawDeflate.Lz77MaxLength) {
        break;
      }
    }
    return new Lz77Match(matchMax, position - (currentMatch ?? 0));
  }

  public getTreeSymbols_(hlit: number, litlenLengths: Uint8Array, hdist: number, distLengths: Uint8Array): TreeSymbols {
    let src: Uint32Array = replace32NumberCount(hlit + hdist);
    let j: number = 0;
    let runLength: number;
    let result = replace32NumberCount(Number_286 + Number_30);
    let nResult: number;
    let rpt: number;
    let freqs: Uint8Array = replace8NumberCount(Number_19);

    for (let i = 0; i < hlit; i++) {
      src[j] = litlenLengths[i];
      j += 1;
    }
    for (let i = 0; i < hdist; i++) {
      src[j] = distLengths[i];
      j += 1;
    }

    nResult = 0;
    let i: number = 0;
    while (i < src.length) {
      j = 1;
      while (i + j < src.length && src[i + j] === src[i]) {
        j += 1;
      }

      runLength = j;

      if (src[i] === 0) {
        if (runLength < Number_3) {
          while (runLength > 0) {
            result[nResult] = 0;
            freqs[0] += 1;
            nResult += 1;
            runLength -= 1;
          }
        } else {
          while (runLength > 0) {
            rpt = Math.min(runLength, Number_138);

            if (rpt > runLength - Number_3 && rpt < runLength) {
              rpt = runLength - Number_3;
            }

            if (rpt <= Number_10) {
              result[nResult] = 17;
              result[nResult + 1] = rpt - Number_3;
              freqs[Number_17] += 1;
              nResult += Number_2;
            } else {
              result[nResult] = 18;
              result[nResult + 1] = rpt - Number_11;
              freqs[Number_18] += 1;
              nResult += Number_2;
            }

            runLength -= rpt;
          }
        }
      } else {
        result[nResult] = src[i];
        nResult += 1;
        freqs[src[i]] += 1;
        runLength -= 1;

        if (runLength < Number_3) {
          while (runLength > 0) {
            result[nResult] = src[i];
            freqs[src[i]] += 1;
            nResult += 1;
            runLength -= 1;
          }
        } else {
          while (runLength > 0) {
            rpt = Math.min(runLength, Number_6);

            if (rpt > runLength - Number_3 && rpt < runLength) {
              rpt = runLength - Number_3;
            }

            result[nResult] = 16;
            result[nResult + 1] = rpt - Number_3;
            freqs[Number_16] += 1;
            nResult += Number_2;
            runLength -= rpt;
          }
        }
      }
      i += j;
    }
    return { codes: result.subarray(0, nResult), freqs: freqs };
  }

  public getLengths_(freqs: Uint8Array | Uint32Array, limit: number): Uint8Array {
    let nSymbols: number = freqs.length;
    let heap: Heap = new Heap(Number_2 * RawDeflate.HUFMAX);
    let length: Uint8Array = replace8NumberCount(nSymbols);
    let nodes: PopObject[];
    let values: Uint32Array;
    let codeLength: Uint8Array;

    for (let i = 0; i < nSymbols; ++i) {
      let intfreqs: number = freqs[i];
      if (intfreqs > 0) {
        heap.push(i, intfreqs);
      }
    }
    nodes = replacePopObjectArray(heap.length / Number_2, {
      index: 0,
      value: 0,
      length: 0
    });
    values = replace32NumberCount(heap.length / Number_2);

    if (nodes.length === 1) {
      length[heap.pop().index] = 1;
      return length;
    }

    for (let i = 0; i < heap.length / Number_2; ++i) {
      nodes[i] = heap.pop();
      values[i] = nodes[i].value;
    }
    codeLength = this.reversePackageMerge_(values, values.length, limit);

    for (let i = 0; i < nodes.length; ++i) {
      length[nodes[i].index] = codeLength[i];
    }

    return length;
  }

  public reversePackageMerge_(freqs: Array<number> | Uint32Array, symbols: number, limit: number): Uint8Array {
    let minimumCost: Uint16Array = replace16NumberCount(limit);
    let flag: Uint8Array = replace8NumberCount(limit);
    let codeLength: Uint8Array = replace8NumberCount(symbols);
    let value: number[][] = new Array(limit);
    let type: number[][] = new Array(limit);
    let currentPosition: number[] = replaceNumberCount(limit);

    let excess: number = (1 << limit) - symbols;
    let half: number = 1 << (limit - 1);
    let weight: number = 0;
    let next: number = 0;

    const takePackage = (index: number): void => {
      let x: number = type[index][currentPosition[index]];

      if (x === symbols) {
        takePackage(index + 1);
        takePackage(index + 1);
      } else {
        codeLength[x] -= 1;
      }

      currentPosition[index] += 1;
    };

    minimumCost[limit - 1] = symbols;

    for (let j = 0; j < limit; ++j) {
      if (excess < half) {
        flag[j] = 0;
      } else {
        flag[j] = 1;
        excess -= half;
      }
      excess <<= 1;
      minimumCost[limit - Number_2 - j] = ((minimumCost[limit - 1 - j] / Number_2) | 0) + symbols;
    }
    minimumCost[0] = flag[0];

    value[0] = replaceNumberCount(minimumCost[0]);
    type[0] = replaceNumberCount(minimumCost[0]);
    for (let j = 1; j < limit; ++j) {
      if (minimumCost[j] > Number_2 * minimumCost[j - 1] + flag[j]) {
        minimumCost[j] = Number_2 * minimumCost[j - 1] + flag[j];
      }
      value[j] = replaceNumberCount(minimumCost[j]);
      type[j] = replaceNumberCount(minimumCost[j]);
    }

    for (let i = 0; i < symbols; ++i) {
      codeLength[i] = limit;
    }

    for (let t = 0; t < minimumCost[limit - 1]; ++t) {
      value[limit - 1][t] = freqs[t];
      type[limit - 1][t] = t;
    }

    for (let i = 0; i < limit; ++i) {
      currentPosition[i] = 0;
    }

    if (flag[limit - 1] === 1) {
      codeLength[0] -= 1;
      currentPosition[limit - 1] += 1;
    }

    let j: number = limit - Number_2;
    while (j >= 0) {
      let i: number = 0;
      weight = 0;
      next = currentPosition[j + 1];

      let t: number = 0;
      while (t < minimumCost[j]) {
        weight = value[j + 1][next] + value[j + 1][next + 1];

        if (weight > freqs[i]) {
          value[j][t] = weight;
          type[j][t] = symbols;
          next += Number_2;
        } else {
          value[j][t] = freqs[i];
          type[j][t] = i;
          i += 1;
        }
        t += 1;
      }

      currentPosition[j] = 0;
      if (flag[j] === 1) {
        takePackage(j);
      }
      j -= 1;
    }

    return codeLength;
  }

  public getCodesFromLengths_(lengths: Uint8Array): Uint16Array {
    let codes: Uint16Array = replace16NumberCount(lengths.length);
    let count: number[] = [];
    let startCode: number[] = [];
    let code: number = 0;

    // Count the codes of each length.
    for (let i = 0; i < lengths.length; i++) {
      count[lengths[i]] = (count[lengths[i]] | 0) + 1;
    }

    // Determine the starting code for each length block.
    for (let i = 1; i <= RawDeflate.MaxCodeLength; i++) {
      startCode[i] = code;
      code += count[i] | 0;
      code <<= 1;
    }

    // Determine the code for each symbol. Mirrored, of course.
    for (let i = 0; i < lengths.length; i++) {
      code = startCode[lengths[i]];
      startCode[lengths[i]] += 1;
      codes[i] = 0;

      for (let j = 0; j < lengths[i]; j++) {
        codes[i] = (codes[i] << 1) | (code & 1);
        code >>>= 1;
      }
    }
    return codes;
  }
}
