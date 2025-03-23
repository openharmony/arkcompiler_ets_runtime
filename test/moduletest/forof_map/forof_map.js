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
 * @tc.name: forof_map
 * @tc.desc: test map forof 
 * @tc.type: FUNC
 */
const combinedMap = new Map([
    ["ark_stringKey", "ark_stringValue"],
    [1, "ark_numberValue"],
    [true, "ark_booleanValue"],
    [{}, "ark_objectValue"],
    [null, "ark_nullValue"],
    [undefined, "ark_undefinedValue"],
    [NaN, "ark_NaNValue"],
    [Infinity, "ark_infinityValue"],
    [-Infinity, "ark_negativeInfinityValue"],
    [RegExp("ark_regexKey"), "ark_regexValue"],
    [new Map(), "ark_mapValue"],
    [new Set(), "ark_setValue"],
    [Array.from([1, 2, 3]), "ark_arrayValue"],
    ["ark_unicodeKey 🔑", "ark_unicodeValue 😀"]
]);

for (const key of combinedMap.keys()) {
  print(key);
}

for (const value of combinedMap.values()) {
  print(value);
}

for (const entries of combinedMap.entries()) {
  print(entries);
}

let keyIterator = combinedMap.keys();
let valueIterator = combinedMap.values();
let entriesIterator = combinedMap.entries();

combinedMap.set("ark_stringKey", "ark_stringValueUpdated");
print(valueIterator.next().value);

combinedMap.delete("ark_stringKey");
print(keyIterator.next().value);

print(entriesIterator.next().value);
print(entriesIterator.next().value);

// update map when getting iterator next
let iterResult;
while (!(iterResult = entriesIterator.next()).done) {
    print(iterResult.value);
    
    if (iterResult.value[0] == undefined) {
        combinedMap.clear();
    }
}

class MyMap extends Map {
    constructor() {
        super();
    }
}

const newMap = new MyMap([
    [1, 1],
    [2, 2],
    ['updatepoint1', 'updatepoint1'],
    [3, 3],
    [4, 4],
    ['updatepoint2', 'updatepoint2'],
    [5, 5],
    [6, 6],
]);

for (let value of newMap.values()) {
    print(value);
    if (value == 'updatepoint1') {
        // refore updatepoint1
        newMap.set(2, 3); 
    }

    if (value == 'updatepoint2') {
        // after pdatepoint2
        newMap.set(5, 10);
        newMap.delete(6);
    }
}

