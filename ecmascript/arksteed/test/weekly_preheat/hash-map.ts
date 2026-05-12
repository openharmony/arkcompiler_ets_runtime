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

let inDebug = false;
function log(str: string): void {
  if (inDebug) {
    print(str);
  }
}

const DEFAULT_SIZE: number = 16;
const NUMBER_THIRTY: number = 30;
const NUMBER_SIXTEEN: number = 16;
const NUMBER_TWO: number = 2;
const NUMBER_FOUR: number = 4;
const NUMBER_EIGHT: number = 8;
const NUMBER_075PER: number = 0.75;
const NUMBER_THIRTYONE: number = 31;
const NUMBER_TEST_VALUE: number = 42;
const NUMBER_REPEAT_COUNT: number = 5;
const NUMBER_BENCHMARK_REPEAT_COUNT: number = 20;
const NUMBER_TIME_CUT: number = 1000;

class HashMap {
  calculateCapacity(x: number): number {
    if (x >= 1 << NUMBER_THIRTY) {
      return 1 << NUMBER_THIRTY;
    }
    if (x === 0) {
      return NUMBER_SIXTEEN;
    }
    x = x - 1;
    x |= x >> 1;
    x |= x >> NUMBER_TWO;
    x |= x >> NUMBER_FOUR;
    x |= x >> NUMBER_EIGHT;
    x |= x >> NUMBER_SIXTEEN;
    return x + 1;
  }

  computeHashCode(key: undefined | string | boolean | number): number {
    switch (typeof key) {
      case 'undefined':
        return 0;
      case 'boolean':
        return 0;
      case 'number':
        return key;
      case 'string':
        let h = 0;
        let len = key.length;
        for (let index = 0; index < len; index++) {
          h = (((NUMBER_THIRTYONE * h) | 0) + key.charCodeAt(index)) | 0;
        }
        return h;
      default:
        throw new Error('Internal error: Bad JavaScript value type');
    }
  }

  equals(a: number, b: number): boolean {
    if (typeof a !== typeof b) {
      return false;
    }
    switch (typeof a) {
      case 'number':
        return a === b;
      case 'object':
        if (a == null) {
          return b == null;
        }

      case 'function':
        switch (typeof b) {
          case 'object':
          case 'function':
            return a === b;
          default:
            return false;
        }
      default:
        return a === b;
    }
  }

  capacity: number;
  elementCount: number;
  elementData: Array<Entry>;
  loadFactor: number;
  modCount: number;
  threshold: number;

  constructor(capacity: number = DEFAULT_SIZE, loadFactor: number = NUMBER_075PER) {
    this.capacity = this.calculateCapacity(capacity);
    this.elementCount = 0;
    this.elementData = new Array(this.capacity);
    this.loadFactor = loadFactor;
    this.modCount = 0;
    this.computeThreshold();
  }

  computeThreshold(): void {
    this.threshold = (this.elementData.length * this.loadFactor) | 0;
  }

  clear(): void {
    if (this.elementCount == null) {
      return;
    }
    this.elementCount = 0;
    for (let i = this.elementData.length; i >= 0; i--) {
      this.elementData[i] = null;
    }
    this.modCount++;
  }

  clone(): HashMap {
    let result = new HashMap(this.elementData.length, this.loadFactor);
    result.putAll(this);
    return result;
  }

  containsValue(value: number): boolean {
    for (let entry of this.elementData) {
      let currentEntry = entry;
      while (currentEntry != null) {
        if (this.equals(value, currentEntry.value)) {
          return true;
        }
        currentEntry = entry.next;
      }
    }
    return false;
  }

  entrySet(): EntrySet {
    return new EntrySet(this);
  }

  get(key: number): number {
    let entry = this.getEntry(key);
    if (entry != null) {
      return entry.value;
    }
    return null;
  }

  getEntry(key: number): Entry {
    let hash = this.computeHashCode(key);
    let index = hash & (this.elementData.length - 1);
    return this.findKeyEntry(key, index, hash);
  }

  findKeyEntry(key: number, index: number, keyHash: number): Entry {
    let entry = this.elementData.length === 0 ? null : this.elementData[index];
    while (entry != null && (entry.origKeyHash !== keyHash || !this.equals(key, entry.key))) {
      entry = entry.next;
    }
    return entry;
  }

  isEmpty(): boolean {
    return this.elementCount === 0;
  }

  put(key: number, value: number): number {
    let hash = this.computeHashCode(key);
    let index = hash & (this.elementData.length - 1);
    let entry = this.findKeyEntry(key, index, hash);
    if (!entry) {
      this.modCount++;
      entry = this.createHashedEntry(key, index, hash);
      this.elementCount += 1;
      if (this.elementCount > this.threshold) {
        this.rehash(0);
      }
    }
    let result = entry.value;
    entry.value = value;
    // if (inDebug) {
    //   if (key % 10000 == 0 ) {
    //       log("put:" + entry.toString());
    //   }
    // }
    return result;
  }

  createHashedEntry(key: number, index: number, hash: number): Entry {
    let entry = new Entry(key, hash, null);
    entry.next = this.elementData[index];
    this.elementData[index] = entry;
    return entry;
  }

  putAll(map: HashMap): void {
    if (map.isEmpty()) {
      return;
    }
    let iterator = map.entrySet().iterator();
    while (iterator.hasNext()) {
      let entry = iterator.next();
      this.put(entry.key, entry.value);
    }
  }

  rehash(capacity: number): void {
    if (capacity <= 0) {
      capacity = this.elementData.length;
    }
    let length = this.calculateCapacity(!capacity ? 1 : capacity << 1);
    let newData = new Array<Entry>(length);
    for (let i = 0; i < this.elementData.length; ++i) {
      let entry = this.elementData[i];
      this.elementData[i] = null;
      while (entry) {
        let index = entry.origKeyHash & (length - 1);
        let next = entry.next;
        entry.next = newData[index];
        newData[index] = entry;
        entry = next;
      }
    }
    this.elementData = newData;
    this.computeThreshold();
  }

  remove(key: number): number {
    let entry = this.removeEntryForKey(key);
    if (entry == null) {
      return null;
    }
    return entry.value;
  }

  removeEntry(entry: Entry): void {
    let index = entry.origKeyHash & (this.elementData.length - 1);
    let current = this.elementData[index];
    if (current === entry) {
      this.elementData[index] = entry.next;
    } else {
      while (current.next !== entry) {
        current = current.next;
      }
      current.next = entry.next;
    }
    this.modCount++;
    this.elementCount--;
  }

  removeEntryForKey(key: number): Entry {
    let hash = this.computeHashCode(key);
    let index = hash & (this.elementData.length - 1);
    let entry = this.elementData[index];
    let last: Entry = null;
    while (entry != null && !(entry.origKeyHash === hash && this.equals(key, entry.value))) {
      last = entry;
      entry = entry.next;
    }
    if (entry == null) {
      return null;
    }
    if (last == null) {
      this.elementData[index] = entry.next;
    } else {
      last.next = entry.next;
    }
    this.modCount++;
    this.elementCount--;
    return entry;
  }

  size(): number {
    return this.elementCount;
  }

  values(): ValueCollection {
    return new ValueCollection(this);
  }
}

class Entry {
  key: number;
  value: number;
  origKeyHash: number;
  next: Entry;

  constructor(key: number, hash: number, value: number) {
    this.key = key;
    this.value = value;
    this.origKeyHash = hash;
  }

  clone(): Entry {
    let result = new Entry(this.key, this.value, this.origKeyHash);
    if (this.next != null) {
      result.next = this.next.clone();
    }
    return result;
  }
  toString(): string {
    return this.key + '=' + this.value;
  }
  getKey(): number {
    return this.key;
  }
  getValue(): number {
    return this.value;
  }
}

class AbstractMapIterator {
  associatedMap: HashMap;
  expectedModCount: number = 0;
  futureEntry: Entry = null;
  currentEntry: Entry = null;
  prevEntry: Entry = null;
  position: number = 0;

  constructor(map: HashMap) {
    this.associatedMap = map;
    this.expectedModCount = map.modCount;
  }

  hasNext(): boolean {
    if (this.futureEntry != null) {
      return true;
    }
    while (this.position < this.associatedMap.elementData.length) {
      if (!this.associatedMap.elementData[this.position]) {
        this.position++;
      } else {
        return true;
      }
    }
    return false;
  }

  checkConcurrentMod(): void {
    if (this.expectedModCount !== this.associatedMap.modCount) {
      throw new Error('Concurrent HashMap modification detected');
    }
  }
  makeNext(): void {
    this.checkConcurrentMod();
    if (this.hasNext() == null) {
      throw new Error('No such element');
    }
    if (this.futureEntry == null) {
      this.currentEntry = this.associatedMap.elementData[this.position++];
      this.futureEntry = this.currentEntry.next;
      this.prevEntry = null;
      return;
    }
    if (this.currentEntry != null) {
      this.prevEntry = this.currentEntry;
    }
    this.currentEntry = this.futureEntry;
    this.futureEntry = this.futureEntry.next;
  }

  remove(): void {
    this.checkConcurrentMod();
    if (this.currentEntry == null) {
      throw new Error('Illegal state');
    }
    if (this.prevEntry == null) {
      let index = this.currentEntry.origKeyHash & (this.associatedMap.elementData.length - 1);
      this.associatedMap.elementData[index] = this.associatedMap.elementData[index].next;
    } else {
      this.prevEntry.next = this.currentEntry.next;
    }
    this.currentEntry = null;
    this.expectedModCount++;
    this.associatedMap.modCount++;
    this.associatedMap.elementCount--;
  }
}

class ValueIterator extends AbstractMapIterator {
  next(): Entry {
    this.makeNext();
    return this.currentEntry;
  }
}

class EntrySet {
  associatedMap: HashMap;

  constructor(map: HashMap) {
    this.associatedMap = map;
  }

  size(): number {
    return this.associatedMap.elementCount;
  }

  clear(): void {
    this.associatedMap.clear();
  }

  iterator(): ValueIterator {
    return new ValueIterator(this.associatedMap);
  }
}

class ValueCollection {
  associatedMap: HashMap;

  constructor(map: HashMap) {
    this.associatedMap = map;
  }

  contains(object: number): boolean {
    return this.associatedMap.containsValue(object);
  }

  size(): number {
    return this.associatedMap.elementCount;
  }

  clear(): void {
    this.associatedMap.clear();
  }

  iterator(): ValueIterator {
    return new ValueIterator(this.associatedMap);
  }
}

/*
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  run(): void {
    let map = new HashMap();
    let COUNT = 90000;
    for (let i = 0; i < COUNT; i++) {
      map.put(i, NUMBER_TEST_VALUE);
    }
    let result = 0;
    for (let j = 0; j < NUMBER_REPEAT_COUNT; j++) {
      for (let i = 0; i < COUNT; i++) {
        result += map.get(i);
        // if (inDebug) {
        //   if (i % 10000 == 0) {
        //     let value = map.get(i);
        //     log("get:" + i + "=" + value);
        //   }
        // }
      }
    }

    let keySum = 0;
    let valueSum = 0;
    let iterator = map.entrySet().iterator();
    while (iterator.hasNext()) {
      let entry = iterator.next();
      keySum += entry.key;
      valueSum += entry.value;
    }
  }

  /**
   * @Benchmark
   */
  runIteration(): void {
    let startTime = currentTimestamp16();
    for (let j = 0; j < NUMBER_BENCHMARK_REPEAT_COUNT; ++j) {
      this.run();
    }
    let endTime = currentTimestamp16();
    let time = endTime - startTime;
    print(`hash-map: ms = ${time / NUMBER_TIME_CUT}`);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new Benchmark().runIteration();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new Benchmark().runIteration();

function currentTimestamp16(): number {
  return ArkTools.timeInUs();
}

declare interface ArkTools {
  timeInUs(args: number): number;
}
