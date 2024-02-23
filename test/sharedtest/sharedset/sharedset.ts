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
 * @tc.name:sharedset
 * @tc.desc:test sharedset
 * @tc.type: FUNC
 * @tc.require: issue#I93TZC
 */

// @ts-nocheck
declare function print(str: any): string;

function FillSet(set: SharedSet) {
  for (let i = 0; i < 5; i++) {
    set.add(i);
  }
}
let sharedSet = new SharedSet<number>();

// Basic tests
print("===Basic test begin===")
FillSet(sharedSet);
print("set size is " + sharedSet.size);
print(SharedSet[Symbol.species] == SharedSet)
print(SharedSet.name == "SharedSet")
print(SharedSet[Symbol.species] == Set)

const keyIter = sharedSet.keys();
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

const valueIter = sharedSet.keys();
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

sharedSet.forEach((key, value, set) => {
  print("set key[forEach]:" + "key:" + key + ", value:" + value);
});

print(sharedSet[Symbol.toStringTag] == "SharedSet")
for (let iter of sharedSet[Symbol.iterator]()) {
  print("set key[Symbol.iterator]:" + iter);
}
print(sharedSet[Symbol.iterator] == sharedSet.values);
print(sharedSet[Symbol.iterator] == sharedSet.keys);

print(sharedSet.has(4));
sharedSet.add(4);
print(sharedSet.size == 5);
print(sharedSet.has(10));
sharedSet.add(10);
print(sharedSet.size == 6);
print(sharedSet.has(10));
sharedSet.delete(10);
print(sharedSet.has(10));
print(sharedSet.size == 5);
sharedSet.clear();
print(sharedSet.size == 0);
try {
  sharedSet["extension"] = "value";
} catch(e) {
  print("add extension(.): " + e);
}
try {
  sharedSet.extension = "value";
} catch(e) {
  print("add extension([]): " + e);
}

print("===Basic test end===");

// No Expected Concurrent modification exception while iterating using iterators
print("===Concurrent modification during iteration Test(iterator) begin===")
sharedSet.clear();
FillSet(sharedSet);
print("set size is " + sharedSet.size);

const iterator = sharedSet.entries();
for (const [key, _] of iterator) {
  print("set key[for-of]: " + key);
}
try {
  const iterator = sharedSet.entries();
  for (const [key, _] of iterator) {
    if (key == 1) {
      sharedSet.add(key + 5);
    }
  }
  print("Add Scenario[for-of] updated size: " + sharedSet.size);
} catch (e) {
  print("Add Scenario[for-of]: " + e);
}
try {
  const iterator = sharedSet.entries();
  for (const [key, _] of iterator) {
    if (key % 2 == 0) {
      sharedSet.delete(key);
    }
  }
  print("Delete Scenario[for-of] updated size: " + sharedSet.size);
} catch (e) {
  print("Delete Scenario[for-of]: " + e);
}
try {
  const iterator = sharedSet.entries();
  for (const [key, _] of iterator) {
    sharedSet.clear();
  }
  print("Clear Scenario[for-of] updated size: " + sharedSet.size);
} catch (e) {
  print("Clear Scenario[for-of]: " + e);
}

sharedSet.clear();
FillSet(sharedSet);
print("set size is " + sharedSet.size);
try {
  const iterator = sharedSet.entries();
  sharedSet.add(6);
  iterator.next();
  print("Add Scenario[next()] updated size: " + sharedSet.size);
} catch (e) {
  print("Add Scenario[next()]: " + e);
}
try {
  const iterator = sharedSet.entries();
  sharedSet.delete(6);
  iterator.next();
  print("Delete Scenario[next()] updated size: " + sharedSet.size);
} catch (e) {
  print("Delete Scenario[next()]: " + e);
}
try {
  const iterator = sharedSet.entries();
  sharedSet.clear();
  iterator.next();
  print("Clear Scenario[next()] updated size: " + sharedSet.size);
} catch (e) {
  print("Clear Scenario[next()]: " + e);
}
print("===Concurrent modification during iteration Test(iterator) end===")

// Expected Concurrent modification exception while iterating using forEach
print("===Concurrent modification during iteration Test(forEach) begin===")
sharedSet.clear();
FillSet(sharedSet);
print("set size is " + sharedSet.size);
sharedSet.forEach((key, _, set) => {
  print("set key[forEach]: " + key);
})
try {
  sharedSet.forEach((key, _, set) => {
    set.add(key + 5);
  });
} catch (e) {
  print("Add Scenario[forEach]: " + e);
}
try {
  sharedSet.forEach((key, _, set) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print("Delete Scenario[forEach]: " + e);
}
try {
  sharedSet.forEach((key, _, set) => {
    set.clear();
  });
} catch (e) {
  print("Clear Scenario[forEach]: " + e);
}
print("===Concurrent modification during iteration Test(forEach) end===");

sharedSet.clear();
FillSet(sharedSet);
const entries = sharedSet.entries();
for (let i = 0; i < 1024; i++) {
  for (const entry of entries) {
  }
}