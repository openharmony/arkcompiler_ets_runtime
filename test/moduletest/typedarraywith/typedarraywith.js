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
 * @tc.name:typedarraywith
 * @tc.desc:test TypedArray.with
 * @tc.type: FUNC
 * @tc.require: issueI7MUIL
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
].forEach(function (ctor, i) {
    if (testTypeArrayWith1(ctor)) {
        print(ctor.name + " test success !!!");
    } else {
        print(ctor.name + " test fail !!!");
    }
});

[
    BigInt64Array,
    BigUint64Array
].forEach(function (ctor, i) {
    if (testTypeArrayWith2(ctor)) {
        print(ctor.name + " test success !!!");
    } else {
        print(ctor.name + " test fail !!!");
    }
});

function testTypeArrayWith1(ctor) {
    let arr = [1, 2, 3, 4, 5];
    let obj = new ctor(arr);
    let result = [];
    let arr2 = obj.with(-2, 40);
    result.push(arr2[3] == 40);
    try {
        arr2 = obj.with(10, 10);
    } catch (err) {
        result.push(err.name == "RangeError");
    }
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArrayWith2(ctor) {
    let result = [];
    let obj = new ctor(5);
    obj[0] = 10n;
    obj[1] = 11n;
    obj[2] = 12n;
    obj[3] = 13n;
    obj[4] = 9017199254740995n;
    let arr2 = obj.with(-2, 40n);
    result.push(arr2[3] == 40n);
    try {
        arr2 = obj.with(10, 10n);
    } catch (err) {
        result.push(err.name == "RangeError");
    }
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}