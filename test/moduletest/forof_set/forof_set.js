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
 * @tc.name: forof_set
 * @tc.desc: test set forof
 * @tc.type: FUNC
 */
let comBinedSet = new Set([
    "ark_stringValue",
    1,
    true,
    {},
    null,
    undefined,
    NaN,
    Infinity,
    -Infinity,
    RegExp("ark_regexValue"),
    new Map(),
    new Set(),
    Array.from([1, 2, 3]),
    "ark_unicodeValue 😀"
]);

for (const key of comBinedSet.keys()) {
    print(key);
}

for (const value of comBinedSet.values()) {
    print(value);
}

for (const entry of comBinedSet.entries()) {
    print(entry);
}

let keyIterator = comBinedSet.keys();
let valueIterator = comBinedSet.values();
let entriesIterator = comBinedSet.entries();

print(keyIterator.next().value);
print(keyIterator.next().done);

print(valueIterator.next().value);
print(valueIterator.next().done);

while (!entriesIterator.next().done) {
}
print(entriesIterator.next().value);

let iterResult;
// start point is [true]
while (!(iterResult = keyIterator.next()).done) {
    print(iterResult.value);
    if (Object.is(iterResult.value, NaN)) {
        // update comBinedSet
        comBinedSet.delete(true);
        comBinedSet.delete("ark_unicodeValue 😀");
        comBinedSet.add("new_key");
    }
    if (iterResult.value == Infinity) {
        // check combinedSet
        if (!comBinedSet.has(true)) {
            print("[true] is deleted");
        }
    }
}

class MySet extends Set {
    constructor() {
        super();
    }
}

const newSet = new MySet([
    1,
    2,
    3,
]);

for (let value of newSet.values()) {
    print(value);
    if (value == 2) {
        break;
    }
}
