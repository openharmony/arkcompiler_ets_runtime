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

export function replaceNumberCount(count: number, element: number = 0): number[] {
  let result = new Array(count).fill(element);

  return result;
}
const Number_2 = 2;
const Number_8 = 8;
const Number_16 = 16;
const Number_24 = 24;
const Number_32 = 32;
const Number_256 = 256;

export function replace8NumberCount(count: number, element: number = 0): Uint8Array {
  let result = new Uint8Array(count).fill(element);

  return result;
}

export function replace16NumberCount(count: number, element: number = 0): Uint16Array {
  let result = new Uint16Array(count).fill(element);

  return result;
}

export function replace32NumberCount(count: number, element: number = 0): Uint32Array {
  let result = new Uint32Array(count).fill(element);

  return result;
}


export class BitStream {
  public static DefaultBlockSize = 0x8000;
  private index: number;
  private bitindex: number;
  private buffer: Uint8Array;
  public static ReverseTable: Uint8Array = new Uint8Array(0);

  constructor(buffer: Uint8Array, bufferPosition: number) {
    BitStream.ReverseTable = this.getReverseTable();
    this.index = bufferPosition;
    this.bitindex = 0;
    this.buffer = buffer;

    if (this.buffer.length * Number_2 <= this.index) {
      throw new Error('invalid index');
    } else if (this.buffer.length <= this.index) {
      this.expandBuffer();
    }
  }

  public expandBuffer(): Uint8Array {
    let oldbuf: Uint8Array = this.buffer;
    let il: number = oldbuf.length;
    // copy buffer
    let uint8Buffer: Uint8Array = replace8NumberCount(il << 1);
    uint8Buffer.set(oldbuf);
    this.buffer = uint8Buffer;
    return this.buffer;
  }

  public writeBits(number: number, n: number, reverse: boolean = false): void {
    let numberTemp: number = number;
    let buffer: Uint8Array = this.buffer;
    let index: number = this.index;
    let bitindex: number = this.bitindex;

    let current: number = buffer[index];

    const rev32_ = (num: number): number => {
      let A = BitStream.ReverseTable[num & 0xff] << Number_24;
      let B = BitStream.ReverseTable[(num >>> Number_8) & 0xff] << Number_16;
      let C = BitStream.ReverseTable[(num >>> Number_16) & 0xff] << Number_8;
      let D = BitStream.ReverseTable[(num >>> Number_24) & 0xff];
      return A | B | C | D;
    };

    if (reverse && n > 1) {
      numberTemp =
        n > Number_8 ? rev32_(numberTemp) >> (Number_32 - n) : BitStream.ReverseTable[numberTemp] >> (Number_8 - n);
    }

    if (n + bitindex < Number_8) {
      current = (current << n) | numberTemp;
      bitindex += n;
    } else {
      for (let i = 0; i < n; ++i) {
        current = (current << 1) | ((numberTemp >> (n - i - 1)) & 1);

        // next byte
        bitindex += 1;
        if (bitindex === Number_8) {
          bitindex = 0;
          buffer[index] = BitStream.ReverseTable[current];
          index += 1;
          current = 0;

          // expand
          if (index === buffer.length) {
            buffer = this.expandBuffer();
          }
        }
      }
    }
    buffer[index] = current;
    this.buffer = buffer;
    this.bitindex = bitindex;
    this.index = index;
  }

  public finish(): Uint8Array {
    let buffer: Uint8Array = this.buffer;
    let index: number = this.index;
    let output: Uint8Array;

    if (this.bitindex > 0) {
      buffer[index] <<= Number_8 - this.bitindex;
      buffer[index] = BitStream.ReverseTable[buffer[index]];
      index += 1;
    }
    // array truncation

    output = buffer.subarray(0, index);

    return output;
  }

  private getReverseTable(): Uint8Array {
    let table = replace8NumberCount(Number_256);

    for (let i = 0; i < Number_256; ++i) {
      table[i] = ((n): number => {
        let N = n;
        let r = n;
        let s = 7;
        N >>>= 1;
        while (N != 0) {
          r <<= 1;
          r |= N & 1;
          s -= 1;
          N >>>= 1;
        }
        return ((r << s) & 0xff) >>> 0;
      })(i);
    }
    return table;
  }
}
