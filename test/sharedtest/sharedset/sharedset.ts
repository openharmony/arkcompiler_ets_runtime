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

function FillSet(set: SharedSet): void {
  for (let i = 0; i < 5; i++) {
    set.add(i);
  }
}
let sharedSet: SharedSet = new SharedSet<number>();

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

sharedSet.forEach((key: number, value: number, set: SharedSet) => {
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
sharedSet.forEach((key: number, _: number, set: SharedSet) => {
  print("set key[forEach]: " + key);
})
try {
  sharedSet.forEach((key: number, _: number, set: SharedSet) => {
    set.add(key + 5);
  });
} catch (e) {
  print("Add Scenario[forEach]: " + e + ", errCode: " + e.code);
}
try {
  sharedSet.forEach((key: number, _: number, set: SharedSet) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print("Delete Scenario[forEach]: " + e + ", errCode: " + e.code);
}
try {
  sharedSet.forEach((key: number, _: number, set: SharedSet) => {
    set.clear();
  });
} catch (e) {
  print("Clear Scenario[forEach]: " + e + ", errCode: " + e.code);
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
  sharedSet = new SharedSet(["str", 1, sObj, undefined, true, null]);
  print("sharedSet add[shared] element success");
} catch (e) {
  print("sharedSet add[unshared]: " + e + ", errCode: " + e.code);
}

try {
  let obj = {}
  sharedSet = new SharedSet([obj]);
} catch (e) {
  print("sharedSet add[unshared]: " + e + ", errCode: " + e.code);
}

try {
  let sym = Symbol("testSymbol")
  sharedSet = new SharedSet([sym, 2]);
} catch (e) {
  print("sharedSet add[unshared]: " + e + ", errCode: " + e.code);
}
print("===Type check end===");

print("===Class inheritance test begin ===");
class SubSharedSet<T> extends SharedSet {
  desc: string = "I'am SubSharedSet";
  constructor(entries?: T[] | null) {
    "use sendable";
    super(entries)
  }
}

let subSharedset = new SubSharedSet<number>();
subSharedset.add(1);
print(subSharedset.has(1));
print(subSharedset.size);

subSharedset = new SubSharedSet<number>([1, 2, 3]);
print(subSharedset.has(1));
print(subSharedset.has(2));
print(subSharedset.has(3));
print(subSharedset.size);

try {
  let obj = {};
  subSharedset = new SubSharedSet<Object>([obj]);
  print(subSharedset.size);
} catch (e) {
  print("SubSharedSet add[unshared]: " + e + ", errCode: " + e.code);
}

subSharedset = new SubSharedSet<string>(["one", "two", "three"]);
for (const [key, _] of subSharedset.entries()) {
  print("SubSharedSet key[for-of]: " + key);
}

try {
  subSharedset = new SubSharedSet<number>([1, 2, 3, 4]);
  print(subSharedset.size);
  subSharedset.forEach((key: number, _: number, set: SubSharedSet) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print("SubSharedSet Delete Scenario[forEach]: " + e + ", errCode: " + e.code);
}

class SubSubSharedSet<T> extends SubSharedSet {
  constructor(entries?: T[] | null) {
    "use sendable";
    super(entries)
  }
}

let subSubSharedSet = new SubSubSharedSet<number>([1, 2, 3]);
print(subSubSharedSet.has(1));
print(subSubSharedSet.has(2));
print(subSubSharedSet.has(3));
print(subSubSharedSet.size);

try {
  subSubSharedSet["extension"] = "value";
} catch(e) {
  print("add extension(.): " + e);
}
try {
  subSubSharedSet.extension = "value";
} catch(e) {
  print("add extension([]): " + e);
}

try {
  let obj = {};
  subSubSharedSet = new SubSubSharedSet<Object>([obj]);
  print(subSubSharedSet.size);
} catch (e) {
  print("SubSubSharedSet add[unshared]: " + e + ", errCode: " + e.code);
}

try {
  subSubSharedSet = new SubSubSharedSet<number>([1, 2, 3, 4]);
  subSubSharedSet.forEach((key: number, _: number, set: SubSubSharedSet) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print("SubSubSharedSet Delete Scenario[forEach]: " + e + ", errCode: " + e.code);
}
print("===Class inheritance test end ===");