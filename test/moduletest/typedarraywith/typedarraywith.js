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
const typedArrayWithConstructors = [
    Float64Array, Float32Array, Int32Array, Int16Array, Int8Array, Uint32Array, Uint16Array, Uint8Array,
    Uint8ClampedArray
];

const typedArrayWithConstructorsAssertSuccessList = [
	"Float64Array",
	"Float32Array",
	"Int32Array",
	"Int16Array",
	"Int8Array",
	"Uint32Array",
	"Uint16Array",
	"Uint8Array",
	"Uint8ClampedArray"
	];
const typedArrayFindConstructorsAssertFailList = [];
let typedArrayFindConstructorsSuccessList = [];
let typedArrayFindConstructorsFailList = [];

typedArrayWithConstructors.forEach(function (ctor, i) {
    let arr = [1, 2, 3, 4, 5];
    if (testTypeArrayWithInt(ctor, arr, false)) {
        typedArrayFindConstructorsSuccessList.push(ctor.name);
    } else {
        typedArrayFindConstructorsFailList.push(ctor.name);
    }
});

assert_equal(typedArrayFindConstructorsSuccessList, typedArrayFindConstructorsAssertSuccessList);
assert_equal(typedArrayFindConstructorsFailList, typedArrayFindConstructorsAssertFailList);

const bigArrayAssertSuccessList = [
	"BigInt64Array",
	"BigUint64Array"
	];
const bigArrayAssertFailList = [];
let bigArraySuccessList = [];
let bigArrayFailList = [];

[
    BigInt64Array,
    BigUint64Array
].forEach(function (ctor, i) {
    let arr = [10n, 11n, 12n, 13n, 9017199254740995n];
    if (testTypeArrayWithInt(ctor, arr, true)) {
        bigArraySuccessList.push(ctor.name);
    } else {
        bigArrayFailList.push(ctor.name);
    }
});

assert_equal(bigArraySuccessList, bigArrayAssertSuccessList);
assert_equal(bigArrayFailList, bigArrayAssertFailList);

function testTypeArrayWithInt(ctor, arr, flagBig) {
    let obj = new ctor(arr);
    let result = [];
    let value1 = 40;
    let value2 = 10;

    if (flagBig) {
        value1 = 40n;
        value2 = 10n;
    }

    let arr2 = obj.with(-2, value1);
    result.push(arr2[3] == value1);
    try {
        arr2 = obj.with(10, value2);
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

const with_arr1 = new Uint8Array([1, 2, 3, 4, 5]);
assert_equal(with_arr1.with(2, 6).toString(),"1,2,6,4,5");

const with_arr2 = new Uint8Array([1, 2, 3, 4, 5]);
assert_equal(with_arr2.with("2", 6).toString(),"1,2,6,4,5");

test_end();