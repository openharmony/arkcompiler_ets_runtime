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
 * @tc.name:arrayjoin
 * @tc.desc:test Array.join
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var a = new Array(1).join("  ");
print(a.length);
var str1 = JSON.stringify(Array(3).join("0"));
print(str1);
var str2 = JSON.stringify(new Array(3).join("0"));
print(str2);
const arr = []
arr.length = 3
var str3 = JSON.stringify(arr.join("0"));
print(str3)

// test circular reference
var arr1 = [1];
arr1.push(arr1);
arr1.push(arr1);
print(arr1.toString());
print(arr1.toString());

var arr2 = [1];
var arr3 = [2];
arr2[10] = arr3;
arr3[10] = arr2;
print(arr2.toString());
print(arr2.toString());

var arr4 = [1];
var arr5 = [2];
var arr6 = [3];
arr4.push(arr5);
arr5.push(arr6);
arr6.push(arr4);
print(arr4.toString());
print(arr4.toString());

var arr7 = [
    {
        toLocaleString() {
            return [1, arr7];
        }
    }
];
print(arr7.toLocaleString());

var aa = this;
var bb = {};
aa.length = 4294967296; // 2 ^ 32 (max array length + 1)
try {
    Array.prototype.join.call(aa,bb)
} catch (e) {
    print(e instanceof TypeError);
}

try {
    Object.getOwnPropertyDescriptors(Array(1e9).join('c'))
} catch (e) {
    print(e instanceof RangeError);
}

([11])["join"]('쏄');

let proxy1 = new Proxy([123], {});
proxy1.pop();
proxy1.toString();
proxy1.push(456);
print(`proxy1: ${proxy1}`);

let proxy2 = new Proxy([123, 456], {});
proxy2.pop();
proxy2.toString();
proxy2.push(456);
print(`proxy2: ${proxy2}`);

const v5 = new Float32Array(1);
v5[0] = NaN;
print(v5.join(String.fromCodePoint(0)));

const v6 = new Float32Array(1);
v6[0] = NaN;
v6[1] = NaN;
print(v6.join(String.fromCodePoint(0)));

const v7 = new Float32Array(2);
v7[0] = NaN;
print(v7.join(String.fromCodePoint(0)));

const element = {
    toString() {
        Array.prototype[1] = 'b';
        return 'a';
    }
};
const arr_join = [element, ,'c'];
print("abc" == arr_join.join(''));

const v9 = new Int16Array(128);
const v10 = new Int8Array(128);
v9[11] = [v9, v10];
print(v9[11]);