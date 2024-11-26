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
print(a.length);

var arr1 = [1, 1, 1, 1, 1, 1];
arr1.fill(0, 2, 4);
print(arr1);

var arr2 = new Array(100);
arr2.fill(0, 2, 4);
var a1 = arr2.slice(0, 5);
print(arr2.length);
print(a1);

var arr3 = [1, 2, 3, 4, 5, 6];
arr3.pop();
print(arr3.length);
print(arr3);

var arr4 = [1, 3, 4];
arr4.splice(1, 0, 2);
print(arr4.length);
print(arr4);
// 1, 2, 3, 4
var arr4 = [1, 2, 3, 4];
arr4.splice(3, 1, 3);
print(arr4.length);
print(arr4);
// 1, 2, 3, 3

var arr5 = [1, 2, 3, 4, 5, 6];
arr5.shift();
print(arr5.length);
print(arr5);
// 1, 2, 3, 4, 5

var arr6 = [1, 2, 3, 4, 5];
arr6.reverse();
print(arr6);

var arr7 = [];
arr7[2] = 10;
print(arr7.pop()); // 10
print(arr7.pop()); // undefined
print(arr7.pop()); // undefined

var arr8 = [];
arr8[1] = 8;
print(arr8.shift()); // undefined
print(arr8.shift()); // 8
print(arr8.shift()); // undefined

// Test on Array::Splice
var arr9 = new Array(9);
print(arr9.length); // 9
arr9.splice(0, 9);
print(arr9.length); // 0
for (let i = 0; i < arr9.length; i++) {
    print(arr9[i]);
}

var arr10 = new Array(9);
print(arr10.length); // 9
arr10.splice(0, 8, 1);
print(arr10.length); // 2
for (let i = 0; i < arr10.length; i++) {
    print(arr10[i]); // 1, undefined
}

var arr11 = new Array(9);
print(arr11.length); // 9
arr11.splice(1, 8, 1);
print(arr11.length); // 2
for (let i = 0; i < arr11.length; i++) {
    print(arr11[i]); // undefined, 1
}

var arr12 = new Array(9);
print(arr12.length); // 9
arr12.splice(0, 4, 1, 2, 3, 4, 5);
print(arr12.length); // 10
for (let i = 0; i < arr12.length; i++) {
    print(arr12[i]); // 1, 2, 3, 4, 5, undefined, undefined, undefined, undefined, undefined
}

var arr13 = new Array(9);
print(arr13.length); // 9
arr13.splice(7, 5, 1, 2, 3);
print(arr13.length); // 10
for (var i = 0; i < arr13.length; i++) {
    print(arr13[i]); // undefined, undefined, undefined, undefined, undefined, undefined, undefined, 1, 2, 3
}

var arr14 = Array.apply(null, Array(16));
print(arr14.length);
var arr15 = Array.apply(null, [1, 2, 3, 4, 5, 6]);
print(arr15.length);

var a = '0';
print(Array(5).indexOf(a));

const v3 = new Float32Array(7);
try {
    v3.filter(Float32Array)
} catch (error) {
    print("The NewTarget is undefined")
}

const v20 = new Array(2);
let v21;
try {
    v21 = v20.pop();
    print(v21);
} catch (error) {

}

var arr21 = [1, 2, 3, 4, , 6];
print(arr21.at(0));
print(arr21.at(5));
print(arr21.at(-1));
print(arr21.at(6));
print(arr21.at('1.9'));
print(arr21.at(true));
var arr22 = arr21.toReversed();
print(arr22)
print(arr21)
var arr23 = [1, 2, 3, 4, 5];
print(arr23.with(2, 6)); // [1, 2, 6, 4, 5]
print(arr23); // [1, 2, 3, 4, 5]

const months = ["Mar", "Jan", "Feb", "Dec"];
const sortedMonths = months.toSorted();
print(sortedMonths); // ['Dec', 'Feb', 'Jan', 'Mar']
print(months); // ['Mar', 'Jan', 'Feb', 'Dec']

const values = [1, 10, 21, 2];
const sortedValues = values.toSorted((a, b) => {
    return a - b
});
print(sortedValues); // [1, 2, 10, 21]
print(values); // [1, 10, 21, 2]

const arrs = new Array(6);
for (let i = 0; i < 6; i++) {
    var str = i.toString();
    if (i != 1) {
        arrs[i] = str;
    }
}
arrs.reverse();
print(arrs); // 5,4,3,2,,0

function handleExpectedErrorCaught(prompt, e) {
    if (e instanceof Error) {
        print(`Expected ${e.name} caught in ${prompt}.`);
    } else {
        print(`Expected error message caught in ${prompt}.`);
    }
}

function handleUnexpectedErrorCaught(prompt, e) {
    if (e instanceof Error) {
        print(`Unexpected ${e.name} caught in ${prompt}: ${e.message}`);
        if (typeof e.stack !== 'undefined') {
            print(`Stacktrace:\n${e.stack}`);
        }
    } else {
        print(`Unexpected error message caught in ${prompt}: ${e}`);
    }
}

// Test cases for reverse()
print("======== Begin: Array.prototype.reverse() ========");
try {
    const arr0 = [];
    print(`arr0.reverse() === arr0 ? ${arr0.reverse() === arr0}`); // true
    print(`arr0.length after reverse() called = ${arr0.length}`); // 0

    const arr1 = [1];
    print(`arr1.reverse() === arr1 ? ${arr1.reverse() === arr1}`); // true
    print(`arr1 after reverse() called = ${arr1}`); // 1

    const arrWithHoles = [];
    arrWithHoles[1] = 1;
    arrWithHoles[4] = 4;
    arrWithHoles[6] = undefined;
    arrWithHoles.length = 10;
    // arrWithHoles = [Hole, 1, Hole, Hole, 4, Hole, undefined, Hole, Hole, Hole]
    print(`arrWithHoles before reverse() called = ${arrWithHoles}`); // ,1,,,4,,,,,
    print(`arrWithHoles.reverse()               = ${arrWithHoles.reverse()}`); // ,,,,,4,,,1,
    print(`arrWithHoles after reverse() called  = ${arrWithHoles}`); // ,,,,,4,,,1,
} catch (e) {
    handleUnexpectedErrorCaught(e);
}
print("======== End: Array.prototype.reverse() ========");

print("======== Begin: Array.prototype.indexOf() & Array.prototype.lastIndexOf() ========");
// Test case for indexOf() and lastIndexOf()
try {
    const arr = [0, 10, 20];
    arr.length = 10;
    arr[3] = 80;
    arr[4] = 40;
    arr[6] = undefined;
    arr[7] = 10;
    arr[8] = "80";
    print("arr = [0, 10, 20, 80, 40, Hole, undefined, 10, \"80\", Hole]");
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
    for (let i = 0; i < resultGroups.length; i += 2) {
        print(`${resultGroups[i]}: ${resultGroups[i + 1]}`);
    }

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
    print(`  - indexOf:     result = ${result2}, accessCount = ${accessCount}`);
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
    print(`  - lastIndexOf: result = ${result3}, accesscount = ${accessCount}`);

    print("Group 11: fromIndex that triggers exceptions:");
    for (let [prompt, fromIndex] of [["bigint", 1n], ["symbol", Symbol()]]) {
        for (let method of [Array.prototype.indexOf, Array.prototype.lastIndexOf]) {
            try {
                const result = method.call(arr, 10, fromIndex);
                print(`ERROR: Unexpected result (which is ${result}) returned by method '${method.name}': ` +
                `Expects a TypeError thrown for fromIndex type '${prompt}'.`);
            } catch (e) {
                // Expects a TypeError thrown and caught here.
                handleExpectedErrorCaught(`${method.name} when fromIndex is ${prompt}`, e);
            }
        }
    }
} catch (e) {
    handleUnexpectedErrorCaught(e);
}
print("======== End: Array.prototype.indexOf() & Array.prototype.lastIndexOf() ========");

// Test Array.prototype.filter when callbackFn is not callable
try {
    [].filter(1);
} catch (e) {
    print("CallbackFn is not callable");
}

var getTrap = function (t, key) {
    if (key === "length") return { [Symbol.toPrimitive]() {
        return 3
    } };
    if (key === "2") return "baz";
    if (key === "3") return "bar";
};
var target1 = [];
var obj = new Proxy(target1, { get: getTrap, has: () => true });
print([].concat(obj));
print(Array.prototype.concat.apply(obj))

const v55 = new Float64Array();
const v98 = [-5.335880620598348e+307, 1.0, 1.0, 2.220446049250313e-16, -1.6304390272817058e+308];

function f99(a100) {
}

function f110() {
    v98[2467] = v55;
}

Object.defineProperty(f99, Symbol.species, { configurable: true, enumerable: true, value: f110 });
v98.constructor = f99;
print(JSON.stringify(v98.splice(4)));

var count;
var params;

class MyObserveArrray extends Array {
    constructor(...args) {
        super(...args);
        print("constructor")
        params = args;
    }

    static get [Symbol.species]() {
        print("get [Symbol.species]")
        count++;
        return this;
    }
}

count = 0;
params = undefined;
new MyObserveArrray().filter(() => {
});
print(count, 1);
print(params, [0]);

count = 0;
params = undefined;
new MyObserveArrray().concat(() => {
});
print(count, 1);
print(params, [0]);

count = 0;
params = undefined;
new MyObserveArrray().slice(() => {
});
print(count, 1);
print(params, [0]);

count = 0;
params = undefined;
new MyObserveArrray().splice(() => {
});
print(count, 1);
print(params, [0]);

new MyObserveArrray([1, 2, 3, 4]).with(0, 0);
new MyObserveArrray([1, 2, 3, 4]).toReversed(0, 0);
new MyObserveArrray([1, 2, 3, 4]).toSpliced(0, 0, 0);

arr = new Array(1026);
arr.fill(100);
print(arr.with(0, "n")[0])

arr = new Array(1026);
arr.fill(100);
print(arr.toReversed()[0])

arr = new Array(1026);
arr.fill(100);
print(arr.toSpliced(0, 0, 0, 0)[0])

var arr25 = []
arr25[1025] = 0;
print(arr25.includes({}, 414));
print(arr25.includes(1025,109));

var arr26 = []
arr25[100] = 0;
print(arr25.includes({}, 26));

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
print(Array.prototype.length)

let unscopables1 = Array.prototype[Symbol.unscopables];
let unscopables2 = Array.prototype[Symbol.unscopables];
print(unscopables1 == unscopables2)

arr = []
let index = 4294967291;
arr[index] = 0;
arr.length = 10;
arr.fill(10);
print(arr)

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
print(result_copywithin1);
var result_copywithin2 = arr_copywithin2.copyWithin(0, 3, 7);
print(result_copywithin2);
var arr_copywithin3 = arr_copywithin3.copyWithin(0, 0, 10);
print(arr_copywithin3);
//elementskind is generic but hclass == generic hclass
var arr_copywithin4 = arr_copywithin4.copyWithin(3, 0, 6);
print(arr_copywithin4);

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
print(result);

for (let i = 0; i < ArraySize; i++) {
    //elementskind is generic but hclass != generic hclass
    result = arr_copywithin6.copyWithin(0, QuarterSize * 2, QuarterSize * 3);
}
print(result);

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
print(result_every1);
var result_every2 = arr_every2.every(testEvery);
print(result_every2);
var result_every3 = arr_every3.every(testEvery);
print(result_every3);
var result_every4 = arr_every4.every(testEvery4);
print(result_every4);

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
print(result_reduceRight1);
var result_reduceRight2 = arr_reduceRight2.reduceRight(testReduceRight, 100);
print(result_reduceRight2);
var result_reduceRight3 = arr_reduceRight3.reduceRight(testReduceRight, 100);
print(result_reduceRight3);
var result_reduceRight4 = arr_reduceRight4.reduceRight(testReduceRight4, 100);
print(result_reduceRight4);

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
print(result_some1);
var result_some2 = arr_some2.some(testSome);
print(result_some2);
var result_some3 = arr_some3.some(testSome);
print(result_some3);

// Test case for unshift()
var arr_unshift1 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr_unshift2 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];

var result_unshift1 = arr_unshift1.unshift(1, 2, 3, 4, 5);
print(result_unshift1);
var result_unshift2 = arr_unshift2.unshift(1, 2, 3);
print(result_unshift2);

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
print(new C3(C3));

// Test case for toSorted()
var arr_toSorted1 = ["Mar", "Jan", "Feb", "Dec"];
var arr_toSorted2 = new Array();
arr_toSorted2[0] = "Mar";
arr_toSorted2[1] = "Jan";
arr_toSorted2[2] = "Feb";
arr_toSorted2[3] = "Dec";
var result_toSorted1 = arr_toSorted1.toSorted();
print(result_toSorted1);
var result_toSorted2 = arr_toSorted2.toSorted();
print(result_toSorted2);

const v0 = [0, 1];
const mapEd = v0.map(() => {
    v0["pop"]();
});
print(new Uint16Array(v0).length);

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
    print(err.name);
}

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
    print(err.name);
}

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
    print(err.name);
}

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
    print(err.name);
}

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
    print(err.name);
}

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
    print(err.name);
}

try {
    const v1 = new Uint16Array();
    for (let i4 = 0, i5 = 10; i5; i5--) {
    }
    undefined instanceof v1.values();
} catch (error) {
    print(error.name);
}

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

for (let i = 0; i < originalArrays.length; i++) {
    print(originalArrays[i]);
}

try {
    const arr1 = [1, 2, 3];
    print(arr1[10]);
    print("Exception usage, but does not throw an error");
} catch (error) {
    print("Caught an error: " + error);
}

try {   
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const arr2 = new Array(-1);
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:from
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const newArray = Array.from(originalArrays);
newArray.forEach(array => {
    print(array);
});

try {
    Array.from([1, 2, 3], "not a function");
} catch (error) {
    print("Caught an error: ", error);
}

try {
    Array.from([1, 2, 3], () => { throw new Error("Something went wrong"); });
} catch (error) {
    print("Caught an error: ", error);
}

try {
    Array.from(123);
    Array.from({});
    Array.from([1, 2, 3], () => {}, 123);
    print("Exception usage, but does not throw an error");
} catch (error) {
    print("Caught an error: " + error);
}

/*
 * @tc.name:isArray
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
originalArrays.forEach((array, index) => {
    print(Array.isArray(array));
});

try {
    print(Array.isArray());
    print(Array.isArray(123));
    print(Array.isArray("not an array"));
    print(Array.isArray(null));
    print(Array.isArray(undefined));
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:Of
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const ArrayOf = Array.of(...originalArrays);
print(ArrayOf[1]);

try {
    const arr1 = Array.of(undefined);
    const arr2 = Array.of(1, 2, 3, 4, 5, 6, 7, 8, 9, 10);
    print(arr1);
    print(arr2);
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:At
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
print(originalArrays.at(1));
print(originalArrays.at(3));
print(originalArrays.at(-1));
print(originalArrays.at(-2));

try {
    print(originalArrays.at());
    print(originalArrays.at(100));
    print(originalArrays.at(null));
    print(originalArrays.at(undefined));
    print(originalArrays.at(new Map()));
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:concat
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const array1 = [1, "two", true];
const array2 = [null, undefined, { key: "value" }];
const array3 = [];

const concatenatedArray = array1.concat(array2, array3);
print("Concatenated Array:", concatenatedArray);

const nestedArray = [[1, 2], ["a", "b"], [true, false]];
const nestedConcatenatedArray = array1.concat(nestedArray, array2);
print("Nested Concatenated Array:", nestedConcatenatedArray);

const mixedConcatenatedArray = array1.concat(4, "five", { prop: "value" });
print("Mixed Concatenated Array:", mixedConcatenatedArray);

const spreadConcatenatedArray = [...array1, ...array2, ...array3];
print("Spread Concatenated Array:", spreadConcatenatedArray);

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

print("Original Arrays:", originalArrays);
print("Copied Array 1:", copiedArray1);
print("Copied Array 2:", copiedArray2);
print("Copied Array 3:", copiedArray3);
print("Copied Array 4:", copiedArray4);
print("Copied Array 5:", copiedArray5);

for (const [index, value] of originalArrays.entries()) {
    print(`Index: ${index}`);
    print(`Value: ${value}`);
}

/*
 * @tc.name:Every
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
const numbers1 = [2, 4, 6, 8, 10];
const allEven = numbers1.every(num => num % 2 === 0);
print(allEven);

const numbers2 = [1, 2, 3, 4, 5];
const allEven2 = numbers2.every(num => num % 2 === 0);
print(allEven2);

const emptyArray1 = [];
const allEmpty = emptyArray1.every(num => num % 2 === 0);
print(allEmpty);

const emptyArray2 = [];
const allEmpty2 = emptyArray2.every(() => true);
print(allEmpty2);

const mixedArray = [2, 4, "hello", 8, 10];
const allNumbers = mixedArray.every(num => typeof num === "number");
print(allNumbers);

const emptyArray3 = [];
const allNonNegative = emptyArray3.every(num => num >= 0);
print(allNonNegative);

try {
    const arr = [1, 2, 3];
    const result = arr.every("not a function");
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const arr = [1, 2, 3];
    const result = arr.every(num => num < undefinedVariable);
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:Fill
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array1 = [1, 2, 3, 4, 5];
    print(array1.fill(0));

    const array2 = [1, 2, 3, 4, 5];
    print(array2.fill(0, 2));

    const array3 = [1, 2, 3, 4, 5];
    print(array3.fill(0, 1, 3));

    const array4 = new Array(5);
    print(array4.fill(0));

    const array5 = Array.from({ length: 5 }, (_, index) => index + 1);
    print(array5.fill(0, 2));

    const array6 = Array.from({ length: 5 }, (_, index) => index + 1);
    print(array6.fill(0, 1, 3));

    const array7 = Array(5).fill("hello");
    print(array7);

    const array8 = [1, 2, 3];
    print(array8.fill(0, -2));

    const array9 = [1, 2, 3];
    print(array9.fill(0, -2, -1));
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
    print(evenNumbers);

    const greaterThanTwo = numbers.filter(num => num > 2);
    print(greaterThanTwo);

    const lessThanTen = numbers.filter(num => num < 10);
    print(lessThanTen);

    const divisibleByThree = numbers.filter(num => num % 3 === 0);
    print(divisibleByThree);

    const words = ["apple", "banana", "pear", "orange"];
    const longWords = words.filter(word => word.length >= 5);
    print(longWords);

    const persons = [
        { name: "Alice", age: 25 },
        { name: "Bob", age: 17 },
        { name: "Charlie", age: 30 }
    ];
    const adults = persons.filter(person => person.age > 18);
    print(adults);
}

try {
    const arr = [1, 2, 3];
    const result = arr.filter("not a function");
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const obj = { a: 1, b: 2 };
    const result = obj.filter(() => true);
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:Find
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [6, 7, 8, 9, 10, NaN, undefined, null, "", false, {name: "John"}, [1, 2, 3]];
    print(array.find(item => item === 5));
    print(array.find(item => item === 11));
    print(array.find(item => item > 5));
    print(array.find(item => item < 0));
    print(array.find(item => typeof item === 'string'));
    print(array.find(item => typeof item === 'object'));
    print(array.find(item => Array.isArray(item)));
    print(array.find(item => item));
    print(array.find(item => item === null));
    print(array.find(item => item === undefined));
    print(array.find(item => isNaN(item)));
    print(array.find(item => item === false));
}

try {
    const array = [1, 2, 3, 4, 5];
    print(array.find());
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const array = [1, 2, 3, 4, 5];
    print(array.find("not a function"));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    let array = null;
    print(array.find(item => item === 1));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    array = undefined;
    print(array.find(item => item === 1));
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:FindIndex
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [6, 7, 8, 9, 10, NaN, undefined, null, "", false, {name: "John"}, [1, 2, 3]];
    print(array.findIndex(item => item === 5));
    print(array.findIndex(item => item === 11));
    print(array.findIndex(item => item > 5));
    print(array.findIndex(item => item < 0));
    print(array.findIndex(item => typeof item === 'string'));
    print(array.findIndex(item => typeof item === 'object'));
    print(array.findIndex(item => Array.isArray(item)));
    print(array.findIndex(item => item));
    print(array.findIndex(item => item === null));
    print(array.findIndex(item => item === undefined));
    print(array.findIndex(item => isNaN(item)));
    print(array.findIndex(item => item === false));
}

try {
    const array = [1, 2, 3, 4, 5];
    print(array.findIndex());
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const array = [1, 2, 3, 4, 5];
    print(array.findIndex("not a function"));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    let array = null;
    print(array.findIndex(item => item === 1));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    array = undefined;
    print(array.findIndex(item => item === 1));
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:FindLast
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [6, 7, 8, 9, 10, NaN, undefined, null, "", false, {name: "John"}, [1, 2, 3]];
    print(array.findLast(item => item === 5));
    print(array.findLast(item => item === 11));
    print(array.findLast(item => item > 5));
    print(array.findLast(item => item < 0));
    print(array.findLast(item => typeof item === 'string'));
    print(array.findLast(item => typeof item === 'object'));
    print(array.findLast(item => Array.isArray(item)));
    print(array.findLast(item => item));
    print(array.findLast(item => item === null));
    print(array.findLast(item => item === undefined));
    print(array.findLast(item => isNaN(item)));
    print(array.findLast(item => item === false));
}

try {
    const array = [1, 2, 3, 4, 5];
    print(array.findLast());
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const array = [1, 2, 3, 4, 5];
    print(array.findLast("not a function"));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    let array = null;
    print(array.findLast(item => item === 1));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    array = undefined;
    print(array.findLast(item => item === 1));
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:FindLastIndex
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [6, 7, 8, 9, 10, NaN, undefined, null, "", false, {name: "John"}, [1, 2, 3]];
    print(array.findLastIndex(item => item === 5));
    print(array.findLastIndex(item => item === 11));
    print(array.findLastIndex(item => item > 5));
    print(array.findLastIndex(item => item < 0));
    print(array.findLastIndex(item => typeof item === 'string'));
    print(array.findLastIndex(item => typeof item === 'object'));
    print(array.findLastIndex(item => Array.isArray(item)));
    print(array.findLastIndex(item => item));
    print(array.findLastIndex(item => item === null));
    print(array.findLastIndex(item => item === undefined));
    print(array.findLastIndex(item => isNaN(item)));
    print(array.findLastIndex(item => item === false));
}

try {
    const array = [1, 2, 3, 4, 5];
    print(array.findLastIndex());
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const array = [1, 2, 3, 4, 5];
    print(array.findLastIndex("not a function"));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    let array = null;
    print(array.findLastIndex(item => item === 1));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    array = undefined;
    print(array.findLastIndex(item => item === 1));
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:Flat
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [1, 2, [3, 4, [5, 6]], [], [[7], 8], 9, [10]];

    print(array.flat());
    print(array.flat(2));

    const deeplyNestedArray = [1, [2, [3, [4, [5]]]]];
    print(deeplyNestedArray.flat(Infinity));

    const emptyArray = [1, [2, [], [3, []]]];
    print(emptyArray.flat());

    const sparseArray = [1, 2, , 3, 4, [5, , 6]];
    print(sparseArray.flat());

    const irregularArray = [1, [2, 3, [4, [5]]]];
    print(irregularArray.flat());

    const arrayWithNonArrays = [1, 'string', {name: 'John'}, null, undefined];
    print(arrayWithNonArrays.flat());

    const arrayWithNaNAndInfinity = [1, [NaN, Infinity], [2, [3, NaN]]];
    print(arrayWithNaNAndInfinity.flat());
}

try {
    const array = [1, 2, [3, 4]];
    print(array.flat('string'));
} catch (error) {
    print("Caught an error: ", error);
}

try {
    const array = [1, 2, [3, 4]];
    print(array.flat(-1));
} catch (error) {
    print("Caught an error: ", error);
}

/*
 * @tc.name:FlatMap
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    const array = [1, 2, 3];
    print(array.flatMap(x => [x, x * 2]));
    print(array.flatMap(x => []));
    print(array.flatMap(x => x * 2));
    print(array.flatMap((x, index) => [x, index]));
    const sparseArray = [1, , 2, , 3];
    print(sparseArray.flatMap(x => [x]));
    const nestedArray = [[1, 2], [3, 4], [5, 6]];
    print(nestedArray.flatMap(arr => arr));
    const arrayWithEmptyArrays = [1, [], [2, 3], [], 4];
    print(arrayWithEmptyArrays.flatMap(x => x));
    print(array.flatMap(x => x % 2 === 0 ? [x, x * 2] : x));
}

/*
 * @tc.name:ForEach
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
originalArrays.forEach(array => {
    array.forEach(item => {
        print(item);
    });
});

try {
    const array = [1, 2, 3];
    array.forEach('not a function');
} catch (error) {
    print("Caught an error: ", error);
}

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

testCases.forEach(({ array, target }) => {
    const result = array.includes(target);
    print(`Array: [${array.join(', ')}], Target: ${target}, Result: ${result}`);
});

/*
 * @tc.name:IndexOf
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 5];
    print(arr.indexOf(3));
    print(arr.indexOf(1));
    print(arr.indexOf(5));
    print([].indexOf(1));
    let arr2 = ["apple", "banana", "cherry"];
    print(arr2.indexOf("banana"))
    let arr3 = [1, 2, 2, 3, 4, 2];
    print(arr3.indexOf(2));
    print(arr.indexOf(10));
    let arr4 = [{id: 1}, {id: 2}, {id: 3}];
    print(arr4.indexOf({id: 2}));
    print(arr4.findIndex(item => item.id === 2));
    print("not an array".indexOf(1));
}

/*
 * @tc.name:Join
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = ["apple", "banana", "cherry"];
    print(arr.join());
    print(arr.join(", "));
    let emptyArr = [];
    print(emptyArr.join());
    let singleElementArr = ["apple"];
    print(singleElementArr.join());
    let mixedArr = ["apple", 1, {name: "John"}];
    print(mixedArr.join());
    let customSeparatorArr = ["apple", "banana", "cherry"];
    print(customSeparatorArr.join(" + "));
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
        print(key);
    }

    let emptyArr = [];
    let emptyKeysIter = emptyArr.keys();
    print(emptyKeysIter.next().done);

    let singleElementArr = ["apple"];
    let singleElementKeysIter = singleElementArr.keys();
    print(singleElementKeysIter.next().value);
    print(singleElementKeysIter.next().done);

    let multiDimArr = [["apple", "banana"], ["cherry", "date"]];
    let multiDimKeysIter = multiDimArr.keys();
    for (let key of multiDimKeysIter) {
        print(key);
    }

    let sparseArr = [1, , 3];
    let sparseKeysIter = sparseArr.keys();
    for (let key of sparseKeysIter) {
        print(key);
    }
}

/*
 * @tc.name:LastIndexOf
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 2, 5];
    print(arr.lastIndexOf(2));

    print(arr.lastIndexOf(1));
    print(arr.lastIndexOf(5));
    print(arr.lastIndexOf(6));

    let emptyArr = [];
    print(emptyArr.lastIndexOf(1));

    let arrWithNaN = [1, 2, NaN, 4, NaN];
    print(arrWithNaN.lastIndexOf(NaN));

    let arrWithUndefined = [1, 2, undefined, 4];
    print(arrWithUndefined.lastIndexOf(undefined));
}

/*
 * @tc.name:Map
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 5];
    let mappedArr = arr.map(num => num * 2);
    print(mappedArr);

    let emptyArr = [];
    let mappedEmptyArr = emptyArr.map(item => item * 2);
    print(mappedEmptyArr);

    let arrWithNaN = [1, 2, NaN, 4, NaN];
    let mappedArrWithNaN = arrWithNaN.map(num => num * 2);
    print(mappedArrWithNaN);

    let sparseArr = [1, , 3];
    let mappedSparseArr = sparseArr.map(num => num * 2);
    print(mappedSparseArr);

    let objArr = [{id: 1}, {id: 2}, {id: 3}];
    let mappedObjArr = objArr.map(obj => obj.id);
    print(mappedObjArr);

    let multiDimArr = [[1, 2], [3, 4], [5, 6]];
    let mappedMultiDimArr = multiDimArr.map(innerArr => innerArr.map(num => num * 2));
    print(mappedMultiDimArr);
}

/*
 * @tc.name:Pop
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 5];
    let poppedElement = arr.pop();
    print(poppedElement);
    print(arr);

    let emptyArr = [];
    let poppedEmptyElement = emptyArr.pop();
    print(poppedEmptyElement);
    print(emptyArr);

    let singleElementArr = [1];
    let poppedSingleElement = singleElementArr.pop();
    print(poppedSingleElement);
    print(singleElementArr);

    let anotherSingleElementArr = ["apple"];
    let poppedAnotherSingleElement = anotherSingleElementArr.pop();
    print(poppedAnotherSingleElement);
}

/*
 * @tc.name:Push
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3];
    arr.push(4);
    print(arr);

    arr.push(5, 6);
    print(arr);

    let emptyArr = [];
    emptyArr.push(1);
    print(emptyArr);

    let objArr = [{ id: 1 }];
    objArr.push({ id: 2 });
    print(objArr);

    let nestedArr = [1, 2];
    nestedArr.push([3, 4]);
    print(nestedArr);

    let arrWithUndefined = [1, 2, 3];
    arrWithUndefined.push(undefined);
    print(arrWithUndefined);

    let singleElementArr = [1];
    singleElementArr.push(2);
    print(singleElementArr);

    var ve = [''];
    try {
        for (var item in ve) {
            var vee = ve[item];
            Array.prototype.push.call(vee[item],[,,]);
        }
    } catch(e) { print(e); }
}

/*
 * @tc.name:Reduce
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let arr = [1, 2, 3, 4, 5];
    let sum = arr.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
    print(sum);

    let emptyArr = [];
    let sumOfEmptyArr = emptyArr.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
    print(sumOfEmptyArr);

    let singleArr = [1];
    let sumOfSingleArr = singleArr.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
    print(sumOfSingleArr);

    let arrNaN = [1, 2, NaN, 4];
    let sumOfArrNaN = arrNaN.reduce((accumulator, currentValue) => accumulator + currentValue, 0);
    print(sumOfArrNaN);
}

/*
 * @tc.name:ReduceRight
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    let result1 = array1.reduceRight((acc, curr) => acc + curr);
    print(result1);

    let array2 = [];
    let result2 = array2.reduceRight((acc, curr) => acc + curr, "initialValue");
    print(result2);

    let array3 = ["a"];
    let result3 = array3.reduceRight((acc, curr) => acc + curr);
    print(result3);

    let array4 = ["a", "b", undefined, "d"];
    let result4 = array4.reduceRight((acc, curr) => acc + curr);
    print(result4);
}

/*
 * @tc.name:Reverse
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    array1.reverse();
    print(array1);

    let emptyArray = [];
    emptyArray.reverse();
    print(emptyArray);

    let singleElementArray = ["a"];
    singleElementArray.reverse();
    print(singleElementArray);

    let arrayWithUndefined = ["a", "b", undefined, "d"];
    arrayWithUndefined.reverse();
    print(arrayWithUndefined);
}

/*
 * @tc.name:Shift
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    let shiftedElement1 = array1.shift();
    print(shiftedElement1);
    print(array1);

    let emptyArray = [];
    let shiftedElement2 = emptyArray.shift();
    print(shiftedElement2);
    print(emptyArray);

    let singleElementArray = ["a"];
    let shiftedElement3 = singleElementArray.shift();
    print(shiftedElement3);
    print(singleElementArray);

    let arrayWithUndefined = ["a", undefined, "b", "c"];
    let shiftedElement4 = arrayWithUndefined.shift();
    print(shiftedElement4);
    print(arrayWithUndefined);
}

/*
 * @tc.name:Slice
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    let slicedArray1 = array1.slice(1, 3);
    print(slicedArray1);

    let emptyArray = [];
    let slicedEmptyArray = emptyArray.slice(1, 3);
    print(slicedEmptyArray);

    let singleElementArray = ["a"];
    let slicedSingleElementArray = singleElementArray.slice(0, 1);
    print(slicedSingleElementArray);

    let arrayWithUndefined = ["a", undefined, "b", "c"];
    let slicedArrayWithUndefined = arrayWithUndefined.slice(1, 3);
    print(slicedArrayWithUndefined);
}

/*
 * @tc.name:Some
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = [1, 2, 3, 4, 5];
    let isSomeEven = array1.some(num => num % 2 === 0);
    print(isSomeEven);

    let array2 = [1, 3, 5, 7, 9];
    let isSomeEven2 = array2.some(num => num % 2 === 0);
    print(isSomeEven2);

    let emptyArray = [];
    let isSomeEmpty = emptyArray.some(num => num > 0);
    print(isSomeEmpty);

    let singleElementArray = [1];
    let isSomeSingleElement = singleElementArray.some(num => num > 0);
    print(isSomeSingleElement);

    let arrayWithUndefined = [1, undefined, 3, 5];
    let isSomeUndefined = arrayWithUndefined.some(num => num === undefined);
    print(isSomeUndefined);
}

/*
 * @tc.name:Sort
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = [3, 1, 4, 1, 5, 9, 2, 6, 5];
    array1.sort();
    print(array1);

    let emptyArray = [];
    emptyArray.sort();
    print(emptyArray);

    let singleElementArray = [1];
    singleElementArray.sort();
    print(singleElementArray);

    let arrayWithUndefined = [1, undefined, 3, 5];
    arrayWithUndefined.sort();
    print(arrayWithUndefined);

    let arrayWithStrings = ["banana", "apple", "cherry"];
    arrayWithStrings.sort();
    print(arrayWithStrings);
}

/*
 * @tc.name:Splice
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    let removedElements1 = array1.splice(2, 2, "x", "y");
    print(removedElements1);
    print(array1);

    let emptyArray = [];
    let removedElements2 = emptyArray.splice(0, 0, "x", "y");
    print(removedElements2);
    print(emptyArray);

    let singleElementArray = ["a"];
    let removedElements3 = singleElementArray.splice(0, 1, "x", "y");
    print(removedElements3);
    print(singleElementArray);

    let arrayWithUndefined = [1, undefined, 3, 5];
    let removedElements4 = arrayWithUndefined.splice(1, 2);
    print(removedElements4);
    print(arrayWithUndefined);
}

/*
 * @tc.name:toString
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array = ["apple", "banana", "cherry"];
    let string = array.toString();
    print(string);

    let numbers = [1, 2, 3, 4, 5];
    let string2 = numbers.toString();
    print(string2);

    let mixed = [1, "two", true];
    let string3 = mixed.toString();
    print(string3);
}

/*
 * @tc.name:Unshift
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c"];
    let newLength1 = array1.unshift("x", "y");
    print(newLength1);
    print(array1);

    let emptyArray = [];
    let newLength2 = emptyArray.unshift("x", "y");
    print(newLength2);
    print(emptyArray);

    let singleElementArray = ["a"];
    let newLength3 = singleElementArray.unshift("x");
    print(newLength3);
    print(singleElementArray);

    let arrayWithUndefined = [1, 2, undefined];
    let newLength4 = arrayWithUndefined.unshift("x");
    print(newLength4);
    print(arrayWithUndefined);

    let arrayWithHole = [1, 2, ,4];
    let newLength5 = arrayWithHole.unshift(5, 6, 7);
    print(newLength5);
    print(arrayWithHole);

    let arrayWithHole1 = [1, 2, ,4];
    let newLength6 = arrayWithHole1.unshift(5, 6, 7, 8);
    print(newLength6);
    print(arrayWithHole1);
}

/*
 * @tc.name:ToReversed ToSorted ToSpliced With
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    let array1 = ["a", "b", "c", "d", "e"];
    print(array1.toReversed());

    let emptyArray = [];
    print(emptyArray.toReversed());

    let singleElementArray = ["a"];
    print(singleElementArray.toReversed());

    let arrayWithUndefined = ["a", "b", undefined, "d"];
    print(arrayWithUndefined.toReversed());
}

{
    let array1 = [3, 1, 4, 1, 5, 9, 2, 6, 5];
    print(array1.toSorted());

    let emptyArray = [];
    print(emptyArray.toSorted());

    let singleElementArray = [1];
    print(singleElementArray.toSorted());

    let arrayWithUndefined = [1, undefined, 3, 5];
    print(arrayWithUndefined.toSorted());

    let arrayWithStrings = ["banana", "apple", "cherry"];
    print(arrayWithStrings.toSorted());
}

{
    let array1 = ["a", "b", "c", "d", "e"];
    let removedElements1 = array1.toSpliced(2, 2, "x", "y");
    print(removedElements1);
    print(array1);

    let emptyArray = [];
    let removedElements2 = emptyArray.toSpliced(0, 0, "x", "y");
    print(removedElements2);
    print(emptyArray);

    let singleElementArray = ["a"];
    let removedElements3 = singleElementArray.toSpliced(0, 1, "x", "y");
    print(removedElements3);
    print(singleElementArray);

    let arrayWithUndefined = [1, undefined, 3, 5];
    let removedElements4 = arrayWithUndefined.toSpliced(1, 2);
    print(removedElements4);
    print(arrayWithUndefined);
}

/*
 * @tc.name:IsArray
 * @tc.desc:test Array
 * @tc.type: FUNC
 */
{
    // print true
    print(Array.isArray([]));
    print(Array.isArray([1]));
    print(Array.isArray(new Array()));
    print(Array.isArray(new Array("a", "b", "c", "d")));
    print(Array.isArray(new Array(3)));
    print(Array.isArray(Array.prototype));

    // print false
    print(Array.isArray());
    print(Array.isArray({}));
    print(Array.isArray(null));
    print(Array.isArray(undefined));
    print(Array.isArray(17));
    print(Array.isArray("Array"));
    print(Array.isArray(true));
    print(Array.isArray(false));
    print(Array.isArray(new Uint8Array(32)));
    print(Array.isArray({ __proto__: Array.prototype }));
}

var arr_push = [];
Object.defineProperty(arr_push, "length", { writable : false});
try {
    arr_push.push(3);
} catch (e) {
    print(e instanceof TypeError);
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
print(findIndexArray.findIndex(element => element == obj1));
print(findIndexArray.findIndex(element => element == { ["obj3"]: 3 }));

// find index bigint
findIndexArray = new Array(4);
findIndexArray[0] = 1n;
findIndexArray[1] = 2n;
findIndexArray[2] = 3n;
findIndexArray[3] = 4n;
print(findIndexArray.findIndex(element => element == 2n));
print(findIndexArray.findIndex(element => element == 0n));


// find index string
findIndexArray = new Array(4);
findIndexArray[0] = "a";
findIndexArray[1] = "b";
findIndexArray[2] = "c";
findIndexArray[3] = "d";
print(findIndexArray.findIndex(element => element == "b"));
print(findIndexArray.findIndex(element => element == "e"));

{
    Array.prototype.__proto__ = null;
    var vp = Array.prototype;
    vp.map(function(vp) {});
    vp.filter(function(vp) {});
    print("succ");
    Array.prototype.__proto__ = Object.prototype;
}
// slice
let sliceArray = [0, 1, 2, 3, 4, 5];
print(sliceArray.slice(0));
print(sliceArray.slice(-1));
print(sliceArray.slice(1, 4));
print(sliceArray.slice(1, 10));

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
    
    try {
        iterator.next()
    } catch {
        print("Exception")
    }
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
        print(result.value, result.done);
    }
    print(result.value, result.done);
  
    // Keys Iterator
    const keysIterator = Array.prototype.keys.call(arrayLike);
    while (!(result = keysIterator.next()).done) {
        print(result.value, result.done);
    }
    print(result.value, result.done);
  
    // Entries Iterator
    const entriesIterator = Array.prototype.entries.call(arrayLike);
    while (!(result = entriesIterator.next()).done) {
        print(result.value, result.done);
    }
    print(result.value, result.done);
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
            print('Test passed');
        } else {
            print('Test failed');
        }
    }      
}


let x1 = new Array(10);
let x2 = new Array(1,2,3,undefined);
let x3 = new Array();

x1.constructor = null

print(ArkTools.hasConstructor(x2));
print(ArkTools.hasConstructor(x3));