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


const array1 = [5, 12, 8, 130, 44];
const found = array1.find((element) => element > 10);
console.log(found);

const arrayLike = {
	  length: 3,
	  0: 2,
	  1: 7.3,
	  2: 4,
};
console.log(Array.prototype.find.call(arrayLike, (x) => !Number.isInteger(x)));

const array = [0, 1, , , , 5, 6];

array.find((value, index) => {
	  console.log(`${index},${value}`);
});

array.find((value, index) => {
	  if (index === 0) {
		      console.log(`array[5]${array[5]}`);
		      delete array[5];
		    }
	  console.log(`${index},${value}`);
});

function isPrime(element, index, array) {
	  let start = 2;
	  while (start <= Math.sqrt(element)) {
		      if (element % start++ < 1) {
			            return false;
			          }
		    }
	  return element > 1;
}

console.log([4, 6, 8, 12].find(isPrime)); 
console.log([4, 5, 8, 12].find(isPrime)); 
//
const numbers = [3, -1, 1, 4, 1, 5, 9, 2, 6];
const firstTrough = numbers
  .filter((num) => num > 0)
  .find((num, idx, arr) => {
    if (idx > 0 && num >= arr[idx - 1]) return false;
    if (idx < arr.length - 1 && num >= arr[idx + 1]) return false;
    return true;
  });
console.log(firstTrough); // 1

const words = ["spray", "limit", "limits"];
const deleteWords = words.find((word, index, arr) => {
  arr.length=4
  word="asd"
  return word == "asd"
});
console.log(deleteWords);
console.log(words.length);

