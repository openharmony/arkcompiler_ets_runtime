/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

/*
 * @tc.name:typedarraytosorted
 * @tc.desc:test TypedArray.toSorted
 * @tc.type: FUNC
 * @tc.require: issueI7MUIL
 */
const typedArraySortedConstructors = [
    Float64Array, Float32Array, Int32Array, Int16Array, Int8Array, Uint32Array, Uint16Array, Uint8Array,
    Uint8ClampedArray
];

typedArraySortedConstructors.forEach(function (ctor, i) {
    let arr = [10, 3, 8, 5, 30, 100, 6, 7, 100, 3];
    if (testTypeArrayToSorted(ctor, arr)) {
        print(ctor.name + " test success !!!");
    } else {
        print(ctor.name + " test fail !!!");
    }
});

[
    BigInt64Array,
    BigUint64Array
].forEach(function (ctor, i) {
    let arr = [10n, 3n, 8n, 5n, 30n, 100n, 6n, 7n, 100n, 3n];
    if (testTypeArrayToSorted(ctor, arr)) {
        print(ctor.name + " test success !!!");
    } else {
        print(ctor.name + " test fail !!!");
    }
});

function testTypeArrayToSorted(ctor, arr) {
    let obj = new ctor(arr);
    let arr2 = obj.toSorted();
    let result = [];
    result.push(arr2.toString() == "3,3,5,6,7,8,10,30,100,100");
    arr2 = obj.toSorted((x, y) => { return x < y });
    result.push(arr2.toString() == "100,100,30,10,8,7,6,5,3,3");
    try {
        arr2 = obj.toSorted(true);
    } catch (err) {
        result.push(err.name == "TypeError");
    }
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

var arr = new Float64Array(["aaa", 10, 20, 30]);
var arrSorted = arr.toSorted();
print(arrSorted);
print(arr);
