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

import { Huffman } from './huffman';
import { replace8NumberCount } from './bitstream';

const Number_2 = 2;
const Number_3 = 3;
const Number_4 = 4;
const Number_5 = 5;
const Number_7 = 7;
const Number_8 = 8;
const Number_9 = 9;
const Number_11 = 11;
const Number_16 = 16;
const Number_17 = 17;
const Number_18 = 18;
const Number_30 = 30;
const Number_143 = 143;
const Number_255 = 255;
const Number_256 = 256;
const Number_257 = 257;
const Number_258 = 258;
const Number_279 = 279;
const Number_288 = 288;
const Number_32768 = 32768;
interface InOptionParams {
  index: number;
  bufferSize: number;
  bufferType: number;
  resize: boolean;
}

interface ExpandOption {
  fixRatio?: number;
  addRatio?: number;
}

enum BufferType {
  BLOCK,
  ADAPTIVE
}

/*
 *@Generator
 */
export class RawInflate {
  public static ZLIB_RAW_INFLATE_BUFFER_SIZE: number = 0x8000;
  public static buildHuffmanTable: (lengths: Uint8Array) => [Uint32Array, number, number] = Huffman.buildHuffmanTable;
  public static MaxBackwardLength: number = Number_32768;
  public static MaxCopyLength: number = Number_258;
  public currentLitlenTable: Uint16Array | [Uint32Array, number, number] | null = null;

  public static order: Uint16Array = ((): Uint16Array => {
    const table = [16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15];
    return new Uint16Array(table);
  })();

  public static LengthCodeTable: Uint16Array = ((table: number[]): Uint16Array => {
    return new Uint16Array(table);
  })([
    0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008, 0x0009, 0x000a, 0x000b, 0x000d, 0x000f, 0x0011, 0x0013, 0x0017,
    0x001b, 0x001f, 0x0023, 0x002b, 0x0033, 0x003b, 0x0043, 0x0053, 0x0063, 0x0073, 0x0083, 0x00a3, 0x00c3, 0x00e3,
    0x0102, 0x0102, 0x0102
  ]);

  public static LengthExtraTable: Uint8Array = ((table: number[]): Uint8Array => {
    return new Uint8Array(table);
  })([0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 0, 0]);

  public static DistCodeTable: Uint16Array = ((table: number[]): Uint16Array => {
    return new Uint16Array(table);
  })([
    0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0007, 0x0009, 0x000d, 0x0011, 0x0019, 0x0021, 0x0031, 0x0041, 0x0061,
    0x0081, 0x00c1, 0x0101, 0x0181, 0x0201, 0x0301, 0x0401, 0x0601, 0x0801, 0x0c01, 0x1001, 0x1801, 0x2001, 0x3001,
    0x4001, 0x6001
  ]);

  public static DistExtraTable: Uint8Array = ((): Uint8Array => {
    const table = [0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13];
    return new Uint8Array(table);
  })();

  public static FixedLiteralLengthTable: [Uint32Array, number, number] = ((): [Uint32Array, number, number] => {
    let lengths = new Uint8Array(replace8NumberCount(Number_288));

    for (let i = 0; i < lengths.length; ++i) {
      lengths[i] = i <= Number_143 ? Number_8 : i <= Number_255 ? Number_9 : i <= Number_279 ? Number_7 : Number_8;
    }
    return RawInflate.buildHuffmanTable(lengths);
  })();

  public static FixedDistanceTable: [Uint32Array, number, number] = ((): [Uint32Array, number, number] => {
    let lengths = new Uint8Array(replace8NumberCount(Number_30));

    for (let i = 0; i < lengths.length; ++i) {
      lengths[i] = 5;
    }

    return RawInflate.buildHuffmanTable(lengths);
  })();
  public buffer: Uint8Array | null = null;
  public blocks: Array<Uint8Array>;
  public bufferSize: number;
  public totalpos: number;
  public ip: number;
  public bitsbuf: number;
  public bitsbuflen: number;
  public input: Uint8Array;
  public output: Uint8Array;
  public op: number;
  public bfinal: boolean = false;
  public bufferType = BufferType.ADAPTIVE;
  public resize: boolean = false;

  constructor(input: Uint8Array, opt_params?: InOptionParams) {
    this.blocks = [];
    this.bufferSize = RawInflate.ZLIB_RAW_INFLATE_BUFFER_SIZE;
    this.totalpos = 0;
    this.ip = 0;
    this.bitsbuf = 0;
    this.bitsbuflen = 0;
    this.input = new Uint8Array(input);
    this.bfinal = false;
    this.bufferType = BufferType.ADAPTIVE;
    this.resize = false;

    // option parameters
    if (opt_params != null) {
      if (opt_params.index) {
        this.ip = opt_params.index;
      }
      if (opt_params.bufferSize) {
        this.bufferSize = opt_params.bufferSize;
      }
      if (opt_params.bufferType) {
        this.bufferType = opt_params.bufferType;
      }
      if (opt_params.resize) {
        this.resize = opt_params.resize;
      }
    }

    // initialize
    switch (this.bufferType) {
      case BufferType.BLOCK:
        this.op = RawInflate.MaxBackwardLength;
        this.output = replace8NumberCount(RawInflate.MaxBackwardLength + this.bufferSize + RawInflate.MaxCopyLength);
        break;
      case BufferType.ADAPTIVE:
        this.op = 0;
        this.output = replace8NumberCount(this.bufferSize);
        break;
      default:
        throw new Error('invalid inflate mode');
    }
  }

  public decompress(): Uint8Array {
    while (!this.bfinal) {
      this.parseBlock();
    }
    switch (this.bufferType) {
      case BufferType.BLOCK:
        return this.concatBufferBlock();
      case BufferType.ADAPTIVE:
        return this.concatBufferDynamic();
      default:
        throw new Error('invalid inflate mode');
    }
  }

  public parseBlock() {
    let hdr: number = this.readBits(Number_3);

    // bfinal
    if ((hdr & 0x1) != 0) {
      this.bfinal = true;
    }

    // hdr btype
    hdr >>>= 1;
    switch (hdr) {
      // case uncompressed 
      case 0:
        this.parseUncompressedBlock();
        break;
      // case fixed huffman
      case 1:
        this.parseFixedHuffmanBlock();
        break;
      // case dynamic huffman
      case Number_2:
        this.parseDynamicHuffmanBlock();
        break;
      // case reserved or other
      default:
        throw new Error(`unknown BTYPE: ${hdr}`);
    }
  }

  public readBits(length: number): number {
    let bitsbuf: number = this.bitsbuf;
    let bitsbuflen: number = this.bitsbuflen;
    let input: Uint8Array = this.input;
    let ip: number = this.ip;

    /** @type {number} */
    let inputLength = input.length;
    /** @type {number} input and output byte. */
    let octet: number;

    // input byte tet
    if (ip + ((length - bitsbuflen + Number_7) >> Number_3) >= inputLength) {
      throw new Error('input buffer is broken');
    }

    // not enough buffer
    while (bitsbuflen < length) {
      bitsbuf |= input[ip] << bitsbuflen;
      ip += 1;
      bitsbuflen += Number_8;
    }

    // output byte tet
    octet = bitsbuf & ((1 << length) - 1);
    bitsbuf >>>= length;
    bitsbuflen -= length;
    // set  bitsbuf,bitsbuflen and ip
    this.bitsbuf = bitsbuf;
    this.bitsbuflen = bitsbuflen;
    this.ip = ip;

    return octet;
  }

  public readCodeByTable(table: Uint16Array | Uint8Array | [Uint32Array, number, number]): number {
    let bitsbuf: number = this.bitsbuf;
    let bitsbuflen: number = this.bitsbuflen;
    let input: Uint8Array = this.input;
    let ip: number = this.ip;

    let inputLength: number = input.length;
    let codeTable: number | Uint32Array = table[0];
    let maxCodeLength: number = table[1];
    let codeWithLength: number;
    let codeLength: number;

    // not enough buffer
    while (bitsbuflen < maxCodeLength) {
      if (ip >= inputLength) {
        break;
      }
      bitsbuf |= input[ip] << bitsbuflen;
      ip += 1;
      bitsbuflen += Number_8;
    }

    // read max length
    codeWithLength = codeTable[bitsbuf & ((1 << maxCodeLength) - 1)];
    codeLength = codeWithLength >>> Number_16;

    if (codeLength > bitsbuflen) {
      throw new Error(`invalid code length: ${codeLength}`);
    }

    bitsbuf >>= codeLength;
    bitsbuflen -= codeLength;
    ip += 1;

    this.bitsbuf = bitsbuf;
    this.bitsbuflen = bitsbuflen;
    this.ip = ip;

    return codeWithLength & 0xffff;
  }

  public parseUncompressedBlock(): void {
    let input: Uint8Array = this.input;
    let ip: number = this.ip;
    let output: Uint8Array = this.output;
    let op: number = this.op;

    let inputLength: number = input.length;
    let len: number;
    let nlen: number;
    let olength: number = output.length;
    let preCopy: number;

    // skip buffered header bits and set 0
    this.bitsbuf = 0;
    this.bitsbuflen = 0;

    // check len
    if (ip + 1 >= inputLength) {
      throw new Error('invalid uncompressed block header: LEN');
    }
    len = input[ip] | (input[ip + 1] << Number_8);
    ip += Number_2;

    // nlen
    if (ip + 1 >= inputLength) {
      throw new Error('invalid uncompressed block header: NLEN');
    }
    nlen = input[ip] | (input[ip + 1] << Number_8);
    ip += Number_2;

    // check len & nlen
    if (len === ~nlen) {
      throw new Error('invalid uncompressed block header: length verify');
    }

    // check size
    if (ip + len > input.length) {
      throw new Error('input buffer is broken');
    }

    // expand buffer
    switch (this.bufferType) {
      case BufferType.BLOCK:

        while (op + len > output.length) {
          preCopy = olength - op;
          len -= preCopy;
          // set input to output
          output.set(input.subarray(ip, ip + preCopy), op);
          op += preCopy;
          ip += preCopy;
          // set op
          this.op = op;
          output = this.expandBufferBlock();
          op = this.op;
        }
        break;
      //  ADAPTIVE
      case BufferType.ADAPTIVE:
        while (op + len > output.length) {
          output = this.expandBufferAdaptive({ fixRatio: 2 });
        }
        break;
      default:
        // Error
        throw new Error('invalid inflate mode');
    }

    // copy input.subarray

    output.set(input.subarray(ip, ip + len), op);
    op += len;
    ip += len;
    // reset ip,op,output
    this.ip = ip;
    this.op = op;
    this.output = output;
  }

  public parseFixedHuffmanBlock(): void {
    switch (this.bufferType) {
      case BufferType.ADAPTIVE:
        this.decodeHuffmanAdaptive(RawInflate.FixedLiteralLengthTable, RawInflate.FixedDistanceTable);
        break;
      case BufferType.BLOCK:
        this.decodeHuffmanBlock(RawInflate.FixedLiteralLengthTable, RawInflate.FixedDistanceTable);
        break;
      default:
        throw new Error('invalid inflate mode');
    }
  }

  public parseDynamicHuffmanBlock(): void {
    let hlit: number = this.readBits(Number_5) + Number_257;
    let hdist: number = this.readBits(Number_5) + 1;
    let hclen: number = this.readBits(Number_4) + Number_4;
    let codeLengths: Uint8Array = replace8NumberCount(RawInflate.order.length);
    let codeLengthsTable: [Uint32Array, number, number];
    let litlenTable: Uint8Array | [Uint32Array, number, number];
    let distTable: Uint8Array | [Uint32Array, number, number];
    let lengthTable: Uint8Array;
    let code: number;
    let prev: number | null = null;
    let repeats: number;

    // decode code lengths
    for (let i = 0; i < hclen; ++i) {
      codeLengths[RawInflate.order[i]] = this.readBits(Number_3);
    }

    // decode length table
    codeLengthsTable = RawInflate.buildHuffmanTable(codeLengths);
    lengthTable = replace8NumberCount(hlit + hdist);
    let i = 0;
    while (i < hlit + hdist) {
      code = this.readCodeByTable(codeLengthsTable);
      switch (code) {
        case Number_16:
          repeats = Number_3 + this.readBits(Number_2);
          while (repeats > 0) {
            lengthTable[i] = prev ?? 0;
            i += 1;
            repeats -= 1;
          }
          break;
        case Number_17:
          repeats = Number_3 + this.readBits(Number_3);
          while (repeats > 0) {
            lengthTable[i] = 0;
            i += 1;
            repeats -= 1;
          }
          prev = 0;
          break;
        case Number_18:
          repeats = Number_11 + this.readBits(Number_7);
          while (repeats > 0) {
            lengthTable[i] = 0;

            repeats -= 1;
            i += 1;
          }
          prev = 0;
          break;
        default:
          lengthTable[i] = code;
          prev = code;
          i += 1;
          break;
      }
    }

    litlenTable = RawInflate.buildHuffmanTable(lengthTable.subarray(0, hlit));

    distTable = RawInflate.buildHuffmanTable(lengthTable.subarray(hlit));

    switch (this.bufferType) {
      case BufferType.ADAPTIVE:
        this.decodeHuffmanAdaptive(litlenTable, distTable);
        break;
      case BufferType.BLOCK:
        this.decodeHuffmanBlock(litlenTable, distTable);
        break;
      default:
        throw new Error('invalid inflate mode');
    }
  }

  public decodeHuffmanBlock(
    litlen: Uint16Array | [Uint32Array, number, number],
    dist: Uint8Array | [Uint32Array, number, number]
  ): void {
    let output: Uint8Array = this.output;
    let op: number = this.op;

    this.currentLitlenTable = litlen;

    let olength: number = output.length - RawInflate.MaxCopyLength;
    let code: number;
    let ti: number;
    let codeDist: number;
    let codeLength: number;

    let lengthCodeTable: Uint16Array = RawInflate.LengthCodeTable;
    let lengthExtraTable: Uint8Array = RawInflate.LengthExtraTable;
    let distCodeTable: Uint16Array = RawInflate.DistCodeTable;
    let distExtraTable: Uint8Array = RawInflate.DistExtraTable;

    code = this.readCodeByTable(litlen);
    while (code !== Number_256) {
      // literal
      if (code < Number_256) {
        if (op >= olength) {
          this.op = op;
          output = this.expandBufferBlock();
          op = this.op;
        }
        output[op] = code;
        op += 1;
        continue;
      }

      // code length 
      ti = code - Number_257;
      codeLength = lengthCodeTable[ti];
      if (lengthExtraTable[ti] > 0) {
        codeLength += this.readBits(lengthExtraTable[ti]);
      }
      // code dist 
      code = this.readCodeByTable(dist);
      codeDist = distCodeTable[code];
      if (distExtraTable[code] > 0) {
        codeDist += this.readBits(distExtraTable[code]);
      }
      // decode lz77
      if (op >= olength) {
        this.op = op;
        output = this.expandBufferBlock();
        op = this.op;
      }
      while (codeLength > 0) {
        output[op] = output[op - codeDist];
        op += 1;
        codeLength -= 1;
      }
      code = this.readCodeByTable(litlen);
    }

    while (this.bitsbuflen >= Number_8) {
      this.bitsbuflen -= 8;
      this.ip -= 1;
    }
    this.op = op;
  }

  public decodeHuffmanAdaptive(
    litlen: Uint16Array | [Uint32Array, number, number],
    dist: Uint8Array | [Uint32Array, number, number]
  ): void {
    let output: Uint8Array = this.output;
    let op: number = this.op;

    this.currentLitlenTable = litlen;

    let olength: number = output.length;
    let code: number;
    let ti: number;
    let codeDist: number;
    let codeLength: number;

    let lengthCodeTable: Uint16Array = RawInflate.LengthCodeTable;
    let lengthExtraTable: Uint8Array = RawInflate.LengthExtraTable;
    let distCodeTable: Uint16Array = RawInflate.DistCodeTable;
    let distExtraTable: Uint8Array = RawInflate.DistExtraTable;

    code = this.readCodeByTable(litlen);
    while (code !== Number_256) {
      // literal
      if (code < Number_256) {
        if (op >= olength) {
          output = this.expandBufferAdaptive();
          olength = output.length;
        }
        output[op] = code;
        op += 1;
        continue;
      }
      // code length 
      ti = code - Number_257;
      codeLength = lengthCodeTable[ti];
      if (lengthExtraTable[ti] > 0) {
        codeLength += this.readBits(lengthExtraTable[ti]);
      }
      // code dist 
      code = this.readCodeByTable(dist);
      codeDist = distCodeTable[code];
      if (distExtraTable[code] > 0) {
        codeDist += this.readBits(distExtraTable[code]);
      }

      // decode lz77
      if (op + codeLength > olength) {
        output = this.expandBufferAdaptive();
        olength = output.length;
      }
      while (codeLength > 0) {
        output[op] = output[op - codeDist];
        op += 1;
        codeLength -= 1;
      }
      code = this.readCodeByTable(litlen);
    }

    while (this.bitsbuflen >= Number_8) {
      this.bitsbuflen -= 8;
      this.ip -= 1;
    }
    this.op = op;
  }

  public expandBufferBlock(): Uint8Array {
    let buffer = replace8NumberCount(this.op - RawInflate.MaxBackwardLength);
    let backward: number = this.op - RawInflate.MaxBackwardLength;

    let output: Uint8Array = this.output;

    // copy to output buffer

    buffer.set(output.subarray(RawInflate.MaxBackwardLength, buffer.length));

    this.blocks.push(buffer);
    this.totalpos += buffer.length;

    // copy to backward buffer

    output.set(output.subarray(backward, backward + RawInflate.MaxBackwardLength));

    this.op = RawInflate.MaxBackwardLength;

    return output;
  }

  public expandBufferAdaptive(opt_param?: ExpandOption): Uint8Array {
    let buffer: Uint8Array;
    let ratio: number = (this.input.length / this.ip + 1) | 0;
    let maxHuffCode: number;
    let newSize: number;
    let maxInflateSize: number;

    let input: Uint8Array = this.input;
    let output: Uint8Array = this.output;

    if (opt_param != null) {
      if (opt_param.fixRatio) {
        ratio = opt_param.fixRatio;
      }
      if (opt_param.addRatio) {
        ratio += opt_param.addRatio;
      }
    }

    // calculate new buffer size
    if (ratio < Number_2) {
      maxHuffCode = (input.length - this.ip) / this.currentLitlenTable![Number_2];
      maxInflateSize = ((maxHuffCode / Number_2) * Number_258) | 0;
      newSize = maxInflateSize < output.length ? output.length + maxInflateSize : output.length << 1;
    } else {
      newSize = output.length * ratio;
    }

    // buffer expantion
    buffer = replace8NumberCount(newSize);
    buffer.set(output);

    this.output = buffer;

    return this.output;
  }

  public concatBufferBlock(): Uint8Array {
    let pos: number = 0;
    let limit: number = this.totalpos + (this.op - RawInflate.MaxBackwardLength);
    let output: Uint8Array = this.output;
    let blocks: Uint8Array[] = this.blocks;
    let buffer: Uint8Array = replace8NumberCount(limit);

    // single buffer
    if (blocks.length === 0) {
      return this.output.subarray(RawInflate.MaxBackwardLength, this.op);
    }

    // copy to buffer
    for (let i = 0; i < blocks.length; ++i) {
      let block: Uint8Array = blocks[i];
      for (let j = 0; j < block.length; ++j) {
        buffer[pos] = block[j];
        pos += 1;
      }
    }

    // current buffer
    for (let i = RawInflate.MaxBackwardLength; i < this.op; ++i) {
      buffer[pos] = output[i];
      pos += 1;
    }

    this.blocks = [];
    this.buffer = buffer;

    return this.buffer ?? new Uint8Array(0);
  }

  public concatBufferDynamic(): Uint8Array {
    let buffer: Uint8Array;
    let op: number = this.op;

    if (this.resize) {
      buffer = replace8NumberCount(op);
      buffer.set(this.output.subarray(0, op));
    } else {
      buffer = this.output.subarray(0, op);
    }

    this.buffer = buffer;

    return this.buffer ?? new Uint8Array(0);
  }
}
