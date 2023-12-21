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

const months = ['March', 'Jan', 'Feb', 'Dec'];
months.sort();
print(months);

const array1 = [1, 30, 4, 21, 100000];
array1.sort();
print(array1);
const numberArray1 = new Array(40, 1, 5, 200);
    let res1 = numberArray1.sort();
print(res1);

const stringArray = ["Blue", "Humpback", "Beluga"];
const numberArray = [40, 1, 5, 200];
const numericStringArray = ["80", "9", "700"];
const mixedNumericArray = ["80", "9", "700", 40, 1, 5, 200];

function compareNumbers(a, b) {
  return a - b;
}

stringArray.join(); // 'Blue,Humpback,Beluga'
stringArray.sort(); // ['Beluga', 'Blue', 'Humpback']
print(stringArray)

numberArray.join(); // '40,1,5,200'
numberArray.sort(); // [1, 200, 40, 5]
numberArray.sort(compareNumbers); // [1, 5, 40, 200]
print(numberArray)

numericStringArray.join(); // '80,9,700'
numericStringArray.sort(); // ['700', '80', '9']
numericStringArray.sort(compareNumbers); // ['9', '80', '700']
print(numericStringArray)

mixedNumericArray.join(); // '80,9,700,40,1,5,200'
mixedNumericArray.sort(); // [1, 200, 40, 5, '700', '80', '9']
mixedNumericArray.sort(compareNumbers); // [1, 5, '9', 40, '80', 200, '700']
print(mixedNumericArray)

print(["a", "c", , "b"].sort()); // ['a', 'b', 'c', empty]
print([, undefined, "a", "b"].sort()); // ["a", "b", undefined, empty]

var items = ["réservé", "premier", "cliché", "communiqué", "café", "adieu"];
items.sort(function (a, b) {
  return a.localeCompare(b);
});
print(items);

const numbers1 = [3, 1, 4, 1, 5];
const sorted1 = numbers1.sort((a, b) => a - b);
sorted1[0] = 10;
print(numbers1[0]); // 10

const numbers = [3, 1, 4, 1, 5];
const sorted = [...numbers].sort((a, b) => a - b);
sorted[0] = 10;
print(numbers[0]); // 3

const arr1 = [3, 1, 4, 1, 5, 9];
const compareFn = (a, b) => (a > b ? 1 : 0);
arr1.sort(compareFn);
print(arr1);

const arr = [3, 1, 4, 1, 5, 9];
const compareFn1 = (a, b) => (a > b ? -1 : 0);
arr.sort(compareFn1);
print(arr); 

print(["a", "c", , "b"].sort()); // ['a', 'b', 'c', empty]
print([, undefined, "a", "b"].sort()); // ["a", "b", undefined, empty]
