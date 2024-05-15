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

const typedArraySliceConstructors = [
  Float64Array, Float32Array, Int32Array, Int16Array, Int8Array, Uint32Array, Uint16Array, Uint8Array,
  Uint8ClampedArray
];

typedArraySliceConstructors.forEach(function (ctor, i) {
    testTypeArraySlice1(ctor, i)
});
//fast path
function testTypeArraySlice1(ctor, i) {
    let obj = new ctor(i+1);
    for (let j = 0; j < i+1; j++) {
        obj[j] = i;
    }
    print(obj.slice(i, i+1));
}

//slow path
class Array1 extends Int8Array {
    static get [Symbol.species]() {
        return Int8Array;
    }
}
const a = new Array1(1, 2, 3);
print(a.slice(0, 1));

var sample = new BigInt64Array([40n, 41n, 42n, 43n]);
var calls = 0;
Object.defineProperty(BigInt64Array.prototype, "constructor", {
  get: function() {
    calls++;
  }
});
sample.slice();
print(calls);

// module test for IR
var arr;
function TestTypedArraySlice(e) {
  arr = new e([-5, 10, 20, 30, -40, 50.5, -60.6]);
}

TestTypedArraySlice(Int8Array);
print(arr.slice());

TestTypedArraySlice(Int8Array);
print(arr.slice(1.6, 6));

TestTypedArraySlice(Int8Array);
print(arr.slice(1));

TestTypedArraySlice(Int8Array);
print(arr.slice(1, 6.6));

TestTypedArraySlice(Int8Array);
print(arr.slice(3, 2));

TestTypedArraySlice(Int8Array);
print(arr.slice(1, 6));