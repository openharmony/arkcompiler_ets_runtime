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

const array1 = [1, 2, 3, 4];
const initialValue = 0;
const sumWithInitial = array1.reduce(
  (accumulator, currentValue) => accumulator + currentValue,
  initialValue,
);
print(sumWithInitial);
const objects = [{ x: 1 }, { x: 2 }, { x: 3 }];
const sum = objects.reduce(
  (accumulator, currentValue) => accumulator + currentValue.x,
  0,
);
print(sum); // 6
print([1, 2, , 4].reduce((a, b) => a + b)); // 7
print([1, 2, undefined, 4].reduce((a, b) => a + b)); // NaN
const arrayLike = {
  length: 3,
  0: 2,
  1: 3,
  2: 4,
};
print(Array.prototype.reduce.call(arrayLike, (x, y) => x + y));
const myArray = ["a", "b", "a", "b", "c", "e", "e", "c", "d", "d", "d", "d"];
const myArrayWithNoDuplicates = myArray.reduce((accumulator, currentValue) => {
  if (!accumulator.includes(currentValue)) {
    return [...accumulator, currentValue];
  }
  return accumulator;
}, []);

print(myArrayWithNoDuplicates);
const numbers = [-5, 6, 2, 0];

const doubledPositiveNumbers = numbers.reduce((accumulator, currentValue) => {
  if (currentValue > 0) {
    const doubled = currentValue * 2;
    return [...accumulator, doubled];
  }
  return accumulator;
}, []);

print(doubledPositiveNumbers); // [12, 4]

function runPromiseInSequence(arr, input) {
  return arr.reduce(
    (promiseChain, currentFunction) => promiseChain.then(currentFunction),
    Promise.resolve(input),
  );
}

function p1(a) {
  return new Promise((resolve, reject) => {
    resolve(a * 5);
  });
}

function p2(a) {
  return new Promise((resolve, reject) => {
    resolve(a * 2);
  });
}

function f3(a) {
  return a * 3;
}

function p4(a) {
  return new Promise((resolve, reject) => {
    resolve(a * 4);
  });
}

const promiseArr = [p1, p2, f3, p4];
runPromiseInSequence(promiseArr, 10).then(console.log); // 1200
