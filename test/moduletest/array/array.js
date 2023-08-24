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

var arr1 = [1,1,1,1,1,1];
arr1.fill(0, 2, 4);
print(arr1);

var arr2 = new Array(100);
arr2.fill(0, 2, 4);
var a1 = arr2.slice(0, 5);
print(arr2.length);
print(a1);

var arr3 = [1,2,3,4,5,6];
arr3.pop();
print(arr3.length);
print(arr3);

var arr4 = [1,3,4];
arr4.splice(1, 0, 2);
print(arr4.length);
print(arr4);
// 1, 2, 3, 4
var arr4 = [1,2,3,4];
arr4.splice(3, 1, 3);
print(arr4.length);
print(arr4);
// 1, 2, 3, 3

var arr5 = [1,2,3,4,5,6];
arr5.shift();
print(arr5.length);
print(arr5);
// 1, 2, 3, 4, 5

var arr6 = [1,2,3,4,5];
arr6.reverse();
print(arr6);

var arr7 = [];
arr7[2] = 10;
print(arr7.pop());  // 10
print(arr7.pop());  // undefined
print(arr7.pop());  // undefined

var arr8 = [];
arr8[1] = 8;
print(arr8.shift());  // undefined
print(arr8.shift());  // 8
print(arr8.shift());  // undefined

// Test on Array::Splice
var arr9 = new Array(9);
print(arr9.length); // 9
arr9.splice(0, 9);
print(arr9.length); // 0
for(let i = 0; i < arr9.length; i++) {
    print(arr9[i]);
}

var arr10 = new Array(9);
print(arr10.length); // 9
arr10.splice(0, 8, 1);
print(arr10.length); // 2
for(let i = 0; i < arr10.length; i++) {
    print(arr10[i]); // 1, undefined
}

var arr11 = new Array(9);
print(arr11.length); // 9
arr11.splice(1, 8, 1);
print(arr11.length); // 2
for(let i = 0; i < arr11.length; i++) {
    print(arr11[i]); // undefined, 1
}

var arr12 = new Array(9);
print(arr12.length); // 9
arr12.splice(0, 4, 1, 2, 3, 4, 5);
print(arr12.length); // 10
for(let i = 0; i < arr12.length; i++) {
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

var arr21 = [1,2,3,4,,6];
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
const sortedValues = values.toSorted((a, b) => {return a- b});
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
    print(`arr0.reverse() === arr0 ? ${arr0.reverse() === arr0}`);  // true
    print(`arr0.length after reverse() called = ${arr0.length}`);   // 0

    const arr1 = [1];
    print(`arr1.reverse() === arr1 ? ${arr1.reverse() === arr1}`);  // true
    print(`arr1 after reverse() called = ${arr1}`);                 // 1

    const arrWithHoles = [];
    arrWithHoles[1] = 1;
    arrWithHoles[4] = 4;
    arrWithHoles[6] = undefined;
    arrWithHoles.length = 10;
    // arrWithHoles = [Hole, 1, Hole, Hole, 4, Hole, undefined, Hole, Hole, Hole]
    print(`arrWithHoles before reverse() called = ${arrWithHoles}`);            // ,1,,,4,,,,,
    print(`arrWithHoles.reverse()               = ${arrWithHoles.reverse()}`);  // ,,,,,4,,,1,
    print(`arrWithHoles after reverse() called  = ${arrWithHoles}`);            // ,,,,,4,,,1,
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
            arr.indexOf(40),                        // 4
            arr.indexOf(40, 5),                     // -1
            arr.indexOf(10),                        // 1
            arr.indexOf(10, 2),                     // 7
            arr.lastIndexOf(40),                    // 4
            arr.lastIndexOf(40, 3),                 // -1
            arr.lastIndexOf(10),                    // 7
            arr.lastIndexOf(10, 6),                 // 1
        ],
        "Group 2: -arr.length <= fromIndex < 0", [
            arr.indexOf(40, -4),                    // -1
            arr.indexOf(40, -8),                    // 4
            arr.indexOf(10, -4),                    // 7
            arr.indexOf(10, -10),                   // 1
            arr.lastIndexOf(40, -4),                // 4
            arr.lastIndexOf(40, -8),                // -1
            arr.lastIndexOf(10, -4),                // 1
            arr.lastIndexOf(10, -10),               // -1
            arr.indexOf(0, -arr.length),            // 0
            arr.indexOf(10, -arr.length),           // 1
            arr.lastIndexOf(0, -arr.length),        // 0
            arr.lastIndexOf(10, -arr.length),       // -1
        ],
        "Group 3: fromIndex >= arr.length", [
            arr.indexOf(0, arr.length),             // -1
            arr.indexOf(0, arr.length + 10),        // -1
            arr.indexOf(10, arr.length),
            arr.indexOf(10, arr.length + 10),
            arr.lastIndexOf(0, arr.length),         // 0
            arr.lastIndexOf(0, arr.length + 10),    // 0
        ],
        "Group 4: fromIndex < -arr.length", [
            arr.indexOf(0, -arr.length - 10),       // 0
            arr.lastIndexOf(0, -arr.length - 10)    // -1
        ],
        "Group 5: fromIndex in [Infinity, -Infinity]", [
            arr.indexOf(10, -Infinity),             // 1
            arr.indexOf(10, +Infinity),             // -1
            arr.lastIndexOf(10, -Infinity),         // -1
            arr.lastIndexOf(10, +Infinity)          // 7
        ],
        "Group 6: fromIndex is NaN", [
            arr.indexOf(0, NaN),                    // 0
            arr.indexOf(10, NaN),                   // 1
            arr.lastIndexOf(0, NaN),                // 0
            arr.lastIndexOf(10, NaN),               // -1
        ],
        "Group 7: fromIndex is not of type 'number'", [
            arr.indexOf(10, '2'),                           // 7
            arr.lastIndexOf(10, '2'),                       // 1
            arr.indexOf(10, { valueOf() { return 3; } }),   // 7
            arr.indexOf(10, { valueOf() { return 3; } }),   // 1
        ],
        "Group 8: Strict equality checking", [
            arr.indexOf("80"),                      // 8
            arr.lastIndexOf(80),                    // 3
        ],
        "Group 9: Searching undefined and null", [
            arr.indexOf(),                          // 6
            arr.indexOf(undefined),                 // 6
            arr.indexOf(null),                      // -1
            arr.lastIndexOf(),                      // 6
            arr.lastIndexOf(undefined),             // 6
            arr.lastIndexOf(null)                   // -1
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
            arr2.length = 2;    // Side effects to arr2
            return 0;
        }
    });                         // Expected: -1 (with accessCount = 6)
    print(`  - indexOf:     result = ${result2}, accessCount = ${accessCount}`);
    // Details on why accessCount = 6 can be seen in ECMAScript specifications:
    // https://tc39.es/ecma262/multipage/indexed-collections.html#sec-array.prototype.lastindexof
    accessCount = 0;
    const arr3 = new Proxy([15, 25, 35, 45, 55, 65], arrProxyHandler);
    const result3 = arr3.lastIndexOf(45, {
        valueOf() {
            arr3.length = 2;    // Side effects to arr3
            return 5;
        }
    });                         // Expected: -1 (with accessCount = 6)
    print(`  - lastIndexOf: result = ${result3}, accesscount = ${accessCount}`);

    print("Group 11: fromIndex that triggers exceptions:");
    for (let [prompt, fromIndex] of [ ["bigint", 1n], ["symbol", Symbol()] ]) {
        for (let method of [ Array.prototype.indexOf, Array.prototype.lastIndexOf ]) {
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
