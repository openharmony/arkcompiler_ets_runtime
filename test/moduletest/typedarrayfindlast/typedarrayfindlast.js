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
 * @tc.name:typedarrayfindlast
 * @tc.desc:test TypedArray.findLast and TypedArray.findLastIndex
 * @tc.type: FUNC
 * @tc.require: issueI7LP2E
 */

const typedArrayFindConstructors = [
    Float64Array, Float32Array, Int32Array, Int16Array, Int8Array, Uint32Array, Uint16Array, Uint8Array,
    Uint8ClampedArray
];

const typedArrayFindConstructorsAssertSuccessList = [
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

typedArrayFindConstructors.forEach(function (ctor, i) {
    if (testTypeArrayAt1(ctor)) {
        typedArrayFindConstructorsSuccessList.push(ctor.name);
    } else {
        typedArrayFindConstructorsFailList.push(ctor.name);
    }
});

assert_equal(typedArrayFindConstructorsSuccessList, typedArrayFindConstructorsAssertSuccessList);
assert_equal(typedArrayFindConstructorsFailList, typedArrayFindConstructorsAssertFailList);

const testTypeArrayAt2SuccessList = [
	"BigInt64Array",
	"BigUint64Array"
	];
const testTypeArrayAt2AssertFailList = [];
let testTypeArrayAt2SuccessList = [];
let testTypeArrayAt2FailList = [];

[
    BigInt64Array,
    BigUint64Array
].forEach(function (ctor, i) {
    if (testTypeArrayAt2(ctor)) {
        testTypeArrayAt2SuccessList.push(ctor.name);
    } else {
        testTypeArrayAt2FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayAt2SuccessList, testTypeArrayAt2AssertSuccessList);
assert_equal(testTypeArrayAt2FailList, testTypeArrayAt2AssertFailList);

function testTypeArrayAt1(ctor) {
    let result = [];
    let obj = new ctor([10, 11, 12, 13, 14]);
    let result1 = obj.findLast((element, index, array) => {
        return (element == 12);
    });
    result.push(result1 == 12);
    result1 = obj.findLast((element, index, array) => {
        return (element < 10);
    });
    result.push(result1 == undefined);
    result1 = obj.findLastIndex((element, index, array) => {
        if (index == 3) {
            array[3] = 100;
        }
        return (element == 100);
    });
    result.push(result1 == -1);
    result1 = obj.findLastIndex((element, index, array) => {
        return (element == 100);
    });
    result.push(result1 == 3);
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArrayAt2(ctor) {
    let result = []
    let obj = new ctor([10n, 11n, 12n, 13n, 9017199254740995n]);
    let result1 = obj.findLast((element, index, array) => {
        return (element == 12n);
    });
    result.push(result1 == 12n);
    result1 = obj.findLast((element, index, array) => {
        return (element < 10n);
    });
    result.push(result1 == undefined);
    result1 = obj.findLastIndex((element, index, array) => {
        if (index == 3) {
            array[3] = 100n;
        }
        return (element == 100n);
    });
    result.push(result1 == -1);
    result1 = obj.findLastIndex((element, index, array) => {
        return (element == 100n);
    });
    result.push(result1 == 3);
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

test_end();