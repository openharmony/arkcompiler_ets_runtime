/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

let array = ['1', '2', '3'];
let map = new Map();
map.set('a', 1);
map.set('b', 2);

function func() {

}

let objs = [array, map, func];

// integer or float constant
let num1 = 42; // int
let num2 = 3.14; // float
let bigInt = BigInt(9007199254740991);
let num3 = NaN; // nan number
let num4 = Infinity; // Positive infinity
let num5 = -Infinity; // negative infinity

// string
let str1 = 'Hello';
let str2 = 'World';
let str3 = `This is a template string. ${str1} ${str2}!`;

// bool
let bool1 = true;
let bool2 = false;

// null type
let nullVar = null;

// undefined type
let undefinedVar;

// array type
let arr0 = new Int8Array();
let arr1 = new Uint8Array();
let arr2 = new Int16Array();
let arr3 = new Uint16Array();
let arr4 = new Int32Array();
let arr5 = new Uint32Array();
let arr6 = new BigInt64Array();
let arr7 = new BigUint64Array();
let arr8 = new Float32Array();
let arr9 = new Float64Array();
let arr10 = new Uint8ClampedArray();
let buffer = new ArrayBuffer(8);
let sharedBuf = new SharedArrayBuffer(1024);

let list = [1, 2, 3, 4, 5];
for (let item of list) {
  print(item);
}

// funtion
function greet() {
  print('Hello, this is a function!');
}

// data type
let date = new Date();

// lambda
let regex = /hello/i; // Case insensitive matching "hello"

// object type
let obj = {
  name: 'Alice',
  age: 25,
  greet: function() {
    print('Hello, my name is ' + this.name);
  }
};

// class
class Person {
  constructor(name, age) {
    this.name = name;
    this.age = age;
  }

  greet() {
    print(`Hello, my name is ${this.name} and I'm ${this.age} years old.`);
  }
}

globalThis.person = new Person('Alice', 25);

// Exception
function divide(a, b) {
  if (b === 0) {
    throw new Error('The divisor cannot be 0');
  }
  return a / b;
}

try {
  let result = divide(10, 0);
} catch (error) {
  print('\nthrow statement\n');
}

// Promise
let promise = new Promise((resolve, reject) => {
  // Simulated asynchronous operation
  setTimeout(() => {
    let success = false; // Simulated operation failed
    if (success) {
      resolve('operation success');
    } else {
      reject(new Error('operation failed'));
    }
  }, 1000);
});

promise.then((result) => {
  print(result);
}).catch((error) => {
  print('Promise handle\n');
});

// Asynchronous type
async function fetchDataWithAsyncAwait() {
  return 'async/await get the data';
}

async function fetchAndLogData() {
  try {
    const data = await fetchDataWithAsyncAwait();
  } catch (err) {
    print('ERROR:', err);
  }
}

fetchAndLogData();
