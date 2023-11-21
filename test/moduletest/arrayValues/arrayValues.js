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
 * @tc.name:stringSlice
 * @tc.desc:test String.slice
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */


const array1 = ['a', 'b', 'c'];
const iterator1 = array1.values();

for (const value of iterator1) {
	  print(value);
}
const arrayLike = {
	  length: 3,
	  0: "a",
	  1: "b",
	  2: "c",
};
for (const entry of Array.prototype.values.call(arrayLike)) {
	  print(entry);
}
for (const element of [, "a"].values()) {
	  print(element);
}
const arr = ["a", "b", "c", "d", "e"];
const iterator = arr.values();
print(iterator.next().value); // "a"
arr[1] = "n";
print(iterator.next().value); // "n"
const array = ["a", "b", "c", "d", "e"];
const values = array.values();
for (const letter of values) {
	  print(letter);
	  if (letter === "b") {
		      break;
		    }
}
for (const letter of values) {
	  print(letter);
}

const arr1 = ["a", "b", "c", "d", "e"];
const values1 = arr1.values();
for (const letter of values1) {
	  print(letter);
}
for (const letter of values1) {
	  print(letter);
}
