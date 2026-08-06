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

function TestSendableMapContainsValueInterface(): void {
  print("Start Test SendableMap containsValue");

  print("=== containsValue common usage case ===");
  let containsValueMap = new SendableMap([
    ['one', 1],
    ['nan', NaN],
    ['zero', +0],
    ['undefined', undefined],
  ]);
  print("containsValue 1: " + containsValueMap.containsValue(1));
  print("containsValue missing: " + containsValueMap.containsValue(2));
  print("containsValue size unchanged: " + containsValueMap.size);
  for (const [key, value] of containsValueMap.entries()) {
    print("containsValue entries:[" + key + ", " + value + "]");
  }

  print("=== containsValue empty case ===");
  let emptyMap = new SendableMap();
  print("containsValue empty: " + emptyMap.containsValue(1));

  print("=== containsValue SameValueZero case ===");
  print("containsValue NaN: " + containsValueMap.containsValue(NaN));
  print("containsValue +0: " + containsValueMap.containsValue(+0));
  print("containsValue -0: " + containsValueMap.containsValue(-0));

  print("=== containsValue undefined/null case ===");
  print("containsValue undefined: " + containsValueMap.containsValue(undefined));
  print("containsValue null: " + containsValueMap.containsValue(null));
  let containsNullMap = new SendableMap([
    ['null', null],
  ]);
  print("containsValue null hit: " + containsNullMap.containsValue(null));

  print("=== containsValue duplicate value case ===");
  let containsDuplicateValueMap = new SendableMap([
    [1, 'same'],
    [2, 'same'],
  ]);
  print("containsValue duplicate value: " + containsDuplicateValueMap.containsValue('same'));
  containsDuplicateValueMap.delete(1);
  print("containsValue duplicate kept after delete: " + containsDuplicateValueMap.containsValue('same'));

  print("=== containsValue deleted entry case ===");
  let containsDeletedSlotMap = new SendableMap([
    [1, 'deleted'],
    [2, 'kept'],
  ]);
  containsDeletedSlotMap.delete(1);
  print("containsValue deleted slot: " + containsDeletedSlotMap.containsValue('deleted'));
  print("containsValue kept after delete: " + containsDeletedSlotMap.containsValue('kept'));

  print("=== containsValue object case ===");
  print("containsValue unshared object: " + containsValueMap.containsValue({}));
  let sharedValue = new SObject();
  let containsObjectMap = new SendableMap([
    ['object', sharedValue],
  ]);
  print("containsValue same sendable object: " + containsObjectMap.containsValue(sharedValue));
  print("containsValue other sendable object: " + containsObjectMap.containsValue(new SObject()));

  print("=== containsValue exception case ===");
  try {
    SendableMap.prototype.containsValue.call({});
  } catch (e) {
    print("containsValue bind error: " + e + ", errCode: " + e.code);
  }
  const unboundContainsValue = containsValueMap.containsValue;
  const boundContainsValue = unboundContainsValue.bind({});
  try {
      print("" + boundContainsValue(1));
  } catch (err) {
      print("containsValue bind error: " + err + ", errCode: " + err.code);
  }

  print("=== containsValue concurrent case ===");
  let concurrentApiMap = new SendableMap([
    [1, 'one'],
    [2, 'two'],
  ]);
  try {
    concurrentApiMap.forEach((value: string, key: number, map: SendableMap) => {
      print("containsValue Scenario[forEach]: " + map.containsValue(value));
    });
  } catch (e) {
    print("containsValue Scenario[forEach]: " + e + ", errCode: " + e.code);
  }
}

function TestSendableMapPutInterface(): void {
  print("Start Test SendableMap put");

  print("=== put common usage case ===");
  let putMap = new SendableMap([
    [1, 'one'],
    [2, 'two'],
  ]);
  print("put missing old undefined: " + (putMap.put(3, 'three') === undefined));
  print("put missing value: " + putMap.get(3));
  print("put missing size: " + putMap.size);
  print("put existing old: " + putMap.put(1, 'ONE'));
  print("put existing value: " + putMap.get(1));
  print("put existing size: " + putMap.size);
  for (const [key, value] of putMap.entries()) {
    print("put entries:[" + key + ", " + value + "]");
  }

  print("=== put SameValueZero case ===");
  let putSameValueZeroMap = new SendableMap();
  print("put NaN first undefined: " + (putSameValueZeroMap.put(NaN, 'first') === undefined));
  print("put NaN second old: " + putSameValueZeroMap.put(NaN, 'second'));
  print("put NaN size: " + putSameValueZeroMap.size);
  print("put NaN value: " + putSameValueZeroMap.get(NaN));
  print("put +0 first undefined: " + (putSameValueZeroMap.put(+0, 'zero') === undefined));
  print("put -0 old: " + putSameValueZeroMap.put(-0, 'minus zero'));
  print("put zero size: " + putSameValueZeroMap.size);
  print("put zero value: " + putSameValueZeroMap.get(+0));
  print("put zero normal zero: " + putSameValueZeroMap.put(0, 'normal zero'));
  print("put zero value: " + putSameValueZeroMap.get(0));

  print("=== put undefined case ===");
  let putUndefinedMap = new SendableMap([
    ['a', undefined],
  ]);
  print("put undefined existed: " + putUndefinedMap.has('a'));
  print("put undefined old: " + (putUndefinedMap.put('a', 'A') === undefined));
  print("put undefined value: " + putUndefinedMap.get('a'));

  print("=== put exception case ===");
  try {
    new SendableMap().put({}, 'value');
  } catch (e) {
    print("put non-sendable key: " + e + ", errCode: " + e.code);
  }
  try {
    new SendableMap().put('key', {});
  } catch (e) {
    print("put non-sendable value: " + e + ", errCode: " + e.code);
  }
  try {
    SendableMap.prototype.put.call({}, 'key', 'value');
  } catch (e) {
    print("put bind error: " + e + ", errCode: " + e.code);
  }
  const unboundPut = putMap.put;
  const boundPut = unboundPut.bind({});
  try {
      print("" + boundPut('key','value'));
  } catch (err) {
      print("put bind error: " + err + ", errCode: " + err.code);
  }

  print("=== put concurrent case ===");
  let concurrentApiMap = new SendableMap([
    [1, 'one'],
    [2, 'two'],
  ]);
  try {
    concurrentApiMap.forEach((_: string, key: number, map: SendableMap) => {
      map.put(3, 'three');
    });
  } catch (e) {
    print("Put Scenario[forEach]: " + e + ", errCode: " + e.code);
  }
}

class PutAllSubSendableMap<K, V> extends SendableMap {
  constructor(entries?: [K, V][] | null) {
    'use sendable';
    super(entries);
  }
}

class PutAllSubBuiltinMap<K, V> extends Map {
  constructor(entries?: [K, V][] | null) {
    super(entries);
  }
}

function TestSendableMapPutAllInterface(): void {
  print("Start Test SendableMap putAll");

  print("=== putAll common usage case ===");
  let putAllMap = new SendableMap([
    [1, 'one'],
    [2, 'two'],
  ]);
  let fromSendableMap = new SendableMap([
    [3, 'three'],
    [4, 'four'],
    [1, 'ONE'],
  ]);
  print("putAll SendableMap return undefined: " + (putAllMap.putAll(fromSendableMap) === undefined));
  print("putAll SendableMap size: " + putAllMap.size);
  for (const [key, value] of putAllMap.entries()) {
    print("putAll SendableMap entries:[" + key + ", " + value + "]");
  }

  let fromBuiltinMap = new Map([
    [5, 'five'],
    [2, 'TWO'],
  ]);
  print("putAll BuiltinMap return undefined: " + (putAllMap.putAll(fromBuiltinMap) === undefined));
  print("putAll BuiltinMap size: " + putAllMap.size);
  for (const [key, value] of putAllMap.entries()) {
    print("putAll BuiltinMap entries:[" + key + ", " + value + "]");
  }

  print("=== putAll source immutable case ===");
  let immutableSendableSource = new SendableMap([
    [1, 'one'],
    [2, 'two'],
  ]);
  let immutableSendableReceiver = new SendableMap([
    [0, 'zero'],
  ]);
  immutableSendableReceiver.putAll(immutableSendableSource);
  print("putAll SendableMap source size: " + immutableSendableSource.size);
  for (const [key, value] of immutableSendableSource.entries()) {
    print("putAll SendableMap source entries:[" + key + ", " + value + "]");
  }
  let immutableBuiltinSource = new Map([
    [1, 'one'],
    [2, 'two'],
  ]);
  let immutableBuiltinReceiver = new SendableMap([
    [0, 'zero'],
  ]);
  immutableBuiltinReceiver.putAll(immutableBuiltinSource);
  print("putAll BuiltinMap source size: " + immutableBuiltinSource.size);
  for (const [key, value] of immutableBuiltinSource.entries()) {
    print("putAll BuiltinMap source entries:[" + key + ", " + value + "]");
  }

  print("=== putAll empty and self case ===");
  let emptyCaseMap = new SendableMap([
    [1, 'one'],
  ]);
  emptyCaseMap.putAll(new SendableMap());
  emptyCaseMap.putAll(new Map());
  print("putAll empty size: " + emptyCaseMap.size);
  print("putAll self return undefined: " + (emptyCaseMap.putAll(emptyCaseMap) === undefined));
  print("putAll self size: " + emptyCaseMap.size);
  for (const [key, value] of emptyCaseMap.entries()) {
    print("putAll self entries:[" + key + ", " + value + "]");
  }

  print("=== putAll SameValueZero case ===");
  let putAllSameValueZeroMap = new SendableMap([
    [NaN, 'nan'],
    [+0, 'zero'],
  ]);
  putAllSameValueZeroMap.putAll(new Map([
    [NaN, 'NAN'],
    [-0, 'minus zero'],
  ]));
  print("putAll NaN value: " + putAllSameValueZeroMap.get(NaN));
  print("putAll zero value: " + putAllSameValueZeroMap.get(+0));
  print("putAll SameValueZero size: " + putAllSameValueZeroMap.size);

  print("=== putAll undefined/null case ===");
  let putAllUndefinedNullMap = new SendableMap();
  putAllUndefinedNullMap.putAll(new Map([
    [undefined, 'undefined key'],
    [null, 'null key'],
    ['undefined value', undefined],
    ['null value', null],
  ]));
  print("putAll undefined key: " + putAllUndefinedNullMap.get(undefined));
  print("putAll null key: " + putAllUndefinedNullMap.get(null));
  print("putAll undefined value: " + (putAllUndefinedNullMap.get('undefined value') === undefined));
  print("putAll null value: " + (putAllUndefinedNullMap.get('null value') === null));
  print("putAll undefined/null size: " + putAllUndefinedNullMap.size);

  print("=== putAll deleted source entry case ===");
  let sourceWithDeletedEntry = new SendableMap([
    [1, 'deleted'],
    [2, 'two'],
    [3, 'three'],
  ]);
  sourceWithDeletedEntry.delete(1);
  sourceWithDeletedEntry.put(4, 'four');
  let putAllDeletedEntryMap = new SendableMap();
  putAllDeletedEntryMap.putAll(sourceWithDeletedEntry);
  for (const [key, value] of putAllDeletedEntryMap.entries()) {
    print("putAll deleted source entries:[" + key + ", " + value + "]");
  }
  let builtinSourceWithDeletedEntry = new Map([
    [1, 'deleted'],
    [2, 'two'],
    [3, 'three'],
  ]);
  builtinSourceWithDeletedEntry.delete(1);
  builtinSourceWithDeletedEntry.delete(3);
  builtinSourceWithDeletedEntry.set(4, 'four');
  let putAllBuiltinDeletedEntryMap = new SendableMap();
  putAllBuiltinDeletedEntryMap.putAll(builtinSourceWithDeletedEntry);
  for (const [key, value] of putAllBuiltinDeletedEntryMap.entries()) {
    print("putAll deleted BuiltinMap source entries:[" + key + ", " + value + "]");
  }

  print("=== putAll subclass case ===");
  let subPutAllReceiver = new PutAllSubSendableMap([
    [1, 'one'],
  ]);
  subPutAllReceiver.putAll(new PutAllSubSendableMap([
    [2, 'two'],
  ]));
  subPutAllReceiver.putAll(new PutAllSubBuiltinMap([
    [3, 'three'],
  ]));
  print("putAll subclass size: " + subPutAllReceiver.size);
  for (const [key, value] of subPutAllReceiver.entries()) {
    print("putAll subclass entries:[" + key + ", " + value + "]");
  }
  print("putAll subclass self return undefined: " + (subPutAllReceiver.putAll(subPutAllReceiver) === undefined));
  print("putAll subclass self size: " + subPutAllReceiver.size);
  for (const [key, value] of subPutAllReceiver.entries()) {
    print("putAll subclass self entries:[" + key + ", " + value + "]");
  }
  let subPutAllReceiver2 = new PutAllSubSendableMap([
    [1, 'one'],
  ]);
  subPutAllReceiver2.putAll(new SendableMap([
    [2, 'two'],
  ]));
  subPutAllReceiver2.putAll(new Map([
    [3, 'three'],
  ]));
  print("putAll subclass size: " + subPutAllReceiver2.size);
  for (const [key, value] of subPutAllReceiver2.entries()) {
    print("putAll subclass entries:[" + key + ", " + value + "]");
  }
  let putAllReceiver = new SendableMap([
    [1, 'one'],
  ]);
  putAllReceiver.putAll(new PutAllSubSendableMap([
    [2, 'two'],
  ]));
  putAllReceiver.putAll(new PutAllSubBuiltinMap([
    [3, 'three'],
  ]));
  print("putAll subclass size: " + putAllReceiver.size);
  for (const [key, value] of putAllReceiver.entries()) {
    print("putAll subclass entries:[" + key + ", " + value + "]");
  }

  print("=== putAll exception case ===");
  let putAllPartialKeyMap = new SendableMap([
    ['origin', 'origin'],
  ]);
  try {
    putAllPartialKeyMap.putAll(new Map([
      ['ok', 'ok'],
      [{}, 'bad'],
      ['after', 'after'],
    ]));
  } catch (e) {
    print("putAll non-sendable key: " + e + ", errCode: " + e.code);
  }
  print("putAll partial key size: " + putAllPartialKeyMap.size);
  for (const [key, value] of putAllPartialKeyMap.entries()) {
    print("putAll partial key entries:[" + key + ", " + value + "]");
  }

  let putAllPartialValueMap = new SendableMap([
    ['origin', 'origin'],
  ]);
  try {
    putAllPartialValueMap.putAll(new Map([
      ['ok', 'ok'],
      ['bad', {}],
      ['after', 'after'],
    ]));
  } catch (e) {
    print("putAll non-sendable value: " + e + ", errCode: " + e.code);
  }
  print("putAll partial value size: " + putAllPartialValueMap.size);
  for (const [key, value] of putAllPartialValueMap.entries()) {
    print("putAll partial value entries:[" + key + ", " + value + "]");
  }

  let invalidPutAllInputs = [
    ['undefined', undefined],
    ['null', null],
    ['array', [[1, 'one']]],
    ['arrayLike', { 0: [1, 'one'], length: 1 }],
    ['plain object', { 1: 'one' }],
    ['custom iterable', { [Symbol.iterator]: function() { return new Map([[1, 'one']]).entries(); } }],
    ['MapIterator', new Map([[1, 'one']]).entries()],
    ['SendableMapIterator', new SendableMap([[1, 'one']]).entries()],
    ['WeakMap', new WeakMap([[{}, 'value']])],
    ['primitive', 1],
  ];
  try {
    new SendableMap().putAll();
  } catch (e) {
    print("putAll invalid missing: " + e);
  }
  for (const [label, input] of invalidPutAllInputs) {
    try {
      new SendableMap().putAll(input);
    } catch (e) {
      print("putAll invalid " + label + ": " + e);
    }
  }

  let invalidReceiverEntriesMap = new SendableMap([
    [1, 'one'],
  ]);
  try {
    invalidReceiverEntriesMap.putAll(invalidReceiverEntriesMap.entries());
  } catch (e) {
    print("putAll invalid receiver.entries: " + e);
  }

  try {
    SendableMap.prototype.putAll.call({}, new Map([
      [1, 'one'],
    ]));
  } catch (e) {
    print("putAll bind error: " + e + ", errCode: " + e.code);
  }
  const unboundPutAll = putAllMap.putAll;
  const boundPutAll = unboundPutAll.bind({});
  try {
      print("" + boundPutAll(new Map([
        [1, 'one'],
      ])));
  } catch (err) {
      print("putAll bind error: " + err + ", errCode: " + err.code);
  }
  let prototypeCallMap = new SendableMap([
    [1, 'one'],
  ]);
  print("putAll prototype call return undefined: " +
    (SendableMap.prototype.putAll.call(prototypeCallMap, new Map([[2, 'two']])) === undefined));
  print("putAll prototype call size: " + prototypeCallMap.size);

  print("=== putAll concurrent case ===");
  let concurrentApiMap = new SendableMap([
    [1, 'one'],
    [2, 'two'],
  ]);
  try {
    concurrentApiMap.forEach((_: string, key: number, map: SendableMap) => {
      map.putAll(new Map([
        [3, 'three'],
      ]));
    });
  } catch (e) {
    print("PutAll Scenario[forEach]: " + e + ", errCode: " + e.code);
  }
  let concurrentApiSelfMap = new SendableMap([
    [1, 'one'],
    [2, 'two'],
  ]);
  try {
    concurrentApiSelfMap.forEach((_value: string, _key: number, map: SendableMap) => {
      map.putAll(concurrentApiSelfMap);
    });
  } catch (e) {
    print("PutAll Scenario[forEach self]: " + e + ", errCode: " + e.code);
  }
}

function TestSendableMapRemoveInterface(): void {
  print("Start Test SendableMap remove");

  print("=== remove common usage case ===");
  let removeMap = new SendableMap([
    [1, 'one'],
    [2, 'two'],
    ['undefined', undefined],
  ]);
  print("remove existing old: " + removeMap.remove(1));
  print("remove existing has: " + removeMap.has(1));
  print("remove existing size: " + removeMap.size);
  print("remove missing undefined: " + (removeMap.remove(99) === undefined));
  print("remove missing size: " + removeMap.size);
  for (const [key, value] of removeMap.entries()) {
    print("remove missing entries:[" + key + ", " + value + "]");
  }

  print("=== remove undefined case ===");
  print("remove undefined existed: " + removeMap.has('undefined'));
  print("remove undefined old: " + (removeMap.remove('undefined') === undefined));
  print("remove undefined has: " + removeMap.has('undefined'));

  print("=== remove SameValueZero case ===");
  let removeSameValueZeroMap = new SendableMap([
    [NaN, 'nan'],
    [+0, 'zero'],
  ]);
  print("remove NaN old: " + removeSameValueZeroMap.remove(NaN));
  print("remove NaN has: " + removeSameValueZeroMap.has(NaN));
  print("remove -0 old: " + removeSameValueZeroMap.remove(-0));
  print("remove zero has: " + removeSameValueZeroMap.has(+0));

  print("=== remove object case ===");
  let key1 = new SObject();
  let key2 = new SObject();
  let removeObjectMap = new SendableMap([
    [key1, 'value'],
  ]);
  print("remove other sendable object: " + (removeObjectMap.remove(key2) === undefined));
  print("remove same sendable object: " + removeObjectMap.remove(key1));

  print("=== remove exception case ===");
  try {
    SendableMap.prototype.remove.call({}, 'key');
  } catch (e) {
    print("remove bind error: " + e + ", errCode: " + e.code);
  }
  const unboundRemove = removeMap.remove;
  const boundRemove = unboundRemove.bind({});
  try {
      print("" + boundRemove(1));
  } catch (err) {
      print("remove bind error: " + err + ", errCode: " + err.code);
  }

  print("=== remove concurrent case ===");
  let concurrentApiMap = new SendableMap([
    [1, 'one'],
    [2, 'two'],
  ]);
  try {
    concurrentApiMap.forEach((_: string, key: number, map: SendableMap) => {
      map.remove(key);
    });
  } catch (e) {
    print("Remove Scenario[forEach]: " + e + ", errCode: " + e.code);
  }
}

TestSendableMapContainsValueInterface();
TestSendableMapPutInterface();
TestSendableMapPutAllInterface();
TestSendableMapRemoveInterface();

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
print(subSendableMap.containsValue('one'));
print(subSendableMap.put(1, 'ONE'));
print(subSendableMap.get(1));
print(subSendableMap.remove(1));
print(subSendableMap.has(1));

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
print(subSubSendableMap.containsValue('one'));
print(subSubSendableMap.put(1, 'ONE'));
print(subSubSendableMap.get(1));
print(subSubSendableMap.remove(1));
print(subSubSendableMap.has(1));

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
