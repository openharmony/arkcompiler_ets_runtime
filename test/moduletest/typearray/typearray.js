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

typedArrayConstructors.forEach(function(ctor, i) {
    if (testTypeArray1(ctor) && testTypeArray2(ctor) &&
        testTypeArray3(ctor) && testTypeArray4(ctor) &&
        testTypeArrayWithSize(ctor, 10) && testTypeArrayWithSize(ctor, 50) &&
        testTypeArrayIC(ctor)) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArray5(ctor) ) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

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
print(a3.length);

const a4 = new Uint32Array(1024);
const obj1 = {
    "c": a4,
    __proto__: a4
}
obj1[235] = 1024;
print(obj1[235]);

try {
    const a5 = new Uint8ClampedArray(new ArrayBuffer(1283053413), 9007199254740991);
    a5.copyWithin(-13602);
} catch(e) {
    print("test successful !!!");
}

try {
    const a6 = new BigInt64Array(10);
    Int16Array.apply(null, a6);
} catch(e) {
    print("test successful !!!");
}

const a7 = new BigInt64Array(4);
function foo() {}
const f = new foo();
const protoOf = f.isPrototypeOf;
print(protoOf.apply(protoOf, a7));
const uint8 = new Uint8Array([1, 2, 3]);
const reversedUint8 = uint8.toReversed();
print(reversedUint8); // Uint8Array [3, 2, 1]
print(uint8); // Uint8Array [1, 2, 3]

try {
    const a8 = new Int8Array(new ArrayBuffer(0x40004141, {"maxByteLength": 0x40004141}));
    const a9 = new Float64Array(a8);
} catch (e) {
    print("test successful !!!");
}

try {
    const a10 = [1, 2];
    const a11 = new Uint8Array(a10);
    const a12 = new Uint32Array(a11);
    a12.set(a10, 0x1ffffffff);
} catch (e) {
    print("test successful !!!");
}

try {
    const v17 = new Int16Array(5);
    const v20 = new Int16Array(5);
    v17.set(v20, 4294967295);
} catch (error) {
    print(error)
}

try {
    const v17 = new Int16Array(5);
    const v20 = new Int16Array(5);
    v17.set(v20, 4294967296);
} catch (error) {
    print(error)
}

try {
    new BigUint64Array(new ArrayBuffer(256), 256, 0x1fffffff)
} catch (error) {
    print(error)
}

let arr12 = new Uint8Array(256).fill(255);
print(arr12[0] == 255);
print(arr12[123] == 255);
print(arr12[255] == 255);

try {
    new Uint8Array(2 ** 32 - 1);
} catch (error) {
    print(error.name);
}

const v21 = new SharedArrayBuffer(32);
const v22 = new BigInt64Array(v21);
Atomics.or(v22, Int16Array, false);
print(v22);
print(Atomics.wait(v22, false, true));

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
print(arr13.length == arr14.length)


const v2 = ("4294967297")["replaceAll"]();
const v4 = new Float32Array();
const t11 = v4.__proto__;
t11[v2] = v2;
print(t11[v2]);

const v3 = String.fromCharCode(564654156456456465465)
const v5 = new Int16Array(true);
print(v5["join"](v3));

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
print(arr.filter(TestTypedArrayFilterFunc));
print(calls);

Object.defineProperty(Int8Array.prototype, "constructor", {
    get: function () {
        calls++;
    }
});

TestTypedArrayFilter(Int8Array);
print(arr.filter(TestTypedArrayFilterFunc));
print(calls);

// test case for some()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

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
print(arr1_every.every(testEvery_true));
print(arr1_every.every(testEvery_false));

let arr2_every = new Int16Array();
print(arr2_every.every(testEvery_false));
let arr3_every = [1, 2, 3, 4, 5, 6, 7, 8, 9];
print(arr3_every.every(testEvery_true));

// Test case for reduce()
let arr1_reduce = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
function test_Reduce(a, b){
    return a+b;
}
print(arr1_reduce.reduce(test_Reduce));
print(arr1_reduce.reduce(test_Reduce, 10));
let arr2_reduce = [1, 2, 3, 4, 5, 6, 7, 8, 9];
print(arr2_reduce.reduce(test_Reduce));

// Test case for reduceRight()
let arr1_reduceRight = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
function test_ReduceRight(a, b){
    return a+b;
}
print(arr1_reduceRight.reduceRight(test_ReduceRight));
print(arr1_reduceRight.reduceRight(test_ReduceRight, 10));
let arr2_reduceRight = [1, 2, 3, 4, 5, 6, 7, 8, 9];
print(arr2_reduceRight.reduceRight(test_ReduceRight));

// Test case for copyWithin()
let arr1_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
print(arr1_copyWithin.copyWithin(-10, 1, 100));
let arr2_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
print(arr2_copyWithin.copyWithin(-3, -100, -1));
let arr3_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
print(arr3_copyWithin.copyWithin(4, 1, 10));
let arr4_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
print(arr4_copyWithin.copyWithin(4, -2));
let arr5_copyWithin = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
print(arr5_copyWithin.copyWithin(4));
let arr6_copyWithin = [1, 2, 3, 4, 5, 6, 7, 8, 9];
print(arr6_copyWithin.copyWithin(1));

// Test case for findIndex()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

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
print(arr1_includes.includes(5, -100));
print(arr1_includes.includes(55,-1));
let arr2_includes = [1, 2, 3, 4, 5, 6, 7, 8, 9];
print(arr2_includes.includes(5));

// Test case for find()
let arr1_find = new Int16Array([1, 2, 3, 4, 5, 6, 7, 8, 9]);
function testFind_true(ele) {
    return ele === 5;
}
function testFind_false(ele) {
    return ele > 10;
}
print(arr1_find.find(testFind_true));
print(arr1_find.find(testFind_false));

let arr2_find = new Int16Array();
print(arr2_find.find(testFind_false));
let arr3_find = [1, 2, 3, 4, 5, 6, 7, 8, 9];
print(arr3_find.find(testFind_true));

// Test case for indexOf()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

function testTypeArrayIndexOf(ctor) {
    let obj = new ctor([5, 10, 20, 30, 40, 50, 60])
    let result = obj.indexOf(60);
    return result != -1;
}

// Test case for lastIndexOf()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

function testTypeArrayLastIndexOf(ctor) {
    let obj = new ctor([5, 10, 20, 30, 40, 50, 60]);
    let result = obj.lastIndexOf(5)
    return result != -1;
}

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArrayLastIndexOf1(ctor) ) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

function testTypeArrayLastIndexOf1(ctor) {
    let obj = new ctor([5n, 10n, 20n, 30n, 40n, 50n, 60n]);
    let result = obj.lastIndexOf(5n)
    return result != -1;
}

let lastIndexOfFailed = new Int16Array(5, 10, 20, 30, 40, 50, 60)
let lastIndexOfFailedResult = lastIndexOfFailed.lastIndexOf('ABC')
print(lastIndexOfFailedResult);

// Test case for reverse()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

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

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArrayReverse2(ctor) ) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

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
    print(accumulator, currentvalue);
    return accumulator + currentvalue;
}
let arr1 = new Uint32Array();
let arr2 = new Uint32Array(3);
for (let i = 0; i < 3; i++) {
    arr2[i] = i + 1;
}
let res1 = arr1.reduceRight(fun1, 1, 1);
print(res1);
let res2 = arr2.reduceRight(fun1, 1, 1);
print(res2);
let res3 = arr1.reduceRight(fun1, 1);
print(res3);
let res4 = arr2.reduceRight(fun1, 1);
print(res4);
try {
    let res5 = arr1.reduceRight(fun1);
    print(res5);
} catch (e) {
    print(e.name);
}
let res6 = arr2.reduceRight(fun1);
print(res6);
let res7 = arr1.reduceRight(fun1, undefined);
print(res3);
let res8 = arr2.reduceRight(fun1, undefined);
print(res4);
let res9 = arr1.reduceRight(fun1, null);
print(res3);
let res10 = arr2.reduceRight(fun1, null);
print(res4);

// Test case for findLastIndex()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

function func(element, Last, array) {
    return element >= 0;
}

function testTypeArrayfindLastIndex(ctor) {
    let obj = new ctor([5, 10, 20, 30, 40, 50, 60])
    let result = obj.findLastIndex(func);
    return result != -1;
}

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArrayfindLastIndex1(ctor) ) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

function testTypeArrayfindLastIndex1(ctor) {
    let obj = new ctor([5n, 10n, 20n, 30n, 40n, 50n, 60n])
    let result = obj.findLastIndex(func);
    return result != -1;
}

// Test case for of()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

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

function typedArraysEqual(typedArr1, typedArr2) {
    if (typedArr1.length !== typedArr2.length) return false;
    for (let i = 0; i < typedArr1.length; i++) {
      if (typedArr1[i] !== typedArr2[i]) return false;
    }
    return true;
}

// typedArray.map()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});
  
function testTypeArray1(ctor) {
    let obj = new ctor([2]);
    let result = obj.map(function (num) {
        return num * 2;
    });
    return result[0] == 4;
}

// test case for toReversed()
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
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

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

[
    BigInt64Array,
    BigUint64Array
].forEach(function(ctor, i) {
    if (testTypeArrayToReversed2(ctor) ) {
        print(ctor.name + " test success !!!")
    } else {
        print(ctor.name + " test fail !!!")
    }
});

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