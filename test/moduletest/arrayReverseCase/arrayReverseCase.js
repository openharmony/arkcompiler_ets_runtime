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

const array1 = ['one', 'two', 'three'];
print('array1:', array1);
// Expected output: "array1:" Array ["one", "two", "three"]

const reversed1 = array1.reverse();
print('reversed1:', reversed1);
// Expected output: "reversed1:" Array ["three", "two", "one"]

// Careful: reverse is destructive -- it changes the original array.
print('array1:', array1);
// Expected output: "array1:" Array ["three", "two", "one"]

print([1, , 3].reverse()); // [3, empty, 1]
print([1, , 3, 4].reverse()); // [4, 3, empty, 1]

const numbers = [3, 2, 4, 1, 5];
const reverted = [...numbers].reverse();
reverted[0] = 5;
print(numbers[0]); // 3

const numbers1 = [3, 2, 4, 1, 5];
const reversed = numbers1.reverse();
reversed[0] = 5;
print(numbers1[0]); // 5

var array2 = new Uint8Array([1, 2, 3, 4]);
Object.defineProperty(array2, "length", { value: 2 });
Array.prototype.reverse.call(array2);
print(array2)
try {
    var array3 = new Uint8Array([1, 2, 3, 4]);
    Object.defineProperty(array3, "length", { value: 5 });
    Array.prototype.reverse.call(array3);
    print(array3)
} catch (error) {
    print(error.name)
}
