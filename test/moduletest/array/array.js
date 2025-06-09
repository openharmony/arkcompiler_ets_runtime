/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
 * @tc.name:array
 * @tc.desc:test Array
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var arr = new Array(100);
var a = arr.slice(90, 100);
assert_equal(a.length, 10);

var arr1 = [1, 1, 1, 1, 1, 1];
arr1.fill(0, 2, 4);
assert_equal(arr1, [1,1,0,0,1,1]);

var arr2 = new Array(100);
arr2.fill(0, 2, 4);
var a1 = arr2.slice(0, 5);
assert_equal(arr2.length, 100);
assert_equal(a1, [,,0,0,,]);

var arr3 = [1, 2, 3, 4, 5, 6];
arr3.pop();
assert_equal(arr3.length, 5);
assert_equal(arr3, [1,2,3,4,5]);

var arr4 = [1, 3, 4];
arr4.splice(1, 0, 2);
assert_equal(arr4.length, 4);
assert_equal(arr4, [1,2,3,4]);
// 1, 2, 3, 4
var arr4 = [1, 2, 3, 4];
arr4.splice(3, 1, 3);
assert_equal(arr4.length, 4);
assert_equal(arr4, [1,2,3,3]);
// 1, 2, 3, 3

var arr5 = [1, 2, 3, 4, 5, 6];
arr5.shift();
assert_equal(arr5.length, 5);
assert_equal(arr5, [2,3,4,5,6]);
// 1, 2, 3, 4, 5

var arr6 = [1, 2, 3, 4, 5];
arr6.reverse();
assert_equal(arr6, [5,4,3,2,1]);

var arr7 = [];
arr7[2] = 10;
assert_equal(arr7.pop(), 10); // 10
assert_equal(arr7.pop(), undefined); // undefined
assert_equal(arr7.pop(), undefined); // undefined

var arr8 = [];
arr8[1] = 8;
assert_equal(arr8.shift(), undefined); // undefined
assert_equal(arr8.shift(), 8); // 8
assert_equal(arr8.shift(), undefined); // undefined

// Test on Array::Splice
var arr9 = new Array(9);
assert_equal(arr9.length, 9); // 9
arr9.splice(0, 9);
assert_equal(arr9.length, 0); // 0

var arr10 = new Array(9);
assert_equal(arr10.length, 9); // 9
arr10.splice(0, 8, 1);
assert_equal(arr10.length, 2); // 2
assert_equal(arr10[0], 1);
assert_equal(arr10[1], undefined);

var arr11 = new Array(9);
assert_equal(arr11.length, 9); // 9
arr11.splice(1, 8, 1);
assert_equal(arr11.length, 2); // 2
assert_equal(arr11[0], undefined);
assert_equal(arr11[1], 1);

var arr12 = new Array(9);
assert_equal(arr12.length, 9); // 9
arr12.splice(0, 4, 1, 2, 3, 4, 5);
assert_equal(arr12.length, 10); // 10
for (let i = 0; i < arr12.length; i++) {
    let v = arr12[i];
    if (i < 5) {
        assert_equal(v, i + 1);
    } else {
        assert_equal(v, undefined);
    }
}

var arr13 = new Array(9);
assert_equal(arr13.length, 9); // 9
arr13.splice(7, 5, 1, 2, 3);
assert_equal(arr13.length, 10); // 10
for (var i = 0; i < arr13.length; i++) {
    let v = arr13[i];
    if (i < 7) {
        assert_equal(v, undefined)
    } else {
        assert_equal(v, i - 6);
    }
}

var arr14 = Array.apply(null, Array(16));
assert_equal(arr14.length, 16);
var arr15 = Array.apply(null, [1, 2, 3, 4, 5, 6]);
assert_equal(arr15.length, 6);

var a = '0';
assert_equal(Array(5).indexOf(a), -1);

let throwException = false;
const v3 = new Float32Array(7);
try {
    v3.filter(Float32Array)
} catch (error) {
    throwException = true;
}
assert_true(throwException)

const v20 = new Array(2);
let v21;
try {
    v21 = v20.pop();
    assert_equal(v21, undefined);
} catch (error) {

}

var arr21 = [1, 2, 3, 4, , 6];
assert_equal(arr21.at(0), 1);
assert_equal(arr21.at(5), 6);
assert_equal(arr21.at(-1), 6);
assert_equal(arr21.at(6), undefined);
assert_equal(arr21.at('1.9'), 2);
assert_equal(arr21.at(true), 2);
var arr22 = arr21.toReversed();
assert_equal(arr22, [6,,4,3,2, 1])
assert_equal(arr21, [1,2,3,4,,6])
var arr23 = [1, 2, 3, 4, 5];
assert_equal(arr23.with(2, 6), [1,2,6,4,5]); // [1, 2, 6, 4, 5]
assert_equal(arr23, [1,2,3,4,5]); // [1, 2, 3, 4, 5]

const months = ["Mar", "Jan", "Feb", "Dec"];
const sortedMonths = months.toSorted();
assert_equal(sortedMonths, ['Dec', 'Feb', 'Jan', 'Mar']); // ['Dec', 'Feb', 'Jan', 'Mar']
assert_equal(months, ['Mar', 'Jan', 'Feb', 'Dec']); // ['Mar', 'Jan', 'Feb', 'Dec']

const values = [1, 10, 21, 2];
const sortedValues = values.toSorted((a, b) => a - b);
assert_equal(sortedValues, [1, 2, 10, 21]); // [1, 2, 10, 21]
assert_equal(values, [1, 10, 21, 2]); // [1, 10, 21, 2]

const arrs = new Array(6);
for (let i = 0; i < 6; i++) {
    var str = i.toString();
    if (i != 1) {
        arrs[i] = str;
    }
}
arrs.reverse();
assert_equal(arrs, ['5', '4', '3', '2', undefined, '0']); // 5,4,3,2,,0


function handleUnexpectedErrorCaught(prompt, e) {
    if (e instanceof Error) {
        print(`Unexpected ${e.name} caught in ${prompt}: ${e.message}`);
        if (typeof e.stack !== 'undefined') {
            print(`Stacktrace:\n${e.stack}`);
        }
    } else {
    }
}

// Test cases for reverse()
try {
    const arr0 = [];
    assert_equal(arr0.reverse(), arr0); // true
    assert_equal(`${arr0.length}`, 0); // 0

    const arr1 = [1];
    assert_equal(arr1.reverse(), arr1); // true
    assert_equal(`${arr1}`, 1); // 1

    const arrWithHoles = [];
    arrWithHoles[1] = 1;
    arrWithHoles[4] = 4;
    arrWithHoles[6] = undefined;
    arrWithHoles.length = 10;
    // arrWithHoles = [Hole, 1, Hole, Hole, 4, Hole, undefined, Hole, Hole, Hole]
    assert_equal(`${arrWithHoles}`, [,1,,,4,,,,,]); // ,1,,,4,,,,,
    assert_equal(arrWithHoles.reverse(), [,,,,,4,,,1,]); // ,,,,,4,,,1,
    assert_equal(`${arrWithHoles}`, [,,,,,4,,,1,]); // ,,,,,4,,,1,
} catch (e) {
    handleUnexpectedErrorCaught(e);
}

// Test case for indexOf() and lastIndexOf()
try {
    const arr = [0, 10, 20];
    arr.length = 10;
    arr[3] = 80;
    arr[4] = 40;
    arr[6] = undefined;
    arr[7] = 10;
    arr[8] = "80";
    // prompt1, results1, prompt2, results2, ...
    const resultGroups = [
        "Group 1: 0 <= fromIndex < arr.length", [
    arr.indexOf(40), // 4
    arr.indexOf(40, 5), // -1
    arr.indexOf(10), // 1
    arr.indexOf(10, 2), // 7
    arr.lastIndexOf(40), // 4
    arr.lastIndexOf(40, 3), // -1
    arr.lastIndexOf(10), // 7
    arr.lastIndexOf(10, 6), // 1
    ],
        "Group 2: -arr.length <= fromIndex < 0", [
    arr.indexOf(40, -4), // -1
    arr.indexOf(40, -8), // 4
    arr.indexOf(10, -4), // 7
    arr.indexOf(10, -10), // 1
    arr.lastIndexOf(40, -4), // 4
    arr.lastIndexOf(40, -8), // -1
    arr.lastIndexOf(10, -4), // 1
    arr.lastIndexOf(10, -10), // -1
    arr.indexOf(0, -arr.length), // 0
    arr.indexOf(10, -arr.length), // 1
    arr.lastIndexOf(0, -arr.length), // 0
    arr.lastIndexOf(10, -arr.length), // -1
    ],
        "Group 3: fromIndex >= arr.length", [
    arr.indexOf(0, arr.length), // -1
    arr.indexOf(0, arr.length + 10), // -1
    arr.indexOf(10, arr.length),
    arr.indexOf(10, arr.length + 10),
    arr.lastIndexOf(0, arr.length), // 0
    arr.lastIndexOf(0, arr.length + 10), // 0
    ],
        "Group 4: fromIndex < -arr.length", [
    arr.indexOf(0, -arr.length - 10), // 0
    arr.lastIndexOf(0, -arr.length - 10) // -1
    ],
        "Group 5: fromIndex in [Infinity, -Infinity]", [
    arr.indexOf(10, -Infinity), // 1
    arr.indexOf(10, +Infinity), // -1
    arr.lastIndexOf(10, -Infinity), // -1
    arr.lastIndexOf(10, +Infinity) // 7
    ],
        "Group 6: fromIndex is NaN", [
    arr.indexOf(0, NaN), // 0
    arr.indexOf(10, NaN), // 1
    arr.lastIndexOf(0, NaN), // 0
    arr.lastIndexOf(10, NaN), // -1
    ],
        "Group 7: fromIndex is not of type 'number'", [
    arr.indexOf(10, '2'), // 7
    arr.lastIndexOf(10, '2'), // 1
    arr.indexOf(10, { valueOf() {
        return 3;
    } }), // 7
    arr.indexOf(10, { valueOf() {
        return 3;
    } }), // 1
    ],
        "Group 8: Strict equality checking", [
    arr.indexOf("80"), // 8
    arr.lastIndexOf(80), // 3
    ],
        "Group 9: Searching undefined and null", [
    arr.indexOf(), // 6
    arr.indexOf(undefined), // 6
    arr.indexOf(null), // -1
    arr.lastIndexOf(), // 6
    arr.lastIndexOf(undefined), // 6
    arr.lastIndexOf(null) // -1
    ]
    ];

    assert_equal(resultGroups[0], [4,-1,1,7,4,-1,7,1]);
    assert_equal(resultGroups[1], [-1,4,7,1,4,-1,1,-1,0,1,0,-1]);
    assert_equal(resultGroups[2], [-1,-1,-1,-1,0,0]);
    assert_equal(resultGroups[3], [0,-1]);
    assert_equal(resultGroups[4], [ 1,-1,-1,7]);
    assert_equal(resultGroups[5], [0,1,0,-1]);
    assert_equal(resultGroups[6], [7,1,7,7]);
    assert_equal(resultGroups[7], [8, 3]);
    assert_equal(resultGroups[8], [6,6,-1,6,6,-1]);

    print("Group 10: fromIndex with side effects:");
    let accessCount = 0;
    const arrProxyHandler = {
        has(target, key) {
            accessCount += 1;
            return key in target;
        }
    };
    // Details on why accessCount = 6 can be seen in ECMAScript specifications:
    // https://tc39.es/ecma262/multipage/indexed-collections.html#sec-array.prototype.indexof
    accessCount = 0;
    const arr2 = new Proxy([10, 20, 30, 40, 50, 60], arrProxyHandler);
    const result2 = arr2.indexOf(30, {
        valueOf() {
            arr2.length = 2; // Side effects to arr2
            return 0;
        }
    }); // Expected: -1 (with accessCount = 6)
    assert_equal(`${result2}`, -1);
    assert_equal(`${accessCount}`, 6);
    // Details on why accessCount = 6 can be seen in ECMAScript specifications:
    // https://tc39.es/ecma262/multipage/indexed-collections.html#sec-array.prototype.lastindexof
    accessCount = 0;
    const arr3 = new Proxy([15, 25, 35, 45, 55, 65], arrProxyHandler);
    const result3 = arr3.lastIndexOf(45, {
        valueOf() {
            arr3.length = 2; // Side effects to arr3
            return 5;
        }
    }); // Expected: -1 (with accessCount = 6)
    assert_equal(`${result3}`, -1);
    assert_equal(`${accessCount}`, 6);

    let indexOfErrorCount = 0;
    let lastIndexOfErrorCount = 0;
    for (let [prompt, fromIndex] of [["bigint", 1n], ["symbol", Symbol()]]) {
        for (let method of
                [Array.prototype.indexOf, Array.prototype.lastIndexOf]) {
            try {
                const result = method.call(arr, 10, fromIndex);
                print(`ERROR: Unexpected result (which is ${result}) returned by method '${method.name}': ` +
                `Expects a TypeError thrown for fromIndex type '${prompt}'.`);
            } catch (e) {
                if (method.name == "indexOf") {
                    indexOfErrorCount++;
                }
                if (method.name == "lastIndexOf") {
                    lastIndexOfErrorCount++;
                }
            }
        }
    }
    assert_equal(indexOfErrorCount, 2);
    assert_equal(lastIndexOfErrorCount, 2);
} catch (e) {
    handleUnexpectedErrorCaught(e);
}

// Test Array.prototype.filter when callbackFn is not callable
let callError = false;
try {
    [].filter(1);
} catch (e) {
    callError = true;
}
assert_true(callError);

var getTrap = function (t, key) {
    if (key === "length") return { [Symbol.toPrimitive]() {
        return 3
    } };
    if (key === "2") return "baz";
    if (key === "3") return "bar";
};
var target1 = [];
var obj = new Proxy(target1, { get: getTrap, has: () => true });
assert_equal([].concat(obj), [,,"baz"]);
assert_equal(Array.prototype.concat.apply(obj), [,,"baz"]);

const v55 = new Float64Array();
const v98 = [-5.335880620598348e+307, 1.0, 1.0, 2.220446049250313e-16, -1.6304390272817058e+308];

function f99(a100) {
}

function f110() {
    v98[2467] = v55;
}

Object.defineProperty(f99, Symbol.species, { configurable: true, enumerable: true, value: f110 });
v98.constructor = f99;
let v98Result = JSON.stringify(v98.splice(4));
assert_equal(v98Result, "{\"0\":-1.6304390272817058e+308,\"length\":1}");

var count;
var params;

class MyObserveArrray extends Array {
    constructor(...args) {
        super(...args);
        params = args;
    }

    static get [Symbol.species]() {
        count++;
        return this;
    }
}

count = 0;
params = undefined;
new MyObserveArrray().filter(() => {
});
assert_equal(count, 1);
assert_equal(params, [0]);

count = 0;
params = undefined;
new MyObserveArrray().concat(() => {
});
assert_equal(count, 1);
assert_equal(params, [0]);

count = 0;
params = undefined;
new MyObserveArrray().slice(() => {
});
assert_equal(count, 1);
assert_equal(params, [0]);

count = 0;
params = undefined;
new MyObserveArrray().splice(() => {
});
assert_equal(count, 1);
assert_equal(params, [0]);

new MyObserveArrray([1, 2, 3, 4]).with(0, 0);
new MyObserveArrray([1, 2, 3, 4]).toReversed(0, 0);
new MyObserveArrray([1, 2, 3, 4]).toSpliced(0, 0, 0);

arr = new Array(1026);
arr.fill(100);
assert_equal(arr.with(0, "n")[0], 'n')

arr = new Array(1026);
arr.fill(100);
assert_equal(arr.toReversed()[0], 100)

arr = new Array(1026);
arr.fill(100);
assert_equal(arr.toSpliced(0, 0, 0, 0)[0], 0)

var arr25 = []
arr25[1025] = 0;
assert_equal(arr25.includes({}, 414), false);
assert_equal(arr25.includes(1025,109), false);

var arr26 = []
arr25[100] = 0;
assert_equal(arr25.includes({}, 26), false);

function fun1(obj, name, type) {
    return typeof type === 'undefined' || typeof desc.value === type;
}

function fun2(obj, type) {
    let properties = [];
    let proto = Object.getPrototypeOf(obj);
    while (proto && proto != Object.prototype) {
        Object.getOwnPropertyNames(proto).forEach(name => {
            if (name !== 'constructor') {
                if (fun1(proto, name, type)) properties.push(name);
            }
        });
        proto = Object.getPrototypeOf(proto);
    }
    return properties;
}

function fun4(seed) {
    let objects = [Object, Error, AggregateError, EvalError, RangeError, ReferenceError, SyntaxError, TypeError, URIError, String, BigInt, Function, Number, Boolean, Date, RegExp, Array, ArrayBuffer, DataView, Int8Array, Int16Array, Int32Array, Uint8Array, Uint8ClampedArray, Uint16Array, Uint32Array, Float32Array, Float64Array, BigInt64Array, BigUint64Array, Set, Map, WeakMap, WeakSet, Symbol, Proxy];
    return objects[seed % objects.length];
}

function fun8(obj, seed) {
    let properties = fun2(obj);
}

fun4(694532)[fun8(fun4(694532), 527224)];
Object.freeze(Object.prototype);

Array.prototype.length = 3000;
assert_equal(Array.prototype.length, 3000)

let unscopables1 = Array.prototype[Symbol.unscopables];
let unscopables2 = Array.prototype[Symbol.unscopables];
assert_true(unscopables1 == unscopables2)

arr = []
let index = 4294967291;
arr[index] = 0;
arr.length = 10;
arr.fill(10);
assert_equal(arr, [10,10,10,10,10,10,10,10,10,10]);

// Test case for copyWithin()
var arr_copywithin1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr_copywithin2 = new Array();
for (let i = 0; i < 10; i++) arr_copywithin2[i] = i;
var arr_copywithin3 = new Array();
for (let i = 0; i < 10; i++) {
    if (i == 2) {
        continue;
    }
    arr_copywithin3[i] = i;
}
var arr_copywithin4 = new Array();
for (let i = 0; i < 10; i++) arr_copywithin4[i] = i;
var result_copywithin1 = arr_copywithin1.copyWithin(0, 3, 7);
assert_equal(result_copywithin1, [3,4,5,6,4,5,6,7,8,9]);
var result_copywithin2 = arr_copywithin2.copyWithin(0, 3, 7);
assert_equal(result_copywithin2, [3,4,5,6,4,5,6,7,8,9]);
var arr_copywithin3 = arr_copywithin3.copyWithin(0, 0, 10);
assert_equal(arr_copywithin3, [0,1,,3,4,5,6,7,8,9]);
//elementskind is generic but hclass == generic hclass
var arr_copywithin4 = arr_copywithin4.copyWithin(3, 0, 6);
assert_equal(arr_copywithin4, [0,1,2,0,1,2,3,4,5,9]);

const ArraySize = 10;
const QuarterSize = ArraySize / 4;
let result;
let arr_copywithin5 = [];
let arr_copywithin6 = [];
arr_copywithin6.proto = arr_copywithin4;
for (let i = 0; i < ArraySize; ++i) arr_copywithin5[i] = i;
for (let i = 0; i < ArraySize; ++i) arr_copywithin6[i] = i;

for (let i = 0; i < ArraySize; i++) {
    //elementskind is not generic
    result = arr_copywithin5.copyWithin(0, QuarterSize * 2, QuarterSize * 3);
}
assert_equal(result, [ 5,6,2,3,4,5,6,7,8,9]);

for (let i = 0; i < ArraySize; i++) {
    //elementskind is generic but hclass != generic hclass
    result = arr_copywithin6.copyWithin(0, QuarterSize * 2, QuarterSize * 3);
}
assert_equal(result, [5,6,2,3,4,5,6,7,8,9]);

// Test case for every()
var arr_every1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr_every2 = new Array();

function testEvery(element, index, array) {
    if (index == 0) {
        array.length = 6;
    }
    return element < 6;
}

function testEvery4(element, index, array) {
    array.pop();
    array.pop();
    array.pop();
    return element < 6;
}

for (let i = 0; i < 10; i++) arr_every2[i] = i;
var arr_every3 = new Array();
for (let i = 0; i < 10; i++) {
    if (i == 2) {
        continue;
    }
    arr_every3[i] = i;
}
var arr_every4 = new Array();
for (let i = 0; i < 10; i++) arr_every4[i] = i;
var result_every1 = arr_every1.every(testEvery);
assert_true(result_every1);
var result_every2 = arr_every2.every(testEvery);
assert_true(result_every2);
var result_every3 = arr_every3.every(testEvery);
assert_true(result_every3);
var result_every4 = arr_every4.every(testEvery4);
assert_true(result_every4);

// Test case for reduceRight()
var arr_reduceRight1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr_reduceRight2 = new Array();

function testReduceRight(accumulator, element, index, array) {
    if (index == 0) {
        array.length = 6;
    }
    return accumulator + element;
}

function testReduceRight4(accumulator, element, index, array) {
    array.pop();
    array.pop();
    return accumulator + element;
}

for (let i = 0; i < 10; i++) arr_reduceRight2[i] = i;
var arr_reduceRight3 = new Array();
for (let i = 0; i < 10; i++) {
    if (i < 9) {
        continue;
    }
    arr_reduceRight3[i] = i;
}
var arr_reduceRight4 = new Array();
for (let i = 0; i < 10; i++) arr_reduceRight4[i] = i;
var result_reduceRight1 = arr_reduceRight1.reduceRight(testReduceRight, 100);
assert_equal(result_reduceRight1, 145);
var result_reduceRight2 = arr_reduceRight2.reduceRight(testReduceRight, 100);
assert_equal(result_reduceRight2, 145);
var result_reduceRight3 = arr_reduceRight3.reduceRight(testReduceRight, 100);
assert_equal(result_reduceRight3, 109);
var result_reduceRight4 = arr_reduceRight4.reduceRight(testReduceRight4, 100);
assert_equal(result_reduceRight4, 125);

// Test case for some()
var arr_some1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr_some2 = new Array();

function testSome(element, index, array) {
    if (index == 0) {
        array.length = 6;
    }
    return element > 8;
}

for (let i = 0; i < 10; i++) arr_some2[i] = i;
var arr_some3 = new Array();
for (let i = 0; i < 10; i++) {
    if (i < 9) {
        continue;
    }
    arr_some3[i] = i;
}
var result_some1 = arr_some1.some(testSome);
assert_equal(result_some1, false);
var result_some2 = arr_some2.some(testSome);
assert_equal(result_some2, false);
var result_some3 = arr_some3.some(testSome);
assert_equal(result_some3, true);

// Test case for unshift()
var arr_unshift1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr_unshift2 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];

var result_unshift1 = arr_unshift1.unshift(1, 2, 3, 4, 5);
assert_equal(result_unshift1, 15);
var result_unshift2 = arr_unshift2.unshift(1, 2, 3);
assert_equal(result_unshift2, 13);

class C3 {
    constructor(a5) {
        try {
            a5(this, this, ..."895053461", ..."p");
        } catch (e) {
        }
    }

    a;

    valueOf(a9) {
        const v10 = a9?.[this];
        -718821.501160539 !== a9;
        const t6 = "895053461";
        t6[a9] = "895053461";
        return v10;
    }
}

const v12 = new C3("p");
new C3(v12);
new C3(C3);

// Test case for toSorted()
var arr_toSorted1 = ["Mar", "Jan", "Feb", "Dec"];
var arr_toSorted2 = new Array();
arr_toSorted2[0] = "Mar";
arr_toSorted2[1] = "Jan";
arr_toSorted2[2] = "Feb";
arr_toSorted2[3] = "Dec";
var result_toSorted1 = arr_toSorted1.toSorted();
assert_equal(result_toSorted1, ["Dec","Feb","Jan","Mar"]);
var result_toSorted2 = arr_toSorted2.toSorted();
assert_equal(result_toSorted2, ["Dec","Feb","Jan","Mar"]);

const v0 = [0, 1];
const mapEd = v0.map(() => v0["pop"]());
assert_equal(new Uint16Array(v0).length, 1);

let errorCount1 = 0;
try {
    const v2 = new ArrayBuffer(103);

    function F3(a5, a6) {
        if (!new.target) {
            throw 'must be called with new';
        }

        function f7(a8, a9, a10) {
            return v2;
        }

        const v13 = new BigUint64Array(35);
        const o14 = {
            ...v13,
        };
        Object.defineProperty(o14, 4, { set: f7 });
    }

    new F3();
    new F3();
    v2();
} catch (err) {
    errorCount1++;
}
assert_equal(errorCount1, 1);

try {
    const v3 = new ArrayBuffer(45);
    const v0 = new Boolean;

    function F4(a6, a7) {
        if (!new.target) {
            throw 'must be called with new';
        }

        function f8(a9, a10, a11) {
            return F4;
        }

        const v14 = new BigUint64Array(15);
        const o20 = {
            [v0](a16, a17, a18, a19) {
                return a6;
            },
            ...v14,
        };
        Object.defineProperty(o20, 4, { set: f8 });
    }

    new F4(v0, 101);
    new F4();
    v3.transfer();
} catch (err) {
    errorCount1++;
}
assert_equal(errorCount1, 2);

try {

    function F6(a8, a9, a10) {
        if (!new.target) {
            throw 'must be called with new';
        }
        const v14 = new Date(-10, 16);
        v14.toDateString();
    }

    const v16 = new F6(11);
    new F6(2064);

    function f18() {
        return v16;
    }

    new BigUint64Array();
    const v23 = [-42, 4, 4];
    const o24 = {};
    const v25 = [5.0, -5.12, 1e-15, -0.05, 5.0, 2.22e-38, -10.0, 10.0, 5.0, -5.0];

    function f26() {
        return v25;
    }

    class C28 extends Date {
        constructor(a30, a31) {
            super();
        }
    }

    new Int8Array();
    new Int8Array();
    v23.filter(Array);
    new ArrayBuffer(26);
    const v44 = new Uint8Array();
    v44.toString();
    const o46 = {};
    for (let i49 = -51, i50 = 10; i49 < i50; i50--) {
        o46[i50] >>= i50;
    }
    /c/iusg;
    const v58 = new Int16Array();
    v58.forEach();
} catch (err) {
    errorCount1++;
}
assert_equal(errorCount1, 3);

try {
    const v3 = new ArrayBuffer(19);

    function F4(a6, a7) {
        if (!new.target) {
            throw 'must be called with new';
        }
        const v10 = new BigUint64Array(34);
        const o11 = {
            ...v10,
        };
        Object.defineProperty(o11, 4, { set: a6 });
    }

    const v12 = new F4();
    new F4(v12, v3);
} catch (err) {
    errorCount1++;
}
assert_equal(errorCount1, 4);

try {
    const v3 = new ArrayBuffer(10);

    function F4(a6, a7) {
        if (!new.target) {
            throw 'must be called with new';
        }

        function f8(a9, a10, a11) {
            return a11;
        }

        const v14 = new BigUint64Array(34);
        const o15 = {
            ...v14,
        };
        Object.defineProperty(o15, 4, { set: f8 });
    }

    const v16 = new F4();
    new F4(v16, v3);
    Int32Array();
} catch (err) {
    errorCount1++;
}
assert_equal(errorCount1, 5);

try {
    const v2 = new ArrayBuffer(10);

    function F3(a5, a6) {
        if (!new.target) {
            throw 'must be called with new';
        }

        function f7(a8, a9, a10) {
            return a9;
        }

        const v13 = new BigUint64Array(35);
        const o14 = {
            ...v13,
        };
        Object.defineProperty(o14, 4, { set: f7 });
    }

    new F3(ArrayBuffer, v2);
    new F3(10, F3);
    v2.transfer();
} catch (err) {
    errorCount1++;
}
assert_equal(errorCount1, 6);


try {
    const v1 = new Uint16Array();
    for (let i4 = 0, i5 = 10; i5; i5--) {
    }
    undefined instanceof v1.values();
} catch (error) {
    errorCount1++;
}
assert_equal(errorCount1, 7);

/*
 * @tc.name:ArrayConstructor
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const originalArrays = [
    [1, 2, 3],
    ["apple", "banana", "orange"],
    [true, false, true],
    [{ name: "John" }, { name: "Doe" }],
    [NaN, NaN, NaN],
    [Infinity, -Infinity],
    [RegExp("pattern1"), RegExp("pattern2")],
    [new Map(), new Map()],
    [new Set(), new Set()],
    [Array.from([1, 2, 3]), Array.from([4, 5, 6])],
    ["ark_unicodeValue 😀", "ark_unicodeValue 😎"],
];

assert_equal(originalArrays[0], [1, 2, 3]);
assert_equal(originalArrays[1], ["apple", "banana", "orange"]);
assert_equal(originalArrays[2], [true, false, true]);
assert_equal(originalArrays[5], [Infinity, -Infinity]);

errorCount1 = 0;
try {
    const arr1 = [1, 2, 3];
    assert_equal(arr1[10], undefined);
} catch (error) {
    errorCount1++;
}
assert_equal(errorCount1, 0);

try {   
} catch (error) {
    print("Caught an error: ", error);
}

let errorStr = "";
try {
    const arr2 = new Array(-1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, "RangeError: Invalid array length");

/*
 * @tc.name:from
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const newArray = Array.from(originalArrays);
newArray.forEach((array, index) => {
    assert_equal(array, originalArrays[index])
});

try {
    Array.from([1, 2, 3], "not a function");
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, "TypeError: the mapfn is not callable.");

try {
    Array.from([1, 2, 3], () => { throw new Error("Something went wrong"); });
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, "Error: Something went wrong");

try {
    Array.from(123);
    Array.from({});
    Array.from([1, 2, 3], () => {}, 123);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, "Error: Something went wrong");

/*
 * @tc.name:isArray
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
originalArrays.forEach((array, index) => {
    assert_true(Array.isArray(array));
});

try {
    assert_true(!Array.isArray());
    assert_true(!Array.isArray(123));
    assert_true(!Array.isArray("not an array"));
    assert_true(!Array.isArray(null));
    assert_true(!Array.isArray(undefined));
} catch (error) {
    errorStr = error.toString();
}

/*
 * @tc.name:Of
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const ArrayOf = Array.of(...originalArrays);
assert_equal(ArrayOf[1], ["apple", "banana", "orange"]);

try {
    const arr1 = Array.of(undefined);
    const arr2 = Array.of(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    assert_equal(arr1, [undefined]);
    assert_equal(arr2, [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]);
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:At
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
assert_equal(originalArrays.at(1), ['apple', 'banana', 'orange']);
assert_equal(originalArrays.at(-1), ["ark_unicodeValue 😀", "ark_unicodeValue 😎"]);
assert_equal(originalArrays.at(-2), [[1,2,3],[4,5,6]]);

try {
    assert_equal(originalArrays.at(), [1,2,3]);
    assert_equal(originalArrays.at(100), undefined);
    assert_equal(originalArrays.at(null), [1,2,3]);
    assert_equal(originalArrays.at(undefined), [1,2,3]);
    assert_equal(originalArrays.at(new Map()), [1,2,3]);
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:concat
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const array1 = [1, "two", true];
const e1 = { key: "value" };
const array2 = [null, undefined, e1];
const array3 = [];

const concatenatedArray = array1.concat(array2, array3);
assert_equal(concatenatedArray, [1, "two", true, null, undefined, e1]);

const nestedArray = [[1, 2], ["a", "b"], [true, false]];
const nestedConcatenatedArray = array1.concat(nestedArray, array2);
assert_equal(nestedConcatenatedArray,
             [1, "two", true, [1, 2], ["a", "b"], [true, false], null, undefined, e1]);

const e2 = { prop: "value" };
const mixedConcatenatedArray = array1.concat(4, "five", e2);
assert_equal(mixedConcatenatedArray, [1, "two", true, 4, "five", e2]);

const spreadConcatenatedArray = [...array1, ...array2, ...array3];
assert_equal(spreadConcatenatedArray, [1, "two", true, null, undefined, e1]);

/*
 * @tc.name:CopyWithin,Entries
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const copiedArray1 = originalArrays[0].slice().copyWithin(0, 2);
const copiedArray2 = originalArrays[1].slice().copyWithin(1, 0, 2);
const copiedArray3 = originalArrays[2].slice().copyWithin(1, -2);
const copiedArray4 = originalArrays[3].slice().copyWithin(-1);
const copiedArray5 = originalArrays[4].slice().copyWithin(0);

assert_equal(copiedArray1, [3, 2, 3]);
assert_equal(copiedArray2, ['apple', 'apple', 'banana']);
assert_equal(copiedArray3, [true, false, true]);
assert_equal(copiedArray4.toString(), '[object Object],[object Object]');
assert_equal(copiedArray5.toString(), 'NaN,NaN,NaN');

for (const [index, value] of originalArrays.entries()) {
    assert_equal(originalArrays[index], value);
}

/*
 * @tc.name:Every
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const numbers1 = [2, 4, 6, 8, 10];
const allEven = numbers1.every(num => num % 2 === 0);
assert_true(allEven);

const numbers2 = [1, 2, 3, 4, 5];
const allEven2 = numbers2.every(num => num % 2 === 0);
assert_true(!allEven2);

const emptyArray1 = [];
const allEmpty = emptyArray1.every(num => num % 2 === 0);
assert_true(allEmpty);

const emptyArray2 = [];
const allEmpty2 = emptyArray2.every(() => true);
assert_true(allEmpty2);

const mixedArray = [2, 4, "hello", 8, 10];
const allNumbers = mixedArray.every(num => typeof num === "number");
assert_true(!allNumbers);

const emptyArray3 = [];
const allNonNegative = emptyArray3.every(num => num >= 0);
assert_true(allNonNegative);

try {
    const arr = [1, 2, 3];
    const result = arr.every("not a function");
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, "TypeError: the callbackfun is not callable.")

try {
    const arr = [1, 2, 3];
    const result = arr.every(num => num < undefinedVariable);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, "ReferenceError: undefinedVariable is not defined");

/*
 * @tc.name:Fill
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array1 = [1, 2, 3, 4, 5];
    assert_equal(array1.fill(0), [0,0,0,0,0]);

    const array2 = [1, 2, 3, 4, 5];
    assert_equal(array2.fill(0, 2), [1,2,0,0,0]);

    const array3 = [1, 2, 3, 4, 5];
    assert_equal(array3.fill(0, 1, 3), [1,0,0,4,5]);

    const array4 = new Array(5);
    assert_equal(array4.fill(0), [0,0,0,0,0]);

    const array5 = Array.from({ length: 5 }, (_, index) => index + 1);
    assert_equal(array5.fill(0, 2), [1,2,0,0,0]);

    const array6 = Array.from({ length: 5 }, (_, index) => index + 1);
    assert_equal(array6.fill(0, 1, 3), [1,0,0,4,5]);

    const array7 = Array(5).fill("hello");
    assert_equal(array7, ['hello', 'hello', 'hello', 'hello', 'hello']);

    const array8 = [1, 2, 3];
    assert_equal(array8.fill(0, -2), [1,0,0]);

    const array9 = [1, 2, 3];
    assert_equal(array9.fill(0, -2, -1), [1,0,3]);
}

try {
    const arr = [1, 2, 3];
    arr.fill(0, 1.5);
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const arr = [1, 2, 3];
    arr.fill(0, NaN);
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:Filter
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const numbers = [1, 2, 3, 4, 5];

    const evenNumbers = numbers.filter(num => num % 2 === 0);
    assert_equal(evenNumbers, [2,4]);

    const greaterThanTwo = numbers.filter(num => num > 2);
    assert_equal(greaterThanTwo, [3,4,5]);

    const lessThanTen = numbers.filter(num => num < 10);
    assert_equal(lessThanTen, [1,2,3,4,5]);

    const divisibleByThree = numbers.filter(num => num % 3 === 0);
    assert_equal(divisibleByThree, [3]);

    const words = ["apple", "banana", "pear", "orange"];
    const longWords = words.filter(word => word.length >= 5);
    assert_equal(longWords, ['apple', 'banana', 'orange']);

    const persons = [
        { name: "Alice", age: 25 },
        { name: "Bob", age: 17 },
        { name: "Charlie", age: 30 }
    ];
    const adults = persons.filter(person => person.age > 18);
    assert_equal(adults.toString(), '[object Object],[object Object]');
}


try {
    const arr = [1, 2, 3];
    const result = arr.filter("not a function");
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the callbackfun is not callable.');

try {
    const obj = { a: 1, b: 2 };
    const result = obj.filter(() => true);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: undefined is not callable');

/*
 * @tc.name:Find
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [6, 7, 8, 9, 10, NaN, undefined, null, "", false, {name: "John"}, [1, 2, 3]];
    assert_equal(array.find(item => item === 5), undefined);
    assert_equal(array.find(item => item === 11), undefined);
    assert_equal(array.find(item => item > 5), 6);
    assert_equal(array.find(item => item < 0), undefined);
    assert_equal(array.find(item => typeof item === 'string'), "");
    assert_equal(array.find(item => typeof item === 'object'), null);
    assert_equal(array.find(item => Array.isArray(item)), [1,2,3]);
    assert_equal(array.find(item => item), 6);
    assert_equal(array.find(item => item === null), null);
    assert_equal(array.find(item => item === undefined), undefined);
    assert_equal(array.find(item => isNaN(item)).toString(), 'NaN');
    assert_equal(array.find(item => item === false), false);
}

try {
    const array = [1, 2, 3, 4, 5];
    array.find();
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the predicate is not callable.');

try {
    const array = [1, 2, 3, 4, 5];
    array.find("not a function");
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the predicate is not callable.');

try {
    let array = null;
    array.find(item => item === 1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: Cannot read property find of null');

try {
    array = undefined;
    array.find(item => item === 1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'ReferenceError: array is not defined');

/*
 * @tc.name:FindIndex
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [6, 7, 8, 9, 10, NaN, undefined, null, "", false, {name: "John"}, [1, 2, 3]];
    assert_equal(array.findIndex(item => item === 5), -1);
    assert_equal(array.findIndex(item => item === 11), -1);
    assert_equal(array.findIndex(item => item > 5), 0);
    assert_equal(array.findIndex(item => item < 0), -1);
    assert_equal(array.findIndex(item => typeof item === 'string'), 8);
    assert_equal(array.findIndex(item => typeof item === 'object'), 7);
    assert_equal(array.findIndex(item => Array.isArray(item)), 11);
    assert_equal(array.findIndex(item => item), 0);
    assert_equal(array.findIndex(item => item === null), 7);
    assert_equal(array.findIndex(item => item === undefined), 6);
    assert_equal(array.findIndex(item => isNaN(item)), 5);
    assert_equal(array.findIndex(item => item === false), 9);
}

try {
    const array = [1, 2, 3, 4, 5];
    array.findIndex();
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the predicate is not callable.');

try {
    const array = [1, 2, 3, 4, 5];
    array.findIndex("not a function");
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the predicate is not callable.');

try {
    let array = null;
    array.findIndex(item => item === 1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: Cannot read property findIndex of null');

try {
    array = undefined;
    array.findIndex(item => item === 1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'ReferenceError: array is not defined');

/*
 * @tc.name:FindLast
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [6, 7, 8, 9, 10, NaN, undefined, null, "", false, {name: "John"}, [1, 2, 3]];
    assert_equal(array.findLast(item => item === 5), undefined);
    assert_equal(array.findLast(item => item === 11), undefined);
    assert_equal(array.findLast(item => item > 5), 10);
    assert_equal(array.findLast(item => item < 0), undefined);
    assert_equal(array.findLast(item => typeof item === 'string'), "");
    assert_equal(array.findLast(item => typeof item === 'object'), [1,2,3]);
    assert_equal(array.findLast(item => Array.isArray(item)), [1,2,3]);
    assert_equal(array.findLast(item => item), [1,2,3]);
    assert_equal(array.findLast(item => item === null), null);
    assert_equal(array.findLast(item => item === undefined), undefined);
    assert_equal(array.findLast(item => isNaN(item)), [1,2,3]);
    assert_equal(array.findLast(item => item === false), false);
}

try {
    const array = [1, 2, 3, 4, 5];
    array.findLast();
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the predicate is not callable.');

try {
    const array = [1, 2, 3, 4, 5];
    array.findLast("not a function");
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the predicate is not callable.');

try {
    let array = null;
    array.findLast(item => item === 1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: Cannot read property findLast of null');

try {
    array = undefined;
    array.findLast(item => item === 1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'ReferenceError: array is not defined');

/*
 * @tc.name:FindLastIndex
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [6, 7, 8, 9, 10, NaN, undefined, null, "", false, {name: "John"}, [1, 2, 3]];
    assert_equal(array.findLastIndex(item => item === 5), -1);
    assert_equal(array.findLastIndex(item => item === 11), -1);
    assert_equal(array.findLastIndex(item => item > 5), 4);
    assert_equal(array.findLastIndex(item => item < 0), -1);
    assert_equal(array.findLastIndex(item => typeof item === 'string'), 8);
    assert_equal(array.findLastIndex(item => typeof item === 'object'), 11);
    assert_equal(array.findLastIndex(item => Array.isArray(item)), 11);
    assert_equal(array.findLastIndex(item => item), 11);
    assert_equal(array.findLastIndex(item => item === null), 7);
    assert_equal(array.findLastIndex(item => item === undefined), 6);
    assert_equal(array.findLastIndex(item => isNaN(item)), 11);
    assert_equal(array.findLastIndex(item => item === false), 9);
}

try {
    const array = [1, 2, 3, 4, 5];
    array.findLastIndex();
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the predicate is not callable.');

try {
    const array = [1, 2, 3, 4, 5];
    array.findLastIndex("not a function");
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the predicate is not callable.');

try {
    let array = null;
    array.findLastIndex(item => item === 1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: Cannot read property findLastIndex of null');

try {
    array = undefined;
    array.findLastIndex(item => item === 1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'ReferenceError: array is not defined');

/*
 * @tc.name:Flat
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [1, 2, [3, 4, [5, 6]], [], [[7], 8], 9, [10]];

    assert_equal(array.flat().toString(), '1,2,3,4,5,6,7,8,9,10');
    assert_equal(array.flat(2), [1,2,3,4,5,6,7,8,9,10]);

    const deeplyNestedArray = [1, [2, [3, [4, [5]]]]];
    assert_equal(deeplyNestedArray.flat(Infinity), [1,2,3,4,5]);

    const emptyArray = [1, [2, [], [3, []]]];
    assert_equal(emptyArray.flat().toString(), '1,2,,3,');

    const sparseArray = [1, 2, , 3, 4, [5, , 6]];
    assert_equal(sparseArray.flat(), [1,2,3,4,5,6]);

    const irregularArray = [1, [2, 3, [4, [5]]]];
    assert_equal(irregularArray.flat().toString(), '1,2,3,4,5');

    const arrayWithNonArrays = [1, 'string', {name: 'John'}, null, undefined];
    assert_equal(arrayWithNonArrays.flat().toString(), '1,string,[object Object],,');

    const arrayWithNaNAndInfinity = [1, [NaN, Infinity], [2, [3, NaN]]];
    assert_equal(arrayWithNaNAndInfinity.flat().toString(), '1,NaN,Infinity,2,3,NaN');
}

errorStr = "";
try {
    const array = [1, 2, [3, 4]];
    array.flat('string');
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, '');

errorStr = "";
try {
    const array = [1, 2, [3, 4]];
    array.flat(-1);
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, '');

/*
 * @tc.name:FlatMap
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [1, 2, 3];
    assert_equal(array.flatMap(x => [x, x * 2]), [1,2,2,4,3,6]);
    assert_equal(array.flatMap(x => []), []);
    assert_equal(array.flatMap(x => x * 2), [2,4,6]);
    assert_equal(array.flatMap((x, index) => [x, index]).toString(), '1,0,2,1,3,2');
    const sparseArray = [1, , 2, , 3];
    assert_equal(sparseArray.flatMap(x => [x]).toString(), '1,2,3');
    const nestedArray = [[1, 2], [3, 4], [5, 6]];
    assert_equal(nestedArray.flatMap(arr => arr).toString(), '1,2,3,4,5,6');
    const arrayWithEmptyArrays = [1, [], [2, 3], [], 4];
    assert_equal(arrayWithEmptyArrays.flatMap(x => x).toString(), '1,2,3,4');
    assert_equal(array.flatMap(x => x % 2 === 0 ? [x, x * 2] : x).toString(), '1,2,4,3');
}

/*
 * @tc.name:ForEach
 * @tc.desc:test Array
 * @tc.type: FUNC
 */

try {
    const array = [1, 2, 3];
    array.forEach('not a function');
} catch (error) {
    errorStr = error.toString();
}
assert_equal(errorStr, 'TypeError: the callbackfun is not callable.')

/*
 * @tc.name:Includes
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const testCases = [
    { array: [1, 2, 3, 4, 5], target: 3 },
    { array: [1, 2, 3, 4, 5], target: 6 },
    { array: [NaN, 2, 3], target: NaN },
    { array: [undefined, 2, 3], target: undefined },
    { array: ["apple", "banana", "orange"], target: "banana" },
    { array: ["apple", "banana", "orange"], target: "grape" },
    { array: [], target: 1 },
    { array: [true, false, true], target: true },
    { array: [true, false, true], target: false },
    { array: [Infinity, -Infinity], target: Infinity },
    { array: [Infinity, -Infinity], target: -Infinity },
    { array: [new Map(), new Map()], target: new Map() },
    { array: [new Set(), new Set()], target: new Set() },
];

/*
 * @tc.name:IndexOf
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 5];
    assert_equal(arr.indexOf(3), 2);
    assert_equal(arr.indexOf(1), 0);
    assert_equal(arr.indexOf(5), 4);
    assert_equal([].indexOf(1), -1);
    let arr2 = ["apple", "banana", "cherry"];
    assert_equal(arr2.indexOf("banana"), 1)
    let arr3 = [1, 2, 2, 3, 4, 2];
    assert_equal(arr3.indexOf(2), 1);
    assert_equal(arr.indexOf(10), -1);
    let arr4 = [{id: 1}, {id: 2}, {id: 3}];
    assert_equal(arr4.indexOf({id: 2}), -1);
    assert_equal(arr4.findIndex(item => item.id === 2), 1);
    assert_equal("not an array".indexOf(1), -1);
}

/*
 * @tc.name:Join
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = ["apple", "banana", "cherry"];
    assert_equal(arr.join(), 'apple,banana,cherry');
    assert_equal(arr.join(", "), 'apple, banana, cherry');
    let emptyArr = [];
    assert_equal(emptyArr.join(), "");
    let singleElementArr = ["apple"];
    assert_equal(singleElementArr.join(), 'apple');
    let mixedArr = ["apple", 1, {name: "John"}];
    assert_equal(mixedArr.join(), 'apple,1,[object Object]');
    let customSeparatorArr = ["apple", "banana", "cherry"];
    assert_equal(customSeparatorArr.join(" + "), 'apple + banana + cherry');
}

/*
 * @tc.name:Keys
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = ["apple", "banana", "cherry"];
    let keysIter = arr.keys();
    for (let key of keysIter) {
        assert_true(key == 0 || key == 1 || key == 2);
    }

    let emptyArr = [];
    let emptyKeysIter = emptyArr.keys();
    assert_equal(emptyKeysIter.next().done, true);

    let singleElementArr = ["apple"];
    let singleElementKeysIter = singleElementArr.keys();
    assert_equal(singleElementKeysIter.next().value, 0);
    assert_equal(singleElementKeysIter.next().done, true);

    let multiDimArr = [["apple", "banana"], ["cherry", "date"]];
    let multiDimKeysIter = multiDimArr.keys();
    for (let key of multiDimKeysIter) {
        assert_true(key == 0 || key == 1);
    }

    let sparseArr = [1, , 3];
    let sparseKeysIter = sparseArr.keys();
    for (let key of sparseKeysIter) {
        assert_true(key == 0 || key == 1 || key == 2);
    }
}

/*
 * @tc.name:LastIndexOf
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 2, 5];
    assert_equal(arr.lastIndexOf(2), 4);

    assert_equal(arr.lastIndexOf(1), 0);
    assert_equal(arr.lastIndexOf(5), 5);
    assert_equal(arr.lastIndexOf(6), -1);

    let emptyArr = [];
    assert_equal(emptyArr.lastIndexOf(1), -1);

    let arrWithNaN = [1, 2, NaN, 4, NaN];
    assert_equal(arrWithNaN.lastIndexOf(NaN), -1);

    let arrWithUndefined = [1, 2, undefined, 4];
    assert_equal(arrWithUndefined.lastIndexOf(undefined), 2);
}

/*
 * @tc.name:Map
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 5];
    let mappedArr = arr.map(num => num * 2);
    assert_equal(mappedArr, [2,4,6,8,10]);

    let emptyArr = [];
    let mappedEmptyArr = emptyArr.map(item => item * 2);
    assert_equal(mappedEmptyArr, []);

    let arrWithNaN = [1, 2, NaN, 4, NaN];
    let mappedArrWithNaN = arrWithNaN.map(num => num * 2);
    assert_equal(mappedArrWithNaN.toString(), '2,4,NaN,8,NaN');

    let sparseArr = [1, , 3];
    let mappedSparseArr = sparseArr.map(num => num * 2);
    assert_equal(mappedSparseArr, [2,,6]);

    let objArr = [{id: 1}, {id: 2}, {id: 3}];
    let mappedObjArr = objArr.map(obj => obj.id);
    assert_equal(mappedObjArr, [1,2,3]);

    let multiDimArr = [[1, 2], [3, 4], [5, 6]];
    let mappedMultiDimArr = multiDimArr.map(innerArr => innerArr.map(num => num * 2));
    assert_equal(mappedMultiDimArr.toString(), '2,4,6,8,10,12');
}

/*
 * @tc.name:Pop
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 5];
    let poppedElement = arr.pop();
    assert_equal(poppedElement, 5);
    assert_equal(arr, [1,2,3,4]);

    let emptyArr = [];
    let poppedEmptyElement = emptyArr.pop();
    assert_equal(poppedEmptyElement, undefined);
    assert_equal(emptyArr, []);

    let singleElementArr = [1];
    let poppedSingleElement = singleElementArr.pop();
    assert_equal(poppedSingleElement, 1);
    assert_equal(singleElementArr, []);

    let anotherSingleElementArr = ["apple"];
    let poppedAnotherSingleElement = anotherSingleElementArr.pop();
    assert_equal(poppedAnotherSingleElement, 'apple');
}

/*
 * @tc.name:Push
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3];
    arr.push(4);
    assert_equal(arr, [1,2,3,4]);

    arr.push(5, 6);
    assert_equal(arr, [1,2,3,4,5,6]);

    let emptyArr = [];
    emptyArr.push(1);
    assert_equal(emptyArr, [1]);

    let objArr = [{ id: 1 }];
    objArr.push({ id: 2 });
    assert_equal(objArr.toString(), '[object Object],[object Object]');

    let nestedArr = [1, 2];
    nestedArr.push([3, 4]);
    assert_equal(nestedArr, [1,2,[3,4]]);

    let arrWithUndefined = [1, 2, 3];
    arrWithUndefined.push(undefined);
    assert_equal(arrWithUndefined, [1,2,3,undefined]);

    let singleElementArr = [1];
    singleElementArr.push(2);
    assert_equal(singleElementArr, [1,2]);

    var ve = [''];
    try {
        for (var item in ve) {
            var vee = ve[item];
            Array.prototype.push.call(vee[item],[,,]);
        }
    } catch(e) { errorStr = e.toString(); }
    assert_equal(errorStr, 'TypeError: Cannot convert a UNDEFINED value to a JSObject');
}

/*
 * @tc.name:Reduce
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 5];
    let sum = arr.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
    assert_equal(sum, 15);

    let emptyArr = [];
    let sumOfEmptyArr = emptyArr.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
    assert_equal(sumOfEmptyArr, 0);

    let singleArr = [1];
    let sumOfSingleArr = singleArr.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
    assert_equal(sumOfSingleArr, 1);

    let arrNaN = [1, 2, NaN, 4];
    let sumOfArrNaN = arrNaN.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
    assert_equal(sumOfArrNaN.toString(), 'NaN');
}

/*
 * @tc.name:ReduceRight
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    let result1 = array1.reduceRight((acc, curr) => acc + curr);
    assert_equal(result1, 'edcba');

    let array2 = [];
    let result2 = array2.reduceRight((acc, curr) => acc + curr, "initialValue");
    assert_equal(result2, 'initialValue');

    let array3 = ["a"];
    let result3 = array3.reduceRight((acc, curr) => acc + curr);
    assert_equal(result3, 'a');

    let array4 = ["a", "b", undefined, "d"];
    let result4 = array4.reduceRight((acc, curr) => acc + curr);
    assert_equal(result4, 'dundefinedba');
}

/*
 * @tc.name:Reverse
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    array1.reverse();
    assert_equal(array1, ['e', 'd', 'c', 'b', 'a']);

    let emptyArray = [];
    emptyArray.reverse();
    assert_equal(emptyArray, []);

    let singleElementArray = ["a"];
    singleElementArray.reverse();
    assert_equal(singleElementArray, ['a']);

    let arrayWithUndefined = ["a", "b", undefined, "d"];
    arrayWithUndefined.reverse();
    assert_equal(arrayWithUndefined, ['d', undefined, 'b', 'a']);
}

/*
 * @tc.name:Shift
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    let shiftedElement1 = array1.shift();
    assert_equal(shiftedElement1, 'a');
    assert_equal(array1, ['b', 'c', 'd', 'e']);

    let emptyArray = [];
    let shiftedElement2 = emptyArray.shift();
    assert_equal(shiftedElement2, undefined);
    assert_equal(emptyArray, []);

    let singleElementArray = ["a"];
    let shiftedElement3 = singleElementArray.shift();
    assert_equal(shiftedElement3, 'a');
    assert_equal(singleElementArray, []);

    let arrayWithUndefined = ["a", undefined, "b", "c"];
    let shiftedElement4 = arrayWithUndefined.shift();
    assert_equal(shiftedElement4, 'a');
    assert_equal(arrayWithUndefined, [undefined, 'b', 'c']);
}

/*
 * @tc.name:Slice
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    let slicedArray1 = array1.slice(1, 3);
    assert_equal(slicedArray1, ['b', 'c']);

    let emptyArray = [];
    let slicedEmptyArray = emptyArray.slice(1, 3);
    assert_equal(slicedEmptyArray, []);

    let singleElementArray = ["a"];
    let slicedSingleElementArray = singleElementArray.slice(0, 1);
    assert_equal(slicedSingleElementArray, ['a']);

    let arrayWithUndefined = ["a", undefined, "b", "c"];
    let slicedArrayWithUndefined = arrayWithUndefined.slice(1, 3);
    assert_equal(slicedArrayWithUndefined, [undefined, 'b']);
}

/*
 * @tc.name:Some
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = [1, 2, 3, 4, 5];
    let isSomeEven = array1.some(num => num % 2 === 0);
    assert_true(isSomeEven);

    let array2 = [1, 3, 5, 7, 9];
    let isSomeEven2 = array2.some(num => num % 2 === 0);
    assert_true(!isSomeEven2);

    let emptyArray = [];
    let isSomeEmpty = emptyArray.some(num => num > 0);
    assert_true(!isSomeEmpty);

    let singleElementArray = [1];
    let isSomeSingleElement = singleElementArray.some(num => num > 0);
    assert_true(isSomeSingleElement);

    let arrayWithUndefined = [1, undefined, 3, 5];
    let isSomeUndefined = arrayWithUndefined.some(num => num === undefined);
    assert_true(isSomeUndefined);
}

/*
 * @tc.name:Sort
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = [3, 1, 4, 1, 5, 9, 2, 6, 5];
    array1.sort();
    assert_equal(array1, [1,1,2,3,4,5,5,6,9]);

    let emptyArray = [];
    emptyArray.sort();
    assert_equal(emptyArray, []);

    let singleElementArray = [1];
    singleElementArray.sort();
    assert_equal(singleElementArray, [1]);

    let arrayWithUndefined = [1, undefined, 3, 5];
    arrayWithUndefined.sort();
    assert_equal(arrayWithUndefined, [1,3,5,undefined]);

    let arrayWithStrings = ["banana", "apple", "cherry"];
    arrayWithStrings.sort();
    assert_equal(arrayWithStrings, ['apple', 'banana', 'cherry']);
}

/*
 * @tc.name:Splice
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    let removedElements1 = array1.splice(2, 2, "x", "y");
    assert_equal(removedElements1, ['c', 'd']);
    assert_equal(array1, ['a', 'b', 'x', 'y', 'e']);

    let emptyArray = [];
    let removedElements2 = emptyArray.splice(0, 0, "x", "y");
    assert_equal(removedElements2, []);
    assert_equal(emptyArray, ['x', 'y']);

    let singleElementArray = ["a"];
    let removedElements3 = singleElementArray.splice(0, 1, "x", "y");
    assert_equal(removedElements3, ['a']);
    assert_equal(singleElementArray, ['x', 'y']);

    let arrayWithUndefined = [1, undefined, 3, 5];
    let removedElements4 = arrayWithUndefined.splice(1, 2);
    assert_equal(removedElements4, [undefined, 3]);
    assert_equal(arrayWithUndefined, [1, 5]);
}

/*
 * @tc.name:toString
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array = ["apple", "banana", "cherry"];
    let string = array.toString();
    assert_equal(string, 'apple,banana,cherry');

    let numbers = [1, 2, 3, 4, 5];
    let string2 = numbers.toString();
    assert_equal(string2, '1,2,3,4,5');

    let mixed = [1, "two", true];
    let string3 = mixed.toString();
    assert_equal(string3, "1,two,true");
}

/*
 * @tc.name:Unshift
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c"];
    let newLength1 = array1.unshift("x", "y");
    assert_equal(newLength1, 5);
    assert_equal(array1, ['x', 'y', 'a', 'b', 'c']);

    let emptyArray = [];
    let newLength2 = emptyArray.unshift("x", "y");
    assert_equal(newLength2, 2);
    assert_equal(emptyArray, ['x', 'y']);

    let singleElementArray = ["a"];
    let newLength3 = singleElementArray.unshift("x");
    assert_equal(newLength3, 2);
    assert_equal(singleElementArray, ['x', 'a']);

    let arrayWithUndefined = [1, 2, undefined];
    let newLength4 = arrayWithUndefined.unshift("x");
    assert_equal(newLength4, 4);
    assert_equal(arrayWithUndefined, ['x', 1, 2, undefined]);

    let arrayWithHole = [1, 2, ,4];
    let newLength5 = arrayWithHole.unshift(5, 6, 7);
    assert_equal(newLength5, 7);
    assert_equal(arrayWithHole, [5,6,7,1,2,,4]);

    let arrayWithHole1 = [1, 2, ,4];
    let newLength6 = arrayWithHole1.unshift(5, 6, 7, 8);
    assert_equal(newLength6, 8);
    assert_equal(arrayWithHole1, [5,6,7,8,1,2,,4]);
}

/*
 * @tc.name:ToReversed ToSorted ToSpliced With
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    assert_equal(array1.toReversed(), ['e', 'd', 'c', 'b', 'a']);

    let emptyArray = [];
    assert_equal(emptyArray.toReversed(), []);

    let singleElementArray = ["a"];
    assert_equal(singleElementArray.toReversed(), ['a']);

    let arrayWithUndefined = ["a", "b", undefined, "d"];
    assert_equal(arrayWithUndefined.toReversed(), ['d', undefined, 'b', 'a']);
}

{
    let array1 = [3, 1, 4, 1, 5, 9, 2, 6, 5];
    assert_equal(array1.toSorted(), [1,1,2,3,4,5,5,6,9]);

    let emptyArray = [];
    assert_equal(emptyArray.toSorted(), []);

    let singleElementArray = [1];
    assert_equal(singleElementArray.toSorted(), [1]);

    let arrayWithUndefined = [1, undefined, 3, 5];
    assert_equal(arrayWithUndefined.toSorted(), [1,3,5,undefined]);

    let arrayWithStrings = ["banana", "apple", "cherry"];
    assert_equal(arrayWithStrings.toSorted(), ['apple', 'banana', 'cherry']);
}

{
    let array1 = ["a", "b", "c", "d", "e"];
    let removedElements1 = array1.toSpliced(2, 2, "x", "y");
    assert_equal(removedElements1, ['a', 'b', 'x', 'y', 'e']);
    assert_equal(array1, ['a', 'b', 'c', 'd', 'e']);

    let emptyArray = [];
    let removedElements2 = emptyArray.toSpliced(0, 0, "x", "y");
    assert_equal(removedElements2, ['x', 'y']);
    assert_equal(emptyArray, []);

    let singleElementArray = ["a"];
    let removedElements3 = singleElementArray.toSpliced(0, 1, "x", "y");
    assert_equal(removedElements3, ['x', 'y']);
    assert_equal(singleElementArray, ['a']);

    let arrayWithUndefined = [1, undefined, 3, 5];
    let removedElements4 = arrayWithUndefined.toSpliced(1, 2);
    assert_equal(removedElements4, [1,5]);
    assert_equal(arrayWithUndefined, [1,undefined,3,5]);
}

/*
 * @tc.name:IsArray
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    // print true
    assert_true(Array.isArray([]));
    assert_true(Array.isArray([1]));
    assert_true(Array.isArray(new Array()));
    assert_true(Array.isArray(new Array("a", "b", "c", "d")));
    assert_true(Array.isArray(new Array(3)));
    assert_true(Array.isArray(Array.prototype));

    // print false
    assert_true(!Array.isArray());
    assert_true(!Array.isArray({}));
    assert_true(!Array.isArray(null));
    assert_true(!Array.isArray(undefined));
    assert_true(!Array.isArray(17));
    assert_true(!Array.isArray("Array"));
    assert_true(!Array.isArray(true));
    assert_true(!Array.isArray(false));
    assert_true(!Array.isArray(new Uint8Array(32)));
    assert_true(!Array.isArray({ __proto__: Array.prototype }));
}

var arr_push = [];
Object.defineProperty(arr_push, "length", { writable : false});
try {
    arr_push.push(3);
} catch (e) {
    assert_true(e instanceof TypeError);
}

// find index object
let findIndexArray = new Array(4);
const obj0 = { ["obj0"]: 0 };
const obj1 = { ["obj1"]: 1 };
const obj2 = { ["obj2"]: 2 };
const obj3 = { ["obj3"]: 3 };

findIndexArray[0] = obj0;
findIndexArray[1] = obj1;
findIndexArray[2] = obj2;
findIndexArray[3] = obj3;

assert_equal(findIndexArray.findIndex(element => element == obj1), 1);
assert_equal(findIndexArray.findIndex(element => element == { ["obj3"]: 3 }), -1);

// find index bigint
findIndexArray = new Array(4);
findIndexArray[0] = 1n;
findIndexArray[1] = 2n;
findIndexArray[2] = 3n;
findIndexArray[3] = 4n;
assert_equal(findIndexArray.findIndex(element => element == 2n), 1);
assert_equal(findIndexArray.findIndex(element => element == 0n), -1);


// find index string
findIndexArray = new Array(4);
findIndexArray[0] = "a";
findIndexArray[1] = "b";
findIndexArray[2] = "c";
findIndexArray[3] = "d";
assert_equal(findIndexArray.findIndex(element => element == "b"), 1);
assert_equal(findIndexArray.findIndex(element => element == "e"), -1);

{
    Array.prototype.__proto__ = null;
    var vp = Array.prototype;
    vp.map(function(vp) {});
    vp.filter(function(vp) {});
    Array.prototype.__proto__ = Object.prototype;
}
// slice
let sliceArray = [0, 1, 2, 3, 4, 5];
assert_equal(sliceArray.slice(0), [0,1,2,3,4,5]);
assert_equal(sliceArray.slice(-1), [5]);
assert_equal(sliceArray.slice(1, 4), [1,2,3]);
assert_equal(sliceArray.slice(1, 10), [1,2,3,4,5]);

// Test ArrayIteratorNext for arraylike object
{
    const arrayLike = {
        get length() {
            if (this._canAccessLength) {
                return 2;
            } else {
                throw new Error("Access to length is denied");
            }
        },
        0: 'a',
        1: 'b'
    };
    
    const iterator = Array.prototype[Symbol.iterator].call(arrayLike);
    let hasException = false;
    try {
        iterator.next()
    } catch {
        hasException = true;
    }
    assert_true(hasException);
}

// Test ArrayIteratorNext for arraylike object
{
    const arrayLike = {
        get length() {
            return 2;
        },
        0: 'a',
        1: 'b'
    };
  
    // Values Iterator
    const valuesIterator = Array.prototype[Symbol.iterator].call(arrayLike);
    let result;
    while (!(result = valuesIterator.next()).done) {
        let v = result.value;
        let d = result.done;
        assert_true((v == 'a' && d == false) || (v == 'b' && d == false));

    }
    assert_true((result.value == undefined) && (result.done == true));
  
    // Keys Iterator
    const keysIterator = Array.prototype.keys.call(arrayLike);
    while (!(result = keysIterator.next()).done) {
        let v = result.value;
        let d = result.done;
        assert_true((v == 0 && d == false) || (v == 1 && d == false));
    }
    assert_true((result.value == undefined) && (result.done == true));
  
    // Entries Iterator
    const entriesIterator = Array.prototype.entries.call(arrayLike);
    while (!(result = entriesIterator.next()).done) {
        let v = result.value.toString();
        let d = result.done;
        assert_true((v == '0,a' && d == false) || (v == '1,b' && d == false));
    }
    assert_true((result.value == undefined) && (result.done == true));
}

// Test ArrayIteratorNext for non-jsArrayIterator
{
    const nonArrayIterator = {
        next: function () {
            return { done: true, value: undefined };
        }
      };
      
    try {
        for (const item of nonArrayIterator) {
            throw new Error();
        }
    } catch (error) {
        if (error instanceof TypeError) {
            assert_true(true);
        } else {
            assert_true(false);
        }
    }      
}


let x1 = new Array(10);
let x2 = new Array(1,2,3,undefined);
let x3 = new Array();

x1.constructor = null

assert_equal(ArkTools.hasConstructor(x2), false);
assert_equal(ArkTools.hasConstructor(x3), false);
// pop 
var y = new Array(10)
for (let i = 0; i < y.length; i++) {
    y[i] = i
}
assert_equal(y.pop(), 9); //: 9
assert_equal(y.pop(), 8); //: 8
assert_equal(y.pop(), 7); //: 7
assert_equal(y.pop(), 6); //: 6
assert_equal(y.pop(), 5); //: 5
assert_equal(y, [0,1,2,3,4]);
// shift
var shiftArray = new Array(10)
for (let i = 0; i < shiftArray.length; i++) {
    shiftArray[i] = i
}
assert_equal(shiftArray.shift(), 0); //: 0
assert_equal(shiftArray.shift(), 1); //: 1
assert_equal(shiftArray.shift(), 2); //: 2
assert_equal(shiftArray.shift(), 3); //: 3
assert_equal(shiftArray.shift(), 4); //: 4
assert_equal(shiftArray, [5,6,7,8,9]);
// copyWithin
let copyWithInArray = new Array(0, 1, 2, 3, 4);
assert_equal(copyWithInArray.copyWithin(2,1,3), [0,1,1,2,4])

for (let i = 0; i < 2; i++) {
    let copyWithInArray1 = [1, 2, 3, 4, 5, 6, 7, 8, 9];
    copyWithInArray1.copyWithin(1,2,3);
    if (i == 0) {
        assert_equal(copyWithInArray1, [1,3,3,4,5,6,7,8,9]);
    }
    copyWithInArray1.copyWithin(1,3,2);
    if (i == 0) {
        assert_equal(copyWithInArray1, [1,3,3,4,5,6,7,8,9]);
    }
    copyWithInArray1.copyWithin(2,1,3);
    if (i == 0) {
        assert_equal(copyWithInArray1, [1,3,3,3,5,6,7,8,9]);
    }
    copyWithInArray1.copyWithin(2,3,1);
    if (i == 0) {
        assert_equal(copyWithInArray1, [1,3,3,3,5,6,7,8,9]);
    }
    copyWithInArray1.copyWithin(3,1,2);
    if (i == 0) {
        assert_equal(copyWithInArray1, [1,3,3,3,5,6,7,8,9]);
    }
    copyWithInArray1.copyWithin(3,2,1);
    if (i == 0) {
        assert_equal(copyWithInArray1, [1,3,3,3,5,6,7,8,9]);
    }
}
assert_equal([0, 1, 2, 3].copyWithin(0, 1, -10), [0,1,2,3]);

test_end();