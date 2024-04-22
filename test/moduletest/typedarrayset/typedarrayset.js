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

[
    Float64Array,
    Float32Array,
    Int32Array,
    Int16Array,
    Int8Array,
    Uint32Array,
    Uint16Array,
    Uint8Array,
    Uint8ClampedArray
].forEach(function (ctor) {
    testTypeArraySet1(ctor)
});

function testTypeArraySet1(ctor) {
    const obj = new ctor(8);
    obj.set(new ctor(6), 2);
    print(obj);
}

var typedArray1 = new Int32Array([1,1,1,1,1,1,1,1]);
var typedArrayIn1 = new Uint16Array([2, 2, 2]);
typedArray1.set(typedArrayIn1, 2);
print(typedArray1);

var typedArray2 = new Int32Array([1,1,1,1,1,1,1,1]);
typedArray2.set([2, 2, 2], 2);
print(typedArray2);

var typedArray3 = new Int32Array([1,1,1,1,1,1,1,1]);
typedArray3.set([2, , 2], 2);
print(typedArray3);

var typedArray4 = new Int32Array([1,1,1,1,1,1,1,1]);
typedArray4.set([2, undefined, 2], 2);
print(typedArray4);
