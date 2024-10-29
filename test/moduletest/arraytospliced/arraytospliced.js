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
 * @tc.name:arraytospliced
 * @tc.desc:test Array.toSpliced
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */

const arr0 = [1, , 3, 4, , 6, undefined];
print(arr0.indexOf(undefined));
const arr1 = arr0.toSpliced(1, 2);
print(arr0.indexOf(undefined));
print(arr1.indexOf(undefined));

const arr2 = new Array(1025);
for(let i = 0; i < 1025; i = i + 1)
    arr2[i] = i;
const arr3 = arr2.toSpliced(0, 0);

var arr4 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
var arr5 = new Array();
for (let i = 0; i < 10; i++) arr5[i] = i;

var result1 = arr4.toSpliced(0, 2, 6);
print(result1);
var result2 = arr5.toSpliced(0, 2, 6);
print(result2);

let outputs = []
// 用例 1: 字符串、数字混合数组
const mixedArray = [1, "two", 3, "four"];
const newArray1 = mixedArray.toSpliced(1, 2, "new", 5);
outputs.push(newArray1);

// 用例 2: 插入布尔值、对象和函数
const complexArray = [true, { key: "value" }, () => "hello"];
const newArray2 = complexArray.toSpliced(2, 0, false, { anotherKey: 42 }, () => "world");
outputs.push(newArray2);

// 用例 3: 替换 undefined 和 null
const arrayWithUndefinedNull = [undefined, null, 3, 4];
const newArray3 = arrayWithUndefinedNull.toSpliced(0, 2, "replaced");
outputs.push(newArray3);

// 用例 4: 处理稀疏数组
const sparseArray = [1, , 3, , 5];
const newArray4 = sparseArray.toSpliced(1, 1, "new");
outputs.push(newArray4);
if (newArray4[3] !== undefined) {
    print("fail: hole should be replaced by undefined");
}

// 用例 5: 在空数组中添加元素
const emptyArray = [];
const newArray5 = emptyArray.toSpliced(0, 0, "first", 2, "third");
outputs.push(newArray5);

// 用例 6: 带有符号类型和 Symbol 类型的数组
const arrayWithSpecialTypes = [Symbol("id"), BigInt(123), "hello"];
const newArray6 = arrayWithSpecialTypes.toSpliced(1, 1, "new value");
outputs.push(newArray6);

// 用例 7: 从数组中移除所有元素
const fullArray = [1, 2, 3, 4];
const emptyArray2 = fullArray.toSpliced(0, fullArray.length);
outputs.push(emptyArray2);

print(outputs.map(out => JSON.stringify(out)).join('\n'));
