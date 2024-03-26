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

/*
 * @tc.name:sharedmap
 * @tc.desc:test sharedmap
 * @tc.type: FUNC
 * @tc.require: issue#I93TZC
 */

// @ts-nocheck
declare function print(str: any): string;

function FillMap(map: SharedMap): void {
  for (let i = 0; i < 5; i++) {
    map.set(i, "value" + i);
  }
}
let sharedMap: SharedMap = new SharedMap<number, string>();

// Basic tests
print("===Basic test begin===")
FillMap(sharedMap);
print("map size is " + sharedMap.size);
print(SharedMap[Symbol.species] == SharedMap);
print(SharedMap.name == "SharedMap");
print(SharedMap[Symbol.species] == Map);

const keyIter = sharedMap.keys();
let nextEntry = keyIter.next();
print("keys next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = keyIter.next();
print("keys next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = keyIter.next();
print("keys next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = keyIter.next();
print("keys next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = keyIter.next();
print("keys next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = keyIter.next();
print("keys next:" + nextEntry.value + ", done: " + nextEntry.done);

const valueIter = sharedMap.values();
nextEntry = valueIter.next();
print("values next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = valueIter.next();
print("values next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = valueIter.next();
print("values next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = valueIter.next();
print("values next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = valueIter.next();
print("values next:" + nextEntry.value + ", done: " + nextEntry.done);
nextEntry = valueIter.next();
print("values next:" + nextEntry.value + ", done: " + nextEntry.done);

sharedMap.forEach((value: string, key: number, map: SharedMap) => {
  print("map key[forEach]:" + "key:" + key + ", value:" + value);
});

print(sharedMap[Symbol.toStringTag] == "SharedMap")
for (let iter of sharedMap[Symbol.iterator]()) {
  print("map key[Symbol.iterator]:" + iter);
}
for (let [key, value] of sharedMap.entries()) {
  print("map entries:[" + key +", " + value + "]");
}
print(sharedMap[Symbol.iterator] == sharedMap.entries);
print(sharedMap[Symbol.iterator] == sharedMap.keys);
print(sharedMap[Symbol.iterator] == sharedMap.values);

print(sharedMap.has(4));
sharedMap.set(4, "value4");
print(sharedMap.size == 5);
print(sharedMap.has(10));
sharedMap.set(10, "value10");
print(sharedMap.get(10) == "value10");
print(sharedMap.size == 6);
print(sharedMap.has(10));
sharedMap.delete(10);
print(sharedMap.has(10));
print(sharedMap.size == 5);
sharedMap.clear();
print(sharedMap.size == 0);
print("===Basic test end===");

// No expected Concurrent modification exception while iterating using iterators
print("===Concurrent modification during iteration Test(iterator) begin===")
sharedMap.clear();
FillMap(sharedMap);
print("map size is " + sharedMap.size);

const iterator = sharedMap.entries();
for (const [key, _] of iterator) {
  print("map key[for-of]: " + key);
}
try {
  const iterator = sharedMap.entries();
  for (const [key, _] of iterator) {
    if (key == 1) {
      sharedMap.set(key + 5, "value" + key + 5);
    }
  }
  print("Set Scenario[for-of] updated size: " + sharedMap.size);
} catch (e) {
  print("Set Scenario[for-of]: " + e);
}
try {
  const iterator = sharedMap.entries();
  for (const [key, _] of iterator) {
    if (key % 2 == 0) {
      sharedMap.delete(key);
    }
  }
  print("Delete Scenario[for-of] updated size: " + sharedMap.size);
} catch (e) {
  print("Delete Scenario[for-of]: " + e);
}
try {
  const iterator = sharedMap.entries();
  for (const [key, _] of iterator) {
    sharedMap.clear();
  }
  print("Clear Scenario[for-of] updated size: " + sharedMap.size);
} catch (e) {
  print("Clear Scenario[for-of]: " + e);
}

sharedMap.clear();
FillMap(sharedMap);
print("map size is " + sharedMap.size);
try {
  const iterator = sharedMap.entries();
  sharedMap.set(6, "value6");
  iterator.next();
  print("Set Scenario[next()] updated size: " + sharedMap.size);
} catch (e) {
  print("Set Scenario[next()]: " + e);
}
try {
  const iterator = sharedMap.entries();
  sharedMap.delete(6);
  iterator.next();
  print("Delete Scenario[next()] updated size: " + sharedMap.size);
} catch (e) {
  print("Delete Scenario[next()]: " + e);
}
try {
  const iterator = sharedMap.entries();
  sharedMap.clear();
  iterator.next();
  print("Clear Scenario[next()] updated size: " + sharedMap.size);
} catch (e) {
  print("Clear Scenario[next()]: " + e);
}
print("===Concurrent modification during iteration Test(iterator) end===")

// Expected Concurrent modification exception while iterating using forEach
print("===Concurrent modification during iteration Test(forEach) begin===")
sharedMap.clear();
FillMap(sharedMap);
print("map size is " + sharedMap.size);
sharedMap.forEach((_: string, key: number, map: SharedMap) => {
  print("map key[forEach]: " + key);
})
try {
  sharedMap.forEach((_: string, key: number, map: SharedMap) => {
    map.set(key + 5, "value" + key + 5);
  });
} catch (e) {
  print("Set Scenario[forEach]: " + e);
}
try {
  sharedMap.forEach((_: string, key: number, map: SharedMap) => {
    if (key % 2 == 0) {
      map.delete(key);
    }
  });
} catch (e) {
  print("Delete Scenario[forEach]: " + e);
}
try {
  sharedMap.forEach((_: string, key: number, map: SharedMap) => {
    map.clear();
  });
} catch (e) {
  print("Clear Scenario[forEach]: " + e);
}
print("===Concurrent modification during iteration Test(forEach) end===");

print("===Type check begin===");
class SObject {
  constructor() {
    "use sendable"
  }
};

try {
  let sObj = new SObject();
  sharedMap = new SharedMap([["str", 1], [sObj, undefined], [true, null]]);
  print("sharedMap set[shared] element");
} catch (e) {
  print("sharedMap set[unshared]: " + e);
}

try {
  let obj = {}
  sharedMap = new SharedMap([["str", 1], [obj, 2]]);
} catch (e) {
  print("sharedMap set[unshared]: " + e);
}

try {
  let sym = Symbol("testSymbol")
  sharedMap = new SharedMap([["str", 1], [sym, 2]]);
} catch (e) {
  print("sharedMap set[unshared]: " + e);
}
print("===Type check end===");