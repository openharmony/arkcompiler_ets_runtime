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
 * @tc.name:typedarrayat
 * @tc.desc:test TypedArray.at
 * @tc.type: FUNC
 * @tc.require: issueI7F8N1
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
    if (testTypeArrayAt1(ctor)) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

[
    BigInt64Array,
    BigUint64Array
].forEach(function (ctor, i) {
    if (testTypeArrayAt2(ctor)) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

function testTypeArrayAt1(ctor) {
    let result = []
    let obj = new ctor(5);
    obj[0] = 10;
    obj[1] = 11;
    obj[2] = 12;
    obj[3] = 13;
    obj[4] = 14;
    result.push(obj.at(-1) == 14);
    result.push(obj.at(1.5) == 11);
    result.push(obj.at(-3) == 12);
    result.push(obj.at("3") == 13)
    result.push(obj.at(-6) == undefined);
    result.push(obj.at(5) == undefined);
    result.push(obj.at({}) == 10);
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArrayAt2(ctor) {
    let result = []
    let obj = new ctor(5);
    obj[0] = 10n;
    obj[1] = 11n;
    obj[2] = 12n;
    obj[3] = 13n;
    obj[4] = 9017199254740995n;
    result.push(obj.at(-1) == 9017199254740995n);
    result.push(obj.at(1.5) == 11n);
    result.push(obj.at(-3) == 12n);
    result.push(obj.at("3") == 13n)
    result.push(obj.at(-6) == undefined);
    result.push(obj.at(5) == undefined);
    result.push(obj.at({}) == 10n);
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}