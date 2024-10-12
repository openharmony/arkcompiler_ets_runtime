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
 * @tc.name:sendablemap
 * @tc.desc:test sendablemap
 * @tc.type: FUNC
 * @tc.require: issue#I93TZC
 */

// @ts-nocheck
declare function print(str: any): string;

function FillMap(map: SendableMap): void {
  for (let i = 0; i < 5; i++) {
    map.set(i, 'value' + i);
  }
}
let sharedMap: SendableMap = new SendableMap<number, string>();

// Basic tests
print("===Basic test begin===")
FillMap(sharedMap);
print("map size is " + sharedMap.size);
print(SendableMap[Symbol.species] == SendableMap);
print(SendableMap.name == 'SendableMap');
print(SendableMap[Symbol.species] == Map);

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

sharedMap.forEach((value: string, key: number, map: SendableMap) => {
  print('map key[forEach]:' + 'key:' + key + ', value:' + value);
});

print(sharedMap[Symbol.toStringTag] == 'SendableMap');
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
sharedMap.forEach((_: string, key: number, map: SendableMap) => {
  print('map key[forEach]: ' + key);
});
try {
  sharedMap.forEach((_: string, key: number, map: SendableMap) => {
    map.set(key + 5, 'value' + key + 5);
  });
} catch (e) {
  print("Set Scenario[forEach]: " + e + ", errCode: " + e.code);
}
try {
  sharedMap.forEach((_: string, key: number, map: SendableMap) => {
    if (key % 2 == 0) {
      map.delete(key);
    }
  });
} catch (e) {
  print("Delete Scenario[forEach]: " + e + ", errCode: " + e.code);
}
try {
  sharedMap.forEach((_: string, key: number, map: SendableMap) => {
    map.clear();
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
  sharedMap = new SendableMap([
    ['str', 1],
    [sObj, undefined],
    [true, null],
  ]);
  print("sharedMap set[shared] element success");
} catch (e) {
  print("sharedMap set[unshared]: " + e + ", errCode: " + e.code);
}

try {
  let obj = {}
  sharedMap = new SendableMap([
    ['str', 1],
    [obj, 2],
  ]);
} catch (e) {
  print("sharedMap set[unshared]: " + e + ", errCode: " + e.code);
}

try {
  let sym = Symbol("testSymbol")
  sharedMap = new SendableMap([
    ['str', 1],
    [sym, 2],
  ]);
} catch (e) {
  print("sharedMap set[unshared]: " + e + ", errCode: " + e.code);
}
print("===Type check end===");

print("===Class inheritance test begin ===");
class SubSendableMap<K, V> extends SendableMap {
  desc: string = "I'am SubSendableMap";
  constructor(entries?: [K, V][] | null) {
    'use sendable';
    super(entries);
  }
}

let subSendableMap = new SubSendableMap<number, string>();
subSendableMap.set(1, 'one');
print(subSendableMap.has(1));
print(subSendableMap.size);

try {
  subSendableMap['extension'] = 'value';
} catch(e) {
  print("add extension(.): " + e);
}
try {
  subSendableMap.extension = 'value';
} catch(e) {
  print("add extension([]): " + e);
}

try {
  let obj = {};
  subSendableMap = new SubSendableMap<string, Object>([['object', obj]]);
  print(subSendableMap.size);
} catch (e) {
  print('SubSendableMap set[unshared]: ' + e + ', errCode: ' + e.code);
}

subSendableMap = new SubSendableMap<number, string>([
  [1, 'one'],
  [2, 'two'],
  [3, 'three'],
]);
print(subSendableMap.size);
for (const [key, value] of subSendableMap.entries()) {
  print('SubSendableMap [key, value][for-of]: ' + '[' + key + ', ' + value + ']');
}

try {
  subSendableMap.forEach((value: string, key: number, map: SubSendableMap) => {
    if (key % 2 == 0) {
      map.delete(key);
    }
  });
} catch (e) {
  print('SubSendableMap Delete Scenario[forEach]: ' + e + ', errCode: ' + e.code);
}

class SubSubSendableMap<K, V> extends SubSendableMap {
  constructor(entries?: [K, V][] | null) {
    'use sendable';
    super(entries);
  }
}

let subSubSendableMap = new SubSubSendableMap<number, string>();
subSubSendableMap.set(1, 'one');
print(subSubSendableMap.has(1));
print(subSubSendableMap.size);

try {
  let obj = {};
  subSubSendableMap = new SubSubSendableMap<string, Object>([['object', obj]]);
  print(subSubSendableMap.size);
} catch (e) {
  print('SubSubSendableMap set[unshared]: ' + e + ', errCode: ' + e.code);
}

subSubSendableMap = new SubSubSendableMap<number, string>([
  [1, 'one'],
  [2, 'two'],
  [3, 'three'],
]);
print(subSendableMap.size);
for (const [key, value] of subSendableMap.entries()) {
  print('SubSubSendableMap [key, value][for-of]: ' + '[' + key + ', ' + value + ']');
}

try {
  subSubSendableMap.forEach((value: string, key: number, map: SubSubSendableMap) => {
    if (key % 2 == 0) {
      map.delete(key);
    }
  });
} catch (e) {
  print('SubSubSendableMap Delete Scenario[forEach]: ' + e + ', errCode: ' + e.code);
}

print("=== An iterable object to convert to an ArkTS Map begin===")
sharedMap.clear();
FillMap(sharedMap);
print("map size is " + sharedMap.size);
try {
  const iterator = sharedMap.entries();
  let sharedMap1: SendableMap = new SendableMap(iterator);
  sharedMap1.forEach((value: string, key: number, map: SendableMap) => {
    print("map key[forEach]:" + "key:" + key + ", value:" + value);
  });
} catch (e) {
  print("SendableMapConstructor Scenario[next()]: " + e);
}

print("===Class inheritance test end ===");
