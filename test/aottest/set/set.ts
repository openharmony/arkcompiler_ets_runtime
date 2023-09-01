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
function logMapElements(value, key, set) {
  print(`m[${key}] = ${value}`);
}

new Set(['foo', 'bar', undefined]).forEach(logMapElements);


function logMapElements1(value, key, set) {
  print(`m[${key}] = ${value}`);
  set.delete(key);
  print(set.size);
}

new Set(['foo', 'bar', undefined]).forEach(logMapElements1);

let mySet = new Set();
for (let i = 0; i < 1000; i++) {
    mySet.add(i);
    mySet.add("key" + i);
    mySet.add(Symbol("foo"));
}

const hugeString = BigInt("9007199254740991");
for (let i = 0; i < 100; i++) {
    mySet.add(hugeString + BigInt(i));
}

var intSum = 0;
var symbolCnt = 0;
var bigIntSum = BigInt("");
var stringCnt = 0;
mySet.forEach((value, key, set) => {
    if (typeof key == "bigint") {
        bigIntSum += value;
        return;
    }
    if (typeof key == "number") {
        intSum += value;
        return;
    }
    if (typeof key == "symbol") {
        symbolCnt += 1;
        return;
    }
    if (typeof key == "string") {
        stringCnt += 1;
        return;
    }
});


print(intSum);
print(bigIntSum);
print(symbolCnt);
print(stringCnt);

