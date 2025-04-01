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

/*
 * @tc.name:typearray
 * @tc.desc:test TypeArray
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

const typedArrayConstructors = [
    Float64Array, Float32Array, Int32Array, Int16Array, Int8Array, Uint32Array, Uint16Array, Uint8Array,
    Uint8ClampedArray
];

const typedArrayConstructorsAssertSuccessList = [
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
const typedArrayConstructorsAssertFailList = [];
let typedArrayConstructorsSuccessList = [];
let typedArrayConstructorsFailList = [];

typedArrayConstructors.forEach(function(ctor, i) {
    if (testTypeArray1(ctor) && testTypeArray2(ctor) &&
        testTypeArray3(ctor) && testTypeArray4(ctor) &&
        testTypeArrayWithSize(ctor, 10) && testTypeArrayWithSize(ctor, 50) &&
        testTypeArrayIC(ctor)) {
        typedArrayConstructorsSuccessList.push(ctor.name);
    } else {
        typedArrayConstructorsFailList.push(ctor.name);
    }
});

assert_equal(typedArrayConstructorsSuccessList, typedArrayConstructorsAssertSuccessList);
assert_equal(typedArrayConstructorsFailList, typedArrayConstructorsAssertFailList);

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
].forEach(function(ctor, i) {
    if (testTypeArray5(ctor) ) {
        bigArraySuccessList.push(ctor.name);
    } else {
        bigArrayFailList.push(ctor.name);
    }
});

assert_equal(bigArraySuccessList, bigArrayAssertSuccessList);
assert_equal(bigArrayFailList, bigArrayAssertFailList);

function testTypeArray1(ctor) {
    let result = []
    let obj = new ctor(5);
    result.push(obj[0] == 0);
    result.push(obj[1] == 0);
    obj[0] = "123";
    result.push(obj[0] == 123)
    result.push(obj["a"] == undefined);
    obj["a"] = "abc";
    result.push(obj["a"] == "abc");
    obj["-0"] = 12;
    result.push(obj["-0"] == undefined)
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray2(ctor) {
    let result = []
    let obj = new ctor(5);
    for (let i = 0; i < 5; i++) {
        obj[i] = i;
    }
    let child = Object.create(obj);
    for (let i = 0; i < 5; i++) {
        result.push(child[i] == i);
    }
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray3(ctor) {
    let result = []
    let obj = new ctor(5);
    let parent = {5: 5, "a": "a"};
    obj.__proto__ = parent;
    result.push(obj[4] == 0);
    result.push(obj[5] == undefined);
    result.push(obj["a"] == "a");
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray4(ctor) {
    let a1 = new Array(1024);
    let a2 = new ctor(1024);
    a2.set(a1);
    for (let i = 0; i < a2.length; i++) {
        if (a2[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArray5(ctor) {
    let result = []
    let a1 = new ctor(10);
    a1.set([21n, 2n, 3n], "2");
    result.push(a1[2] == 21n);
    result.push(a1[3] == 2n);
    result.push(a1[4] == 3n);
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArrayWithSize(ctor, size) {
    let result = []
    let test = new Array(size);
    for (var i = 0; i < 10; i++) {
        test[i] = i;
    }
    let obj = new ctor(test);
    for (var i = 0; i < 10; i++) {
        result.push(obj[i] == i);
    }
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

function testTypeArrayIC(ctor) {
    let result = []
    let obj = new ctor(100);
    for (var i = 0; i < 100; i++) {
        obj[i] = i;
    }
    for (var i = 0; i < 100; i++) {
        result.push(obj[i] == i);
    }
    for (var i = 0; i < 100; i++) {
        result.push(obj.at(i) == i);
    }
    for (let i = 0; i < result.length; i++) {
        if (!result[i]) {
            return false;
        }
    }
    return true;
}

let a1 = new ArrayBuffer(1024*1024*8);
let a2 = new Uint8Array(a1);
let a3 = Uint8Array.from(a2);
assert_equal(a3.length,8388608);

const a4 = new Uint32Array(1024);
const obj1 = {
    "c": a4,
    __proto__: a4
}
obj1[235] = 1024;
assert_equal(obj1[235],1024);

try {
    const a5 = new Uint8ClampedArray(new ArrayBuffer(1283053413), 9007199254740991);
    a5.copyWithin(-13602);
    assert_unreachable();
} catch(e) {
}

try {
    const a6 = new BigInt64Array(10);
    Int16Array.apply(null, a6);
    assert_unreachable();
} catch(e) {
}

const a7 = new BigInt64Array(4);
function foo() {}
const f = new foo();
const protoOf = f.isPrototypeOf;
assert_equal(protoOf.apply(protoOf, a7),false);
const uint8 = new Uint8Array([1, 2, 3]);
const reversedUint8 = uint8.toReversed();
assert_equal(reversedUint8.toString(),"3,2,1"); // Uint8Array [3, 2, 1]
assert_equal(uint8.toString(),"1,2,3"); // Uint8Array [1, 2, 3]

try {
    const a8 = new Int8Array(new ArrayBuffer(0x40004141, {"maxByteLength": 0x40004141}));
    const a9 = new Float64Array(a8);
    assert_unreachable();
} catch (e) {
}

try {
    const a10 = [1, 2];
    const a11 = new Uint8Array(a10);
    const a12 = new Uint32Array(a11);
    a12.set(a10, 0x1ffffffff);
    assert_unreachable();
} catch (e) {
}

try {
    const v17 = new Int16Array(5);
    const v20 = new Int16Array(5);
    v17.set(v20, 4294967295);
    assert_unreachable();
} catch (error) {
    assert_equal(error instanceof RangeError, true);
}

try {
    const v17 = new Int16Array(5);
    const v20 = new Int16Array(5);
    v17.set(v20, 4294967296);
    assert_unreachable();
} catch (error) {
    assert_equal(error instanceof RangeError, true);
}

try {
    new BigUint64Array(new ArrayBuffer(256), 256, 0x1fffffff)
    assert_unreachable();
} catch (error) {
    assert_equal(error instanceof RangeError, true);
}


let arr12 = new Uint8Array(256).fill(255);
assert_equal(arr12[0] == 255,true);
assert_equal(arr12[123] == 255,true);
assert_equal(arr12[255] == 255,true);

try {
    new Uint8Array(2 ** 32 - 1);
    assert_unreachable();
} catch (error) {
    assert_equal(error instanceof RangeError, true);
}

const v21 = new SharedArrayBuffer(32);
const v22 = new BigInt64Array(v21);
Atomics.or(v22, Int16Array, false);
assert_equal(v22.toString(),"0,0,0,0");
assert_equal(Atomics.wait(v22, false, true),"not-equal");

var arr13 = { [0]: 1, [1]: 20, [2]: 300, [3]: 4000, length: 4};
var proxy = new Proxy(arr13, {
    get: function(target, name) {
        if (name == Symbol.iterator) {
            return undefined;
        }
        return target[name];
    }
})
var arr14 = new Uint8Array(proxy);
assert_equal(arr13.length == arr14.length,true)


const v2 = ("4294967297")["replaceAll"]();
const v4 = new Float32Array();
const t11 = v4.__proto__;
t11[v2] = v2;
assert_equal(t11[v2],"4294967297");

const v3 = String.fromCharCode(564654156456456465465)
const v5 = new Int16Array(true);
assert_equal(v5["join"](v3),"0");

// Test case for filter()
let arr;
var calls = 0;
function TestTypedArrayFilter(e) {
    arr = new e([-5, 10, 20, 30, 40, 50, 60.6]);
}

function TestTypedArrayFilterFunc(element, index, array) {
    return element >= 10;
}

TestTypedArrayFilter(Int8Array);
assert_equal(arr.filter(TestTypedArrayFilterFunc).toString(),"10,20,30,40,50,60");
assert_equal(calls,0);

Object.defineProperty(Int8Array.prototype, "constructor", {
    get: function () {
        calls++;
    }
});

TestTypedArrayFilter(Int8Array);
assert_equal(arr.filter(TestTypedArrayFilterFunc).toString(),"10,20,30,40,50,60");
assert_equal(calls,1);

// test case for some()
const testTypeArraySomeAssertSuccessList = [
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
const testTypeArraySomeAssertFailList = [];
let testTypeArraySomeSuccessList = [];
let testTypeArraySomeFailList = [];

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
if (testTypeArraySome(ctor)) {
    	testTypeArraySomeSuccessList.push(ctor.name);
    } else {
    	testTypeArraySomeFailList.push(ctor.name);
    }
});

assert_equal(testTypeArraySomeSuccessList, testTypeArraySomeAssertSuccessList);
assert_equal(testTypeArraySomeFailList, testTypeArraySomeAssertFailList);

function testTypeArraySome(ctor) {
    let obj = new ctor([-1, 0, 2, 5, 8]);
    return obj.some((element, index, array) => element > 5);
}

// Test case for every()
let arr1_every = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
function testEvery_true(ele) {
    return ele > 0;
}
function testEvery_false(ele) {
    return ele > 10;
}
assert_equal(arr1_every.every(testEvery_true),true);
assert_equal(arr1_every.every(testEvery_false),false);

let arr2_every = new Int16Array();
assert_equal(arr2_every.every(testEvery_false),true);
let arr3_every = [1, 2, 3, 4, 5, 6, 7, 8, 9];
assert_equal(arr3_every.every(testEvery_true),true);

// Test case for reduce()
let arr1_reduce = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
function test_Reduce(a, b){
    return a+b;
}
assert_equal(arr1_reduce.reduce(test_Reduce),45);
assert_equal(arr1_reduce.reduce(test_Reduce, 10),55);
let arr2_reduce = [1, 2, 3, 4, 5, 6, 7, 8, 9];
assert_equal(arr2_reduce.reduce(test_Reduce),45);

// Test case for reduceRight()
let arr1_reduceRight = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
function test_ReduceRight(a, b){
    return a+b;
}
assert_equal(arr1_reduceRight.reduceRight(test_ReduceRight),45);
assert_equal(arr1_reduceRight.reduceRight(test_ReduceRight, 10),55);
let arr2_reduceRight = [1, 2, 3, 4, 5, 6, 7, 8, 9];
assert_equal(arr2_reduceRight.reduceRight(test_ReduceRight),45);

// Test case for copyWithin()
let arr1_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
assert_equal(arr1_copyWithin.copyWithin(-10, 1, 100).toString(),"2,3,4,5,6,7,8,9,9");
let arr2_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
assert_equal(arr2_copyWithin.copyWithin(-3, -100, -1).toString(),"1,2,3,4,5,6,1,2,3");
let arr3_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
assert_equal(arr3_copyWithin.copyWithin(4, 1, 10).toString(),"1,2,3,4,2,3,4,5,6");
let arr4_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
assert_equal(arr4_copyWithin.copyWithin(4, -2).toString(),"1,2,3,4,8,9,7,8,9");
let arr5_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
assert_equal(arr5_copyWithin.copyWithin(4).toString(),"1,2,3,4,1,2,3,4,5");
let arr6_copyWithin = [1, 2, 3, 4, 5, 6, 7, 8, 9];
assert_equal(arr6_copyWithin.copyWithin(1).toString(),"1,1,2,3,4,5,6,7,8");

// Test case for findIndex()

const testTypeArrayFindIndexAssertSuccessList = [
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
const testTypeArrayFindIndexAssertFailList = [];
let testTypeArrayFindIndexSuccessList = [];
let testTypeArrayFindIndexFailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArrayFindIndex(ctor)) {
    	testTypeArrayFindIndexSuccessList.push(ctor.name);
    } else {
    	testTypeArrayFindIndexFailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayFindIndexSuccessList, testTypeArrayFindIndexAssertSuccessList);
assert_equal(testTypeArrayFindIndexFailList, testTypeArrayFindIndexAssertFailList);

function testFindIndex(element, Last, array) {
    return element >= 60;
}

function testTypeArrayFindIndex(ctor) {
    let obj = new ctor([5, 10, 20, 30, 40, 50, 60])
    let result = obj.findIndex(testFindIndex);
    return result != -1;
}

// Test case for includes()
let arr1_includes = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
assert_equal(arr1_includes.includes(5, -100),true);
assert_equal(arr1_includes.includes(55,-1),false);
let arr2_includes = [1, 2, 3, 4, 5, 6, 7, 8, 9];
assert_equal(arr2_includes.includes(5),true);

// Test case for find()
let arr1_find = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
function testFind_true(ele) {
    return ele === 5;
}
function testFind_false(ele) {
    return ele > 10;
}
assert_equal(arr1_find.find(testFind_true),5);
assert_equal(arr1_find.find(testFind_false),undefined);

let arr2_find = new Int16Array();
assert_equal(arr2_find.find(testFind_false),undefined);
let arr3_find = [1, 2, 3, 4, 5, 6, 7, 8, 9];
assert_equal(arr3_find.find(testFind_true),5);

// Test case for indexOf()

const testTypeArrayIndexOfAssertSuccessList = [
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
const testTypeArrayIndexOfAssertFailList = [];
let testTypeArrayIndexOfSuccessList = [];
let testTypeArrayIndexOfFailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArrayIndexOf(ctor)) {
    	testTypeArrayIndexOfSuccessList.push(ctor.name);
    } else {
    	testTypeArrayIndexOfFailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayIndexOfSuccessList, testTypeArrayIndexOfAssertSuccessList);
assert_equal(testTypeArrayIndexOfFailList, testTypeArrayIndexOfAssertFailList);

function testTypeArrayIndexOf(ctor) {
    let obj = new ctor([5, 10, 20, 30, 40, 50, 60])
    let result = obj.indexOf(60);
    return result != -1;
}

// Test case for lastIndexOf()

const testTypeArrayLastIndexOfAssertSuccessList = [
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
const testTypeArrayLastIndexOfAssertFailList = [];
let testTypeArrayLastIndexOfSuccessList = [];
let testTypeArrayLastIndexOfFailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArrayLastIndexOf(ctor)) {
    	testTypeArrayLastIndexOfSuccessList.push(ctor.name);
    } else {
    	testTypeArrayLastIndexOfFailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayLastIndexOfSuccessList, testTypeArrayLastIndexOfAssertSuccessList);
assert_equal(testTypeArrayLastIndexOfFailList, testTypeArrayLastIndexOfAssertFailList);

function testTypeArrayLastIndexOf(ctor) {
    let obj = new ctor([5, 10, 20, 30, 40, 50, 60]);
    let result = obj.lastIndexOf(5)
    return result != -1;
}

const testTypeArrayLastIndexOf1AssertSuccessList = [
	"BigInt64Array",
	"BigUint64Array"
	];
const testTypeArrayLastIndexOf1AssertFailList = [];
let testTypeArrayLastIndexOf1SuccessList = [];
let testTypeArrayLastIndexOf1FailList = [];

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArrayLastIndexOf1(ctor) ) {
        testTypeArrayLastIndexOf1SuccessList.push(ctor.name);
    } else {
        testTypeArrayLastIndexOf1FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayLastIndexOf1SuccessList, testTypeArrayLastIndexOf1AssertSuccessList);
assert_equal(testTypeArrayLastIndexOf1FailList, testTypeArrayLastIndexOf1AssertFailList);

function testTypeArrayLastIndexOf1(ctor) {
    let obj = new ctor([5n, 10n, 20n, 30n, 40n, 50n, 60n]);
    let result = obj.lastIndexOf(5n)
    return result != -1;
}

let lastIndexOfFailed = new Int16Array(5, 10, 20, 30, 40, 50, 60)
let lastIndexOfFailedResult = lastIndexOfFailed.lastIndexOf('ABC')
assert_equal(lastIndexOfFailedResult,-1);

// Test case for reverse()
const testTypeArrayReverse1AssertSuccessList = [
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
const testTypeArrayReverse1AssertFailList = [];
let testTypeArrayReverse1SuccessList = [];
let testTypeArrayReverse1FailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArrayReverse1(ctor)) {
        testTypeArrayReverse1SuccessList.push(ctor.name);
    } else {
        testTypeArrayReverse1FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayReverse1SuccessList, testTypeArrayReverse1AssertSuccessList);
assert_equal(testTypeArrayReverse1FailList, testTypeArrayReverse1AssertFailList);

function testTypeArrayReverse1(ctor) {
    let arr1 = new ctor([1, 2, 3, 4, 5]);
    arr1.reverse();
    let arr2 = new ctor([5, 4, 3, 2, 1]);
    if (arr1.length !== arr2.length) return false;
    for (let i = 0; i < arr1.length; i++) {
        if (arr1[i] !== arr2[i]) return false;
    }
    return true;
}

const testTypeArrayReverse2AssertSuccessList = [
	"BigInt64Array",
	"BigUint64Array"
	];
const testTypeArrayReverse2AssertFailList = [];
let testTypeArrayReverse2SuccessList = [];
let testTypeArrayReverse2FailList = [];

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArrayReverse2(ctor) ) {
        testTypeArrayReverse2SuccessList.push(ctor.name);
    } else {
        testTypeArrayReverse2FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayReverse2SuccessList, testTypeArrayReverse2AssertSuccessList);
assert_equal(testTypeArrayReverse2FailList, testTypeArrayReverse2AssertFailList);

function testTypeArrayReverse2(ctor) {
    let arr1 = new ctor([1n, 2n, 3n, 4n, 5n]);
    arr1.reverse();
    let arr2 = new ctor([5n, 4n, 3n, 2n, 1n]);
    if (arr1.length !== arr2.length) return false;
    for (let i = 0; i < arr1.length; i++) {
        if (arr1[i] !== arr2[i]) return false;
    }
    return true;
}

function fun1(accumulator, currentvalue) {
    return accumulator + currentvalue;
}
let arr1 = new Uint32Array();
let arr2 = new Uint32Array(3);
for (let i = 0; i < 3; i++) {
    arr2[i] = i + 1;
}
let res1 = arr1.reduceRight(fun1, 1, 1);
assert_equal(res1,1);
let res2 = arr2.reduceRight(fun1, 1, 1);
assert_equal(res2,7);
let res3 = arr1.reduceRight(fun1, 1);
assert_equal(res3,1);
let res4 = arr2.reduceRight(fun1, 1);
assert_equal(res4,7);
try {
    let res5 = arr1.reduceRight(fun1);
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError, true);
}
let res6 = arr2.reduceRight(fun1);
assert_equal(res6,6);
let res7 = arr1.reduceRight(fun1, undefined);
assert_equal(res7,undefined);
let res8 = arr2.reduceRight(fun1, undefined);
assert_equal(res8.toString(),"NaN");
let res9 = arr1.reduceRight(fun1, null);
assert_equal(res9,null);
let res10 = arr2.reduceRight(fun1, null);
assert_equal(res10,6);

for (let i = 0; i < 3; i++) {
  arr2[i] = i + 1;
}
res1 = arr1.reduce(fun1, 1, 1);
assert_equal(res1,1);
res2 = arr2.reduce(fun1, 1, 1);
assert_equal(res2,7);
res3 = arr1.reduce(fun1, 1);
assert_equal(res3,1);
res4 = arr2.reduce(fun1, 1);
assert_equal(res4,7);
try {
  let res5 = arr1.reduce(fun1);
  assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError, true);
}
res6 = arr2.reduce(fun1);
assert_equal(res6,6);
res7 = arr1.reduce(fun1, undefined);
assert_equal(res7,undefined);
res8 = arr2.reduce(fun1, undefined);
assert_equal(res8.toString(),"NaN");
res9 = arr1.reduce(fun1, null);
assert_equal(res9,null);
res10 = arr2.reduce(fun1, null);
assert_equal(res10,6);

// Test case for findLastIndex()
const testTypeArrayfindLastIndexAssertSuccessList = [
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
const testTypeArrayfindLastIndexAssertFailList = [];
let testTypeArrayfindLastIndexSuccessList = [];
let testTypeArrayfindLastIndexFailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArrayfindLastIndex(ctor)) {
        testTypeArrayfindLastIndexSuccessList.push(ctor.name);
    } else {
        testTypeArrayfindLastIndexFailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayfindLastIndexSuccessList, testTypeArrayfindLastIndexAssertSuccessList);
assert_equal(testTypeArrayfindLastIndexFailList, testTypeArrayfindLastIndexAssertFailList);

function func(element, Last, array) {
    return element >= 0;
}

function testTypeArrayfindLastIndex(ctor) {
    let obj = new ctor([5, 10, 20, 30, 40, 50, 60])
    let result = obj.findLastIndex(func);
    return result != -1;
}

const testTypeArrayfindLastIndex1AssertSuccessList = [
	"BigInt64Array",
	"BigUint64Array"
	];
const testTypeArrayfindLastIndex1AssertFailList = [];
let testTypeArrayfindLastIndex1SuccessList = [];
let testTypeArrayfindLastIndex1FailList = [];

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArrayfindLastIndex1(ctor) ) {
        testTypeArrayfindLastIndex1SuccessList.push(ctor.name);
    } else {
        testTypeArrayfindLastIndex1FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayfindLastIndex1SuccessList, testTypeArrayfindLastIndex1AssertSuccessList);
assert_equal(testTypeArrayfindLastIndex1FailList, testTypeArrayfindLastIndex1AssertFailList);

function testTypeArrayfindLastIndex1(ctor) {
    let obj = new ctor([5n, 10n, 20n, 30n, 40n, 50n, 60n])
    let result = obj.findLastIndex(func);
    return result != -1;
}

// Test case for of()
const testTypeArrayOf1AssertSuccessList = [
	"Int32Array",
	"Int16Array",
	"Int8Array",
	"Uint32Array",
	"Uint16Array",
	"Uint8Array",
	"Uint8ClampedArray"
	];
const testTypeArrayOf1AssertFailList = [
	"Float64Array",
	"Float32Array",
	];
let testTypeArrayOf1SuccessList = [];
let testTypeArrayOf1FailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArrayOf1(ctor) && testTypeArrayOf2(ctor)) {
        testTypeArrayOf1SuccessList.push(ctor.name);
    } else {
        testTypeArrayOf1FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayOf1SuccessList, testTypeArrayOf1AssertSuccessList);
assert_equal(testTypeArrayOf1FailList, testTypeArrayOf1AssertFailList);

function testTypeArrayOf1(ctor) {
    let arr1 = ctor.of("1", 2, undefined);
    let arr2 = new ctor([1, 2, undefined])
    return typedArraysEqual(arr1, arr2);
}

function testTypeArrayOf2(ctor) {
    var obj1 = {
        valueOf() {
          return 41;
        }
      };
      var obj2 = {
        valueOf() {
          return 42;
        }
      };
    let arr1 = ctor.of(obj1, obj2, obj1);
    let arr2 = new ctor([41, 42, 41])
    return typedArraysEqual(arr1, arr2);
}

const testTypeArrayOf3AssertSuccessList = [
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
const testTypeArrayOf3AssertFailList = [];
let testTypeArrayOf3SuccessList = [];
let testTypeArrayOf3FailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArrayOf3(ctor)) {
        testTypeArrayOf3SuccessList.push(ctor.name);
    } else {
        testTypeArrayOf3FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayOf3SuccessList, testTypeArrayOf3AssertSuccessList);
assert_equal(testTypeArrayOf3FailList, testTypeArrayOf3AssertFailList);

function testTypeArrayOf3(ctor) {
    try {
        const arr1 = new ctor();
        arr1.proto = ctor;
        Uint8Array.of("m");
    } catch (e) {
        return false;
    }
    return true;
}

function typedArraysEqual(typedArr1, typedArr2) {
    if (typedArr1.length !== typedArr2.length) return false;
    for (let i = 0; i < typedArr1.length; i++) {
      if (typedArr1[i] !== typedArr2[i]) return false;
    }
    return true;
}

// typedArray.map()

const testTypeArray1AssertSuccessList = [
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
const testTypeArray1AssertFailList = [];
let testTypeArray1SuccessList = [];
let testTypeArray1FailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArray1(ctor)) {
        testTypeArray1SuccessList.push(ctor.name);
    } else {
        testTypeArray1FailList.push(ctor.name);
    }
});

assert_equal(testTypeArray1SuccessList, testTypeArray1AssertSuccessList);
assert_equal(testTypeArray1FailList, testTypeArray1AssertFailList);

  
function testTypeArray1(ctor) {
    let obj = new ctor([2]);
    let result = obj.map(function (num) {
        return num * 2;
    });
    return result[0] == 4;
}

// test case for toReversed()

const testTypeArrayToReversed1AssertSuccessList = [
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
const testTypeArrayToReversed1AssertFailList = [];
let testTypeArrayToReversed1SuccessList = [];
let testTypeArrayToReversed1FailList = [];

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
].forEach(function(ctor, i) {
    if (testTypeArrayToReversed1(ctor)) {
        testTypeArrayToReversed1SuccessList.push(ctor.name);
    } else {
        testTypeArrayToReversed1FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayToReversed1SuccessList, testTypeArrayToReversed1AssertSuccessList);
assert_equal(testTypeArrayToReversed1FailList, testTypeArrayToReversed1AssertFailList);

function testTypeArrayToReversed1(ctor) {
    let arr1 = new ctor([1, 2, 3, 4, 5]);
    let arr2 = arr1.toReversed();
    let arr3 = new ctor([5, 4, 3, 2, 1]);
    if (arr2.length !== arr3.length) return false;
    for (let i = 0; i < arr2.length; i++) {
        if (arr2[i] !== arr3[i]) return false;
    }
    return true;
}

const testTypeArrayToReversed2AssertSuccessList = [
	"BigInt64Array",
	"BigUint64Array"
	];
const testTypeArrayToReversed2AssertFailList = [];
let testTypeArrayToReversed2SuccessList = [];
let testTypeArrayToReversed2FailList = [];

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArrayToReversed2(ctor) ) {
        testTypeArrayToReversed2SuccessList.push(ctor.name);
    } else {
        testTypeArrayToReversed2FailList.push(ctor.name);
    }
});

assert_equal(testTypeArrayToReversed2SuccessList, testTypeArrayToReversed2AssertSuccessList);
assert_equal(testTypeArrayToReversed2FailList, testTypeArrayToReversed2AssertFailList);


function testTypeArrayToReversed2(ctor) {
    let arr1 = new ctor([1n, 2n, 3n, 4n, 5n]);
    let arr2 = arr1.toReversed();
    let arr3 = new ctor([5n, 4n, 3n, 2n, 1n]);
    if (arr2.length !== arr3.length) return false;
    for (let i = 0; i < arr2.length; i++) {
        if (arr2[i] !== arr3[i]) return false;
    }
    return true;
}

var arr_every = new Uint8Array([11, 22, 33, 44]);
ArkTools.arrayBufferDetach(arr_every.buffer);
try {
    arr_every.every(() => true)
} catch (e) {
    assert_equal(e instanceof TypeError,true);
}

var arr_forEach = new Uint8Array([11, 22, 33, 44]);
ArkTools.arrayBufferDetach(arr_forEach.buffer);
try {
    arr_forEach.forEach(() => true)
} catch (e) {
    assert_equal(e instanceof TypeError,true);
}

var typedArrayConstructorsSort = [
    Uint8Array,
    Int8Array,
    Uint16Array,
    Int16Array,
    Uint32Array,
    Int32Array,
    Uint8ClampedArray,
    Float32Array,
    Float64Array
];

var typedArrayConstructorsSortAssertValue = {
    Uint8Array: ["0","0","0","0","0","0","1","2","3"],
    Int8Array: ["0","0","0","0","0","0","1","2","3"],
    Uint16Array: ["0","0","0","0","0","0","1","2","3"],
    Int16Array: ["0","0","0","0","0","0","1","2","3"],
    Uint32Array: ["0","0","0","0","0","0","1","2","3"],
    Int32Array: ["0","0","0","0","0","0","1","2","3"],
    Uint8ClampedArray: ["0","0","0","0","0","0","1","2","3"],
    Float32Array:["-0","-0","0","0","1","2","3","NaN","NaN"],
    Float64Array:["-0","-0","0","0","1","2","3","NaN","NaN"],
}

for (var constructor of typedArrayConstructorsSort) {
    // For arrays of floats, certain handling of +-0/NaN
    var b = new constructor([1, +0, -0, NaN, -0, NaN, +0, 3, 2])
    b.sort();
    var constructorValue = [
        prettyPrinted(b[0]), prettyPrinted(b[1]), prettyPrinted(b[2]),
        prettyPrinted(b[3]), prettyPrinted(b[4]), prettyPrinted(b[5]),
        prettyPrinted(b[6]), prettyPrinted(b[7]), prettyPrinted(b[8])
    ]
    assert_equal(constructorValue,typedArrayConstructorsSortAssertValue[constructor.name]);
}

function prettyPrinted(value) {
    let visited = new Set();
    function prettyPrint(value) {
        try {
            switch (typeof value) {
                case "string":
                    return JSONStringify(value);
                case "bigint":
                    return String(value) + "n";
                case "number":
                    if (value === 0 && (1 / value) < 0) return "-0";
                // FALLTHROUGH.
                case "boolean":
                case "undefined":
                case "function":
                case "symbol":
                    return String(value);
                case "object":
                    if (value === null) return "null";
                    // Guard against re-visiting.
                    if (visited.has(value)) return "<...>";
                    visited.add(value);
                    var objectClass = classOf(value);
                    switch (objectClass) {
                        case "Number":
                        case "BigInt":
                        case "String":
                        case "Boolean":
                        case "Date":
                            return objectClass + "(" + prettyPrint(ValueOf(value)) + ")";
                        case "RegExp":
                            return RegExpPrototypeToString.call(value);
                        case "Array":
                            var mapped = ArrayPrototypeMap.call(
                                value, (v, i, array) => {
                                    if (v === undefined && !(i in array)) return "";
                                    return prettyPrint(v, visited);
                                });
                            var joined = ArrayPrototypeJoin.call(mapped, ",");
                            return "[" + joined + "]";
                        case "Int8Array":
                        case "Uint8Array":
                        case "Uint8ClampedArray":
                        case "Int16Array":
                        case "Uint16Array":
                        case "Int32Array":
                        case "Uint32Array":
                        case "Float32Array":
                        case "Float64Array":
                        case "BigInt64Array":
                        case "BigUint64Array":
                            var joined = ArrayPrototypeJoin.call(value, ",");
                            return objectClass + "([" + joined + "])";
                        case "Object":
                            break;
                        default:
                            return objectClass + "(" + String(value) + ")";
                    }
                    // classOf() returned "Object".
                    var name = value.constructor?.name ?? "Object";
                    var pretty_properties = [];
                    for (let [k, v] of Object.entries(value)) {
                        ArrayPrototypePush.call(
                            pretty_properties, `${k}:${prettyPrint(v, visited)}`);
                    }
                    var joined = ArrayPrototypeJoin.call(pretty_properties, ",");
                    return `${name}({${joined}})`;
                default:
                    return "-- unknown value --";
            }
        } catch (e) {
            // Guard against general exceptions (especially stack overflows).
            return "<error>"
        }
    }
    return prettyPrint(value);
}

const testfunctionAssertSuccessList = [
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
let testfunctionSuccessList = [];

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
].forEach(function(ctor, i) {
    class C extends ctor{

    }
    C.of();
    testfunctionSuccessList.push(ctor.name);
});
assert_equal(testfunctionSuccessList, testfunctionAssertSuccessList);

const testfunction2AssertSuccessList = [
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
let testfunction2SuccessList = [];

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
    try {
        let obj = {
            __proto__: [1, 2, 3, 4],
            length: 1n,
        }
        new ctor(obj);
        assert_unreachable();
    } catch (e) {
        testfunction2SuccessList.push(ctor.name);
    }
});
assert_equal(testfunction2SuccessList, testfunction2AssertSuccessList);

try { 
    new Uint8Array(5).map(function() {
		ArkTools.ArrayBufferDetach(this.buffer);
    })
    assert_unreachable();
} catch(e) {
    assert_equal(e instanceof TypeError,true);
}

var array_reverse = new Uint8Array([11, 22, 33, 44]);
ArkTools.arrayBufferDetach(array_reverse.buffer);
try {
    array_reverse.reverse();
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError,true);
}

var arr_indexOf = new Uint8Array([11, 22, 33]);
ArkTools.arrayBufferDetach(arr_indexOf.buffer);
try {
    arr_indexOf.indexOf(0);
    assert_unreachable();
} catch(e) {
    assert_equal(e instanceof TypeError,true);
}

var arr_lastIndexOf = new Uint8Array([11, 22, 33]);
ArkTools.arrayBufferDetach(arr_lastIndexOf.buffer);
try {
    arr_lastIndexOf.lastIndexOf(0);
    assert_unreachable();
} catch(e) {
    assert_equal(e instanceof TypeError,true);
}

var arr_map = new Uint8Array([11, 22, 33, 44]);
ArkTools.arrayBufferDetach(arr_map.buffer);
try {
    arr_map.map((v) => v)
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError,true);
}

var arr_filter = new Uint8Array([11, 22]);
ArkTools.arrayBufferDetach(arr_filter.buffer);
try {
    arr_filter.filter(false)
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError,true);
}

var arr_some = new Uint8Array([11, 22]);
ArkTools.arrayBufferDetach(arr_some.buffer);
try {
    arr_some.some(false)
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError,true);
}

var arr_some1 = new Uint8Array([33, 44]);
Object.defineProperty(arr_some1, 'length', { value: 1 });
assert_equal(arr_some1.some(function(elt) { return elt == 44; }),true);
assert_equal(Array.prototype.some.call(arr_some1, function(elt) {
    return elt == 44;
}),false);

var arr_fill = new Uint8Array([2, 2]);
Object.defineProperty(arr_fill, 'length', {value: 1});
arr_fill.fill(3);
Array.prototype.fill.call(arr_fill, 4);
assert_equal(4 == arr_fill[0],true);
assert_equal(3 == arr_fill[1],true);
ArkTools.arrayBufferDetach(arr_fill.buffer);
try {
    arr_fill.fill(0);
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError,true);
}

var arr_fill1 = new Uint8ClampedArray([0, 0, 0]).fill(2.50000);
assert_equal(arr_fill1.toString(),"3,3,3")

try {
    var obj = {intt8: Int8Array};
    print(obj.intt8(16));
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof TypeError, true);
}

try {
    const typedd = new Int8Array(Symbol());
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof TypeError, true);
}

try {
    const typedd1 = new Int32Array(-100.3);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

const typedd2 = new Int32Array(4294967296 + 1);
assert_equal(typedd2.length,1);

const typedd3 = new Int32Array(512 * 8* 4);
assert_equal(typedd3.byteLength,65536);
assert_equal(typedd3.byteOffset,0);
assert_equal(typedd3.length,16384);

var buffer = new ArrayBuffer(512 * 8* 4);

try {
    const typedd4 = new Int32Array(buffer, 11);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    const typedd5 = new Int32Array(buffer, -100.5);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    const typedd6 = new Int32Array(buffer, Symbol());
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof TypeError, true);
}

try {
    var buffer1 = new ArrayBuffer(13);
    const typedd7 = new Int32Array(buffer1, 16);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

var buffer2 = new ArrayBuffer(16);
try {
    const typedd8 = new Float64Array(buffer2, 24);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    const typedd9 = new Float64Array(buffer2, 8, 5);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

const typedd10 = new Int8Array(buffer2, 3, 7);
assert_equal(typedd10.byteLength,7);
assert_equal(typedd10.byteOffset,3);
assert_equal(typedd10.length,7);

try {
    let bufferTest = new ArrayBuffer(2147483648);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    let bufferTest = new ArrayBuffer(2147483694.55);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    let bufferTest = new ArrayBuffer(-49.55);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    let bufferTest = new ArrayBuffer(48  - 1);
    const typedd11 = new Int32Array(bufferTest, 2147483648 + 7);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    let bufferTest = new ArrayBuffer(48  - 8);
    const typedd12 = new Int32Array(bufferTest, 8, 2147483648 + 15);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    const typedd13 = new Int8Array(2147483648 + 3);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

try {
    var bufferTest1 = new ArrayBuffer(1024);
    const typedd14 = new Int8Array(bufferTest1, 2147483648 + 12);
    assert_unreachable();
} catch (e){
    assert_equal(e instanceof RangeError, true);
}

var source11 = new Uint16Array(16);
var target15 = new Int16Array(source11);
assert_equal(target15.byteLength,32);
assert_equal(target15.byteOffset,0);
assert_equal(target15.length,16);

var target16 = new Uint32Array([1,2,3,4]);
assert_equal(target16.toString(),"1,2,3,4");

var target17 = new Int16Array(target16);
assert_equal(target17.toString(),"1,2,3,4");

let uint8Array = new Uint8Array([233, -1, 4.48, 255, 275]);
assert_equal(uint8Array[0],233);
assert_equal(uint8Array[1],255);
assert_equal(uint8Array[2],4);
assert_equal(uint8Array[3],255);
assert_equal(uint8Array[4],19);

try {
    new Uint8Array([233, -1, Symbol('ss')]);
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError, true);
}

let int8Array = new Int8Array([2354.44, -128, -128.4, 12, -3212.84, 127]);
assert_equal(int8Array[0],50);
assert_equal(int8Array[1],-128);
assert_equal(int8Array[2],-128);
assert_equal(int8Array[3],12);
assert_equal(int8Array[4],116);
assert_equal(int8Array[5],127);

try {
    new Int8Array([2354.44, -43.4, 12, Symbol('kk'), -3212.84]);
    assert_unreachable();
} catch (e) {
    assert_equal(e instanceof TypeError, true);
}

let uint8ClampedArray = new Uint8ClampedArray([2354.44, -43.4, 12.6, -3212.84, 255]);
assert_equal(uint8ClampedArray[0],255);
assert_equal(uint8ClampedArray[1],0);
assert_equal(uint8ClampedArray[2],13);
assert_equal(uint8ClampedArray[3],0);
assert_equal(uint8ClampedArray[4],255);

let int16Array = new Int16Array([2354.44, -32768, 12.6, -32768.84, -5893, 32767, 42767]);
assert_equal(int16Array[0],2354);
assert_equal(int16Array[1],-32768);
assert_equal(int16Array[2],12);
assert_equal(int16Array[3],-32768);
assert_equal(int16Array[4],-5893);
assert_equal(int16Array[5],32767);
assert_equal(int16Array[6],-22769);

let uint16Array = new Uint16Array([2354.44, -42767, 12.6, -3212.84, 67535, 65535]);
assert_equal(uint16Array[0],2354);
assert_equal(uint16Array[1],22769);
assert_equal(uint16Array[2],12);
assert_equal(uint16Array[3],62324);
assert_equal(uint16Array[4],1999);
assert_equal(uint16Array[5],65535);

let int32Array = new Int32Array([2354.44, -2147483648, 12.6, -2147483648.84, -5893, 2147483647, 2347483647]);
assert_equal(int32Array[0],2354);
assert_equal(int32Array[1],-2147483648);
assert_equal(int32Array[2],12);
assert_equal(int32Array[3],-2147483648);
assert_equal(int32Array[4],-5893);
assert_equal(int32Array[5],2147483647);
assert_equal(int32Array[6],-1947483649);

let uint32Array = new Uint32Array([2354.44, -2147483648, 12.6, -5893, 4294967295, 4394967295]);
assert_equal(uint32Array[0],2354);
assert_equal(uint32Array[1],2147483648);
assert_equal(uint32Array[2],12);
assert_equal(uint32Array[3],4294961403);
assert_equal(uint32Array[4],4294967295);
assert_equal(uint32Array[5],99999999);

let float32Array = new Float32Array([2354.44, -2147483648, 12.6, -5893.3483, 4294967295, 4394967295.3201]);
assert_equal(float32Array[0],2354.43994140625);
assert_equal(float32Array[1],-2147483648);
assert_equal(float32Array[2],12.600000381469727);
assert_equal(float32Array[3],-5893.34814453125);
assert_equal(float32Array[4],4294967296);
assert_equal(float32Array[5],4394967040);

let float64Array = new Float64Array([2354.44, -2147483648, 12.6, -5893.3483, 4294967295, 4394967295.3201]);
assert_equal(float64Array[0],2354.44);
assert_equal(float64Array[1],-2147483648);
assert_equal(float64Array[2],12.6);
assert_equal(float64Array[3],-5893.3483);
assert_equal(float64Array[4],4294967295);
assert_equal(float64Array[5],4394967295.3201);


let aa = new Array(5000);
for (let i = 0; i < 4900; i++) {
    aa[i] = i;
}
let kk = new Int32Array(aa);
assert_equal(kk[3],3);
assert_equal(kk[4911],0);
assert_equal(kk[4999],0);

let obj11 = {[Symbol.toPrimitive] : function (a) {
    new Date();
    return 111;
}};

let bb = new Int16Array([obj11, 302.32]);
assert_equal(bb[0],111);
assert_equal(bb[1],302);
var typedArrForTestIter1 = new Uint32Array([1,2,3,4]);
var typedArrForTestIter1AssertEqual = [1,2,3,4];
let typedArrForTestIter1AssertRes = [];
for (let typedArrEle of typedArrForTestIter1) {
    typedArrForTestIter1AssertRes.push(typedArrEle);
}
assert_equal(typedArrForTestIter1AssertRes,typedArrForTestIter1AssertEqual);

var typedArrForTestIter2 = new Uint32Array([1,2,3,4]);
var iterForTypedArrKey = typedArrForTestIter2.keys();
var iterForTypedArrKeyAssertEqual = [0,1,2,3];
let iterForTypedArrKeyAssertRes = [];
for (let typedArrEle of iterForTypedArrKey) {
    iterForTypedArrKeyAssertRes.push(typedArrEle);
}
assert_equal(iterForTypedArrKeyAssertRes,iterForTypedArrKeyAssertEqual);

var typedArrForTestIter2 = new Uint32Array([1,2,3,4]);
var iterForTypedArrEntry = typedArrForTestIter2.entries();
var typedArrEleAssertEqual = [[0,1],[1,2],[2,3],[3,4]];
let typedArrEleAssertRes = [];
var typedArrEleAssertLengthEqual = [2,2,2,2];
let typedArrEleAssertLengthRes = [];
for (let typedArrEle of iterForTypedArrEntry) {
    typedArrEleAssertRes.push(typedArrEle);
    typedArrEleAssertLengthRes.push(typedArrEle.length)
}
assert_equal(typedArrEleAssertRes,typedArrEleAssertEqual);
assert_equal(typedArrEleAssertLengthRes,typedArrEleAssertLengthEqual);

{
    var arrayBuffer = new ArrayBuffer(8);
    var floatArray = new Float32Array(arrayBuffer);
    var floatArray2 = new Float32Array(1);
    function setBit(obj, bitCount, value, isDouble) {
        var currentBit = 1 << 31;
        var current = 0;
        for (var i = 0; i < bitCount; i++) {
            currentBit = 1 << (31 - i);
            current = current | currentBit;
        }
        var tmp = new Uint32Array(obj.buffer);
        if (isDouble) {
            tmp[1] = current;
        }
        else {
            tmp[0] = current;
        }
    }

    function testOneNan(typedArray, backup, isDouble) {
        setBit(typedArray, 12, 1, isDouble)
        backup[0] = typedArray[0];
        assert_equal(isNaN(backup[0]), true);
        assert_equal(isNaN(typedArray[0]), true);
    }
    testOneNan(floatArray, floatArray2, false);
}

{
    let v0 = new ArrayBuffer(8);
    let v1 = new Int32Array(v0);
    v1[0] = 0xcafe0000;
    v1[1] = 0xffff0000;
    let v2 = new Float32Array(v0);
    Array.prototype.push.apply(v0, v2);
    assert_equal(Number.isNaN(v2[0]), false);
}

test_end();