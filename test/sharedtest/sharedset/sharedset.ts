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
 * @tc.name:sendableset
 * @tc.desc:test sendableset
 * @tc.type: FUNC
 * @tc.require: issue#I93TZC
 */

// @ts-nocheck
declare function print(str: any): string;

function FillSet(set: SendableSet): void {
  for (let i = 0; i < 5; i++) {
    set.add(i);
  }
}
let sharedSet: SendableSet = new SendableSet<number>();

// Basic tests
print("===Basic test begin===")
FillSet(sharedSet);
print("set size is " + sharedSet.size);
print(SendableSet[Symbol.species] == SendableSet);
print(SendableSet.name == 'SendableSet');
print(SendableSet[Symbol.species] == Set);

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

sharedSet.forEach((key: number, value: number, set: SendableSet) => {
  print('set key[forEach]:' + 'key:' + key + ', value:' + value);
});

print(sharedSet[Symbol.toStringTag] == 'SendableSet');
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
sharedSet.forEach((key: number, _: number, set: SendableSet) => {
  print('set key[forEach]: ' + key);
});
try {
  sharedSet.forEach((key: number, _: number, set: SendableSet) => {
    set.add(key + 5);
  });
} catch (e) {
  print("Add Scenario[forEach]: " + e + ", errCode: " + e.code);
}
try {
  sharedSet.forEach((key: number, _: number, set: SendableSet) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print("Delete Scenario[forEach]: " + e + ", errCode: " + e.code);
}
try {
  sharedSet.forEach((key: number, _: number, set: SendableSet) => {
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
  sharedSet = new SendableSet(['str', 1, sObj, undefined, true, null]);
  print("sharedSet add[shared] element success");
} catch (e) {
  print("sharedSet add[unshared]: " + e + ", errCode: " + e.code);
}

try {
  let obj = {}
  sharedSet = new SendableSet([obj]);
} catch (e) {
  print("sharedSet add[unshared]: " + e + ", errCode: " + e.code);
}

try {
  let sym = Symbol("testSymbol")
  sharedSet = new SendableSet([sym, 2]);
} catch (e) {
  print("sharedSet add[unshared]: " + e + ", errCode: " + e.code);
}
print("===Type check end===");

print("===Class inheritance test begin ===");
class SubSendableSet<T> extends SendableSet {
  desc: string = "I'am SubSendableSet";
  constructor(entries?: T[] | null) {
    'use sendable';
    super(entries);
  }
}

let subSharedset = new SubSendableSet<number>();
subSharedset.add(1);
print(subSharedset.has(1));
print(subSharedset.size);

subSharedset = new SubSendableSet<number>([1, 2, 3]);
print(subSharedset.has(1));
print(subSharedset.has(2));
print(subSharedset.has(3));
print(subSharedset.size);

try {
  let obj = {};
  subSharedset = new SubSendableSet<Object>([obj]);
  print(subSharedset.size);
} catch (e) {
  print('SubSendableSet add[unshared]: ' + e + ', errCode: ' + e.code);
}

subSharedset = new SubSendableSet<string>(['one', 'two', 'three']);
for (const [key, _] of subSharedset.entries()) {
  print('SubSendableSet key[for-of]: ' + key);
}

try {
  subSharedset = new SubSendableSet<number>([1, 2, 3, 4]);
  print(subSharedset.size);
  subSharedset.forEach((key: number, _: number, set: SubSendableSet) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print('SubSendableSet Delete Scenario[forEach]: ' + e + ', errCode: ' + e.code);
}

class SubSubSendableSet<T> extends SubSendableSet {
  constructor(entries?: T[] | null) {
    'use sendable';
    super(entries);
  }
}

let subSubSendableSet = new SubSubSendableSet<number>([1, 2, 3]);
print(subSubSendableSet.has(1));
print(subSubSendableSet.has(2));
print(subSubSendableSet.has(3));
print(subSubSendableSet.size);

try {
  subSubSendableSet['extension'] = 'value';
} catch(e) {
  print("add extension(.): " + e);
}
try {
  subSubSendableSet.extension = 'value';
} catch(e) {
  print("add extension([]): " + e);
}

try {
  let obj = {};
  subSubSendableSet = new SubSubSendableSet<Object>([obj]);
  print(subSubSendableSet.size);
} catch (e) {
  print('SubSubSendableSet add[unshared]: ' + e + ', errCode: ' + e.code);
}

try {
  subSubSendableSet = new SubSubSendableSet<number>([1, 2, 3, 4]);
  subSubSendableSet.forEach((key: number, _: number, set: SubSubSendableSet) => {
    if (key % 2 == 0) {
      set.delete(key);
    }
  });
} catch (e) {
  print('SubSubSendableSet Delete Scenario[forEach]: ' + e + ', errCode: ' + e.code);
}

print("=== An iterable object to convert to an ArkTS Set  begin===")
sharedSet.clear();
FillSet(sharedSet);
try {
  
  const iterator = sharedSet.entries();
  let sharedSet1: SendableSet = new SendableSet<>(iterator);
  sharedSet1.forEach((key: number, _: number, set: SendableSet) => {
    print("set key[forEach]: " + key);
  })
} catch (e) {
  print("SendableSetConstructor Scenario[next()]: " + e);
}

print("===Class inheritance test end ===");
