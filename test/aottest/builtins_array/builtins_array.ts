/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
declare function print(arg:any):string;
{
    // test new builtin array
    let array1 = new Array();
    let array2 = new Array(1);

    print(array1.length);
    print(array2.length);

    // test call builtin constructor
    print(Array().length);   // c++ path
    print(Array(1).length);  // c++ path

    try {
        let array3 = new Array(-1);
        print(array3.length);
    } catch (e) {
        print("throw exception");
    }

    try {
        let array3 = new Array(2.1);
        print(array3.length);
    } catch (e) {
        print("throw exception");
    }

    try {
        let array3 = new Array(4294967296);
        print(array3.length);
    } catch (e) {
        print("throw exception");
    }
}

function unshift() {
  for (var i = 0; i < 10; i++) {
    [0, 0, 0, 0, 0, 0, 0].unshift(0);
	  [0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0].unshift(0);
  }
}

function forEach() {
  let array = [];
  for (let i = 0; i < 10; i++) array.push(i);
  array.forEach(function(item){
      item += 1;
  });
}

function spliced() {
  let array = [];
  for (let i = 0; i < 10; i++) array.push(i);
  array.splice(0, 10);
}

function toSpliced() {
  let array = [];
  for (let i = 0; i < 10; i++) array.push(i);
  let b = array.toSpliced(0, 2);
}

function some() {
  let array = [];
  for (let i = 0; i < 10; i++) array.push(i);
  for (let i = 0; i < 10; i++) {
      const condition = (element) => element < 10;
      array.some(condition);
  }
}

function every() {
  let array = [];
  for (let i = 0; i < 10; i++) array.push(i);
  const condition = (element) => element < 10;
  array.every(condition);
}

function reduceRight() {
  const array1 = [
      [0, 1],
      [2, 3],
      [4, 5],
    ];
  for (let i = 0; i < 5; i++) {
      const result = array1.reduceRight((accumulator, currentValue) =>
      accumulator.concat(currentValue),
    );
  }
}

function findLastIndex() {
  let array = [];
  for (let i = 0; i < 10; i++) array.push(i);
  for (let i = 0; i < 10; i++) {
      const condition = (element) => element < 10;
      array.findLastIndex(condition);
  }
}

function findLast() {
  let array = [];
  for (let i = 0; i < 10; i++) array.push(i);
  for (let i = 0; i < 10; i++) {
      const condition = (element) => element < 10;
      array.findLast(condition);
  }
}

function flatMap() {
  let array = [];
  for (let i = 0; i < 10; i++) array.push(i);
  for (let i = 0; i < 10; i++) {
      array.flatMap((num) => (num % 2 == 0 ? [num, num] : num));
  }
}

function testGenericFastPath() {
  for (var i = 0; i < 10; i++) {
    let a = [0, 0, , undefined, 0, 0, 0];	// elementsKind is generic
    a.unshift(0); // undefined can set array generic
  }
}

function testGenericSlowPath() {
  for (var i = 0; i < 10; i++) {
    let a = [0, 0, , undefined, 0, 0, 0];
    a.x = 1; // elementsKind is generic but transition
    a.unshift(0);
  }
}
  
unshift();
forEach();
spliced();
toSpliced();
some();
every();
reduceRight();
findLastIndex();
findLast();
flatMap();
testGenericFastPath();
testGenericSlowPath();