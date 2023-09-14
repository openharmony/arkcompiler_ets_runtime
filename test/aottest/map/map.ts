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
function logMapElements(value, key, map) {
  print(`m[${key}] = ${value}`);
}

new Map([
  ['foo', 3],
  ['bar', {}],
  ['baz', undefined],
]).forEach(logMapElements);


function logMapElements1(value, key, map) {
  print(`m[${key}] = ${value}`);
  map.delete(key);
  print(map.size);
}

new Map([
  ['foo', 3],
  ['bar', {}],
  ['baz', undefined],
]).forEach(logMapElements1);

let myMap = new Map();
for (let i = 0; i < 1000; i++) {
    myMap.set(i, i);
    myMap.set("key" + i, i);
    myMap.set(Symbol("foo"), i)
}

const hugeString = BigInt("9007199254740991");
for (let i = 0; i < 100; i++) {
    myMap.set(hugeString + BigInt(i), hugeString);
}

var intSum = 0;
var symbolSum = 0;
var bigIntSum = BigInt("");
var stringSum = 0;
myMap.forEach((value, key, map) => {
    if (typeof key == "bigint") {
        bigIntSum += value;
        return;
    }
    if (typeof key == "number") {
        intSum += value;
        return;
    }
    if (typeof key == "symbol") {
        symbolSum += value;
        return;
    }
    if (typeof key == "string") {
        stringSum += value;
        return;
   }
});

print(intSum);
print(bigIntSum);
print(symbolSum);
print(stringSum)
