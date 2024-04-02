/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

// @ts-nocheck
declare function print(arg:any):string;

function test() {
    let array = ['12dasd','1dads','sadq','tretgre'];
    let key = "1";
    print(array[key]);
}

print(ArkTools.isAOTDeoptimized(test));
test();
print(ArkTools.isAOTDeoptimized(test));

class BigInteger {
  static one: BigInteger = BigInteger.nbv(1);
  array: (number | null)[] = [];

  static nbi(): BigInteger {
    return new BigInteger();
  }

  static nbv(i: number): BigInteger {
    let r = BigInteger.nbi();
    r.fromInt(i);
    return r;
  }

  fromInt(x: number): void {
    this.array = insertValue(this.array, x, 0);
    print(this.array.length)
  }
}

function insertValue(array: (number | null)[], value: number, i: number): (number | null)[] {
  let arr = array;
  const surplus = i - arr.length;
  const surplusArr: (number | null)[] = new Array(surplus + 1).fill(null);
  arr = arr.concat(surplusArr);
  arr[i] = value
  return arr
}

function hexSha1(s: string): number {
  let x : number[] = str2binb(s);
  x = x.concat(Array.from({ length: 12 }, () => 0));
  return x.length;
}

function str2binb(str: string): Array<number> {
  let bin = Array<number>();
  const chrsz = 8
  const bitoffset = 5
  const mask = (1 << chrsz) - 1;
  const toMax = ((str.length * chrsz - 1) >> bitoffset) - bin.length;
  if (toMax >= 0) {
    bin = bin.concat(Array<number>(toMax + 1));
  }

  for (let i = 0; i < str.length * chrsz; i += chrsz) {
    bin[i >> bitoffset] |= (str.charCodeAt(i / chrsz) & mask) << (32 - chrsz - (i % 32));
  }
  return bin;
}

function run(): void {
  let plainText: string =
    "Two households, both alike in dignity,\nIn here shall miss, our toil shall strive to mend.";

  const copy = 5
  for (let i = 0; i < copy; i++) {
    plainText += plainText;
  }

  print(hexSha1(plainText));
}

run();

function testFastPath() {
    let array = [ 1 ]

    function foo(arr) {
        let ret = arr[0];
        array[0] = undefined
        return ret
    }

    for (let i = 0; i < 100000; i++) {
        foo(array)
        foo(array)
    }
    print(array[0])
}

testFastPath();
