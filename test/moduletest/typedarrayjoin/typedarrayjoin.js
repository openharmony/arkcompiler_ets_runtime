/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
const constructors = [
  [Uint8Array, [0, 1]],
  [Int8Array, [0, 1]],
  [Uint16Array, [0, 1]],
  [Int16Array, [0, 1]],
  [Uint32Array, [0, 1]],
  [Int32Array, [0, 1]],
  [Float32Array, [0, 1]],
  [Float64Array, [0, 1]],
  [Uint8ClampedArray, [0, 1]],
  [BigInt64Array, [0n, 1n]],
  [BigUint64Array, [0n, 1n]]
];

const constructs = constructors;

let typedArray;

const separator = {
  toString() {
    ArkTools.arrayBufferDetach(typedArray.buffer);
    return '*';
  }
};

constructors.forEach(([constructor, arr]) => {
  typedArray = new constructor(arr);
  assert_equal(typedArray.join(separator) == '*',true);
});

let typedArrays;
function detachBuffer() {
  ArkTools.arrayBufferDetach(typedArrays.buffer);
  return 'a';
}
Number.prototype.toString = detachBuffer;
BigInt.prototype.toString = detachBuffer;
Number.prototype.toLocaleString = detachBuffer;
BigInt.prototype.toLocaleString = detachBuffer;

constructs.forEach(([constructor, arr]) => {
  typedArrays = new constructor(arr);
  assert_equal(typedArrays.join() == '0,1',true);
  assert_equal(typedArrays.toLocaleString() == 'a,',true);
});

test_end();