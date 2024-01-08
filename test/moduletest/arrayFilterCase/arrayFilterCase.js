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

const words = ['spray', 'elite', 'exuberant', 'destruction', 'present'];

const result = words.filter((word) => word.length > 6);

print(result);
let wordss = ["spray", "limit", "exuberant", "destruction", "elite", "present"];

const modifiedWords = wordss.filter((word, index, arr) => {
	  arr[index + 1] += " extra";
	    return word.length < 6;
});

print(modifiedWords);

const words1 = ["spray", "limit", "exuberant", "destruction", "elite", "present"];
const appendedWords = words1.filter((word, index, arr) => {
	  arr.push("new");
	    return word.length < 6;
});
print(appendedWords);
const words2 = ["spray", "limit", "exuberant", "destruction", "elite", "present"];
const deleteWords = words2.filter((word, index, arr) => {
	  arr.pop();
	    return word.length < 6;
});

print(deleteWords);

const arrayLike = {
	  length: 3,
	    0: "a",
	    1: "b",
	    2: "c",
};
print(Array.prototype.filter.call(arrayLike, (x) => x <= "b"));

const fruits = ["apple", "banana", "grapes", "mango", "orange"];
function filterItems(arr, query) {
	  return arr.filter((el) => el.toLowerCase().includes(query.toLowerCase()));
}

print(filterItems(fruits, "ap")); // ['apple', 'grapes']
print(filterItems(fruits, "an")); // ['banana', 'mango', 'orange']

const array = [-3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13];

function isPrime(num) {
	  for (let i = 2; num > i; i++) {
		  if (num % i === 0) {
			  return false;
		  }
	  }
	return num > 1;
}

print(array.filter(isPrime)); // [2, 3, 5, 7, 11, 13]

var array1=[,]
function filter(){
    return array1.filter(v => v>0 );
}
array1.__proto__.push(6);
var narr = filter();
print(JSON.stringify(Object.getOwnPropertyDescriptor(narr,0)));

var bPar = true;
var bCalled = false;

function callbackfn(val, idx, obj)
{
  bCalled = true;
  if (obj[idx] !== val)
    bPar = false;
}

var srcArr = [0, 1, true, null, new Object(), "five"];
srcArr[9999999] = -6.6;
var resArr = srcArr.filter(callbackfn);


print(bCalled);
print(bPar);

const emptyArr = [];
print(emptyArr.filter(isPrime).length);
