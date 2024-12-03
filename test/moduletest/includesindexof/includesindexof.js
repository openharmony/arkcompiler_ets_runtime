/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function AssertTrue(value) {
    if (!value) {
      throw new Error('Assertion failed: expected true');
    }
}

function AssertFalse(value) {
    if (value) {
      throw new Error('Assertion failed: expected false');
    }
}

let array1 = [0,,2,,4,,];
let array2 = [0.5,,2.5,,NaN,,];
let array3 = new Array(7);
array3[1] = 1;
array3[4] = 5.588;
array3[6] = NaN;
let array4 = new Array(5);
array4[1] = 1;
array4[2] = "asc";
array4[3] = NaN;
array4[4] = 9.0;
AssertTrue(array1.includes(0));
AssertTrue(array1.includes(4));
AssertTrue(array1.includes());
AssertTrue(array2.includes(0.5));
AssertTrue(array2.includes(2.5));
AssertTrue(array2.includes(NaN));
AssertTrue(array2.includes(undefined));
AssertTrue(array3.includes(1));
AssertTrue(array3.includes(5.588));
AssertTrue(array3.includes(NaN));
AssertTrue(array3.includes());
AssertTrue(array4.includes(1));
AssertTrue(array4.includes("asc"));
AssertTrue(array4.includes(NaN));
AssertTrue(array4.includes(9.0));


var sample = [42.0000000, 0, 1, NaN];
AssertTrue(sample.includes(-0));
AssertTrue(sample.includes(42));
print(sample.indexOf(-0));
print(sample.indexOf(42));

var toStrObj = {
    toString: function() {
        return 0
    }
};
var one = 1;
var _float = -(4 / 3);
var a = new Array(false, undefined, null, "0", toStrObj, -1.3333333333333, "str", -0, true, +0, one, 1, 0, false, _float, -(4 / 3));
print(a.indexOf(0));
print([-0].indexOf(+0));

AssertTrue([1, 2, 3].includes(2));
AssertFalse([1, 2, 3].includes(4));
AssertTrue([1, 2, NaN].includes(NaN));
AssertTrue([1, 2, -0].includes(+0));
AssertTrue([1, 2, -0].includes(-0));
AssertTrue([1, 2, +0].includes(-0));
AssertTrue([1, 2, +0].includes(+0));
AssertFalse([1, 2, -Infinity].includes(+Infinity));
AssertTrue([1, 2, -Infinity].includes(-Infinity));
AssertFalse([1, 2, +Infinity].includes(-Infinity));
AssertTrue([1, 2, +Infinity].includes(+Infinity));
AssertFalse(["a", "b", "c"].includes({}));
AssertFalse([{}, {}].includes({}));
var obj = {};
AssertTrue([obj].includes(obj));
AssertFalse([obj].includes(obj, 1));
AssertTrue([obj, obj].includes(obj, 1));
AssertTrue([,,,].includes(undefined));
AssertTrue([12, 13].includes(13, -1));
AssertFalse([12, 13].includes(12, -1));
AssertTrue([12, 13].includes(12, -2));


const arr3 = [true, false, 0, 1];
print(arr3.indexOf(true) === 0);
print(arr3.indexOf(false) === 1);

const arr4 = [NaN, 1, 2, 3];
print(arr4.indexOf(NaN) === 0);

const arr5 = [undefined, 1, 2, 3];
print(arr5.indexOf(undefined) === 0);
print(arr5.indexOf(null) === 0);

const arr7 = [];
print(arr7.indexOf(1) === -1);

const arr9 = [1, 2, 3, 1, 4];
print(arr9.indexOf(1) === 0,);

const arr10 = [{ id: 1 }, { id: 2 }];
const obj1 = { id: 2 };
print(arr10.indexOf(obj1) === -1);

const arr11 = [{ id: 1 }, { id: 2 }];
const obj2 = { id: 2 };
print(arr11.indexOf({ id: 2 }) === -1);

const arr12 = [undefined, 1, 2, 3];
print(arr12.indexOf(undefined) === 0);

const arr13 = [null, 1, 2, 3];
print(arr13.indexOf(null) === 0);

const arr14 = [1, 2, 3, 4, 5];
print(arr14.indexOf(3, 2) === 2);
print(arr14.indexOf(1, 2) === -1);

const largeArr = new Array(1000).fill(0).concat(1);
print(largeArr.indexOf(1) === 1000);

const arr15 = [0, '0', false];
print(arr15.indexOf('0') === 1);
print(arr15.indexOf(0) === 0);
print(arr15.indexOf(false) === 2);

const arr16 = ['', 'apple', 'banana'];
print(arr16.indexOf('') === 0);

const arr17 = ['', ,];
print(arr17.indexOf(undefined) === 0);
print(arr17.indexOf() === 0);

