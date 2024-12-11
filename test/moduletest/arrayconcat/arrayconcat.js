/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
const array1 = ['a', 'b', 'c'];
const array2 = ['d', 'e', 'f'];
const array3 = array1.concat(array2);
print(array3);

const letters = ["a", "b", "c"];
const numbers = [1, 2, 3];

const alphaNumeric = letters.concat(numbers);
print(alphaNumeric);

const num1 = [1, 2, 3];
const num2 = [4, 5, 6];
const num3 = [7, 8, 9];

const numbers1 = num1.concat(num2, num3);

print(numbers1);


const letters1 = ["a", "b", "c"];

const alphaNumeric1 = letters1.concat(1, [2, 3]);

print(alphaNumeric1);

const num11 = [[1]];
const num22 = [2, [3]];

const numbers2 = num1.concat(num22);

print(numbers2);
// [[1], 2, [3]]

num11[0].push(4);

print(numbers2);

print([1, , 3].concat([4, 5])); // [1, empty, 3, 4, 5]
print([1, 2].concat([3, , 5])); // [1, 2, 3, empty, 5]

const emptyArr = [];
print(emptyArr.concat([]).length);

{
    let v1 = Math.pow(2, 31);
    let v2 = [];
    v2[v1] = 31;
    let v3 = [];
    let v4 = v2.concat(v3);
    let v5 = v3.concat(1, -24323, v1, v4);
    print(v5.length);
}

/*
 * @tc.name:Array concat1
 * @tc.desc:test long Array Concat long Array and change proto
 * @tc.type: FUNC
 */
{
    let arr1 = new Array(2000);
    arr1[0] = 1;
    arr1[1] = 1;
    let arr2 = new Array(2000);
    arr2.__proto__ = arr1;
    arr2[1] = 3;
    arr2[2] = 2;
    arr2[3] = "ab";
    arr2[4] = 1;
    let arr3 = new Array(2000);
    let arr4 = arr3.concat(arr2);
    print(arr4[2000], arr4[2001], arr4[2002], arr4[2003], arr4[2004]);
}

/*
 * @tc.name:Array concat2
 * @tc.desc:test long Array Concat short Array and change proto
 * @tc.type: FUNC
 */
{
    let arr1 = new Array(10);
    arr1[0] = 1;
    arr1[1] = 1;
    let arr2 = new Array(10);
    arr2.__proto__ = arr1;
    arr2[1] = 3;
    arr2[2] = 2;
    arr2[3] = "ab";
    arr2[4] = 1;
    let arr3 = new Array(2000);
    let arr4 = arr3.concat(arr2);
    print(arr4[2000], arr4[2001], arr4[2002], arr4[2003], arr4[2004]);
}

/*
 * @tc.name:Array concat3
 * @tc.desc:test short Array Concat long Array and change proto
 * @tc.type: FUNC
 */
{
    let arr1 = new Array(2000);
    arr1[0] = 1;
    arr1[1] = 1;
    let arr2 = new Array(2000);
    arr2.__proto__ = arr1;
    arr2[1] = 3;
    arr2[2] = 2;
    arr2[3] = "ab";
    arr2[4] = 1;
    let arr3 = new Array(10);
    let arr4 = arr3.concat(arr2);
    print(arr4[10], arr4[11], arr4[12], arr4[13], arr4[14]);
}

/*
 * @tc.name:Array concat4
 * @tc.desc:test long Array Concat long Array
 * @tc.type: FUNC
 */
{
    let arr2 = new Array(2000);
    arr2[1] = 3;
    arr2[2] = 2;
    arr2[3] = "ab";
    arr2[4] = 1;
    let arr3 = new Array(2000);
    let arr4 = arr3.concat(arr2);
    print(arr4[2000], arr4[2001], arr4[2002], arr4[2003], arr4[2004]);
}

/*
 * @tc.name:Array concat5
 * @tc.desc:test long Array Concat short Array
 * @tc.type: FUNC
 */
{
    let arr2 = new Array(10);
    arr2[1] = 3;
    arr2[2] = 2;
    arr2[3] = "ab";
    arr2[4] = 1;
    let arr3 = new Array(2000);
    let arr4 = arr3.concat(arr2);
    print(arr4[2000], arr4[2001], arr4[2002], arr4[2003], arr4[2004]);
}

/*
 * @tc.name:Array concat6
 * @tc.desc:test short Array Concat long Array
 * @tc.type: FUNC
 */
{
    let arr2 = new Array(2000);
    arr2[1] = 3;
    arr2[2] = 2;
    arr2[3] = "ab";
    arr2[4] = 1;
    let arr3 = new Array(10);
    let arr4 = arr3.concat(arr2);
    print(arr4[10], arr4[11], arr4[12], arr4[13], arr4[14]);
}

/*
 * @tc.name:Array concat7
 * @tc.desc:test long Array Concat long Array change origin prototype
 * @tc.type: FUNC
 */
{
    let arr2 = new Array(2000);
    arr2.__proto__[0] = "??";
    arr2[1] = 3;
    arr2[2] = 2;
    arr2[3] = "ab";
    arr2[4] = 1;
    let arr3 = new Array(2000);
    let arr4 = arr3.concat(arr2);
    print(arr4[0], arr4[2000], arr4[2001], arr4[2002], arr4[2003], arr4[2004]);
}

print("test arrayconcat success!");