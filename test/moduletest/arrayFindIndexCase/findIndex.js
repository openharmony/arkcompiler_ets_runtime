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

/*
 * @tc.name:findIndex
 * @tc.desc:test array.findIndex
 * @tc.type: FUNC
 * @tc.require:
 */

const array1 = [5, 12, 8, 130, 44];
const isLargeNumber = (element) => element > 13;
print(array1.findIndex(isLargeNumber));

const arrayLike = {
	  length: 3,
	  0: 2,
	  1: 7.3,
	  2: 4,
};
print(Array.prototype.findIndex.call(arrayLike, (x) => !Number.isInteger(x)));
print([1, , 3].findIndex((x) => x === undefined));
function isPrime(element) {
	if (element % 2 === 0 || element < 2) {
		return false;
	}
	for (let factor = 3; factor <= Math.sqrt(element); factor += 2) {
	    if (element % factor === 0) {
			return false;
		}
	}
	return true;
}
print([4, 6, 8, 9, 12].findIndex(isPrime));
print([4, 6, 7, 9, 12].findIndex(isPrime));

const words = ["spray", "limit", "limits"];
const deleteWords = words.findIndex((word, index, arr) => {
  arr.length=2
  return word == "limits"
});
print(deleteWords);
print(words.length);

var array = ["first", "second"];
Object.defineProperty(array, 0, {
  get: function() {
    array.length = 0;
    return "first";
  }
});
const hasFirst = (element) => element == "first";
print(array.findIndex(hasFirst));
