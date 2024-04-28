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
 * @tc.name:mapget
 * @tc.desc: test Map.get
 * @tc.type: FUNC
 * @tc.require: issueI97M5M
 */
let map = new Map();

map.set('key', 'value');
print(map.get('key'))

for (let i = 0; i < 3; ++i) {
    map.set(i, -i);
}

for (let i = 0; i < 4; ++i) {
    let value = map.get(i);
    print(value);
}

map = new Map();
let key = Number.parseFloat("1392210229");
map.set(key, "success");
let value = map.get(key);
print(value);


function check(key) {
    let irHash = ArkTools.hashCode(key);
    let rtHash = ArkTools.hashCode(key, true);
    if (irHash != rtHash) {
        throw new Error("Mismatch hash for " + key + ": expected " + rtHash + ", but got " + irHash);
    }
}

function checkIntAsDouble(intKey) {
    intKey /= 2;
    intKey += 0.5;
    intKey *= 2;
    intKey -= 1;
    check(intKey);
}

check(0);
check(1);
check(1 << 30);
check((1 << 30) - 1);
check(-1);
check(1.5);
check(-1.5);
check(Number.EPSILON);
check(Number.NaN);
check(Number.MIN_VALUE);
check(Number.MAX_VALUE);
check(Number.MIN_SAFE_INTEGER);
check(Number.MIN_SAFE_INTEGER - 1);
check(Number.MAX_SAFE_INTEGER);
check(Number.MAX_SAFE_INTEGER + 1);
check(Number.NaN);
check(Number.POSITIVE_INFINITY);
check(Number.NEGATIVE_INFINITY);
check(Number.parseFloat("+0.0"));
check(Number.parseFloat("-0.0"));
check(true);
check(false);
check(undefined);
check(null);
check("");
check("ab");
check({});
check(12n);
checkIntAsDouble(0);
checkIntAsDouble(1);
checkIntAsDouble(-1);
checkIntAsDouble(-1234);
checkIntAsDouble(1234);
checkIntAsDouble(1 << 29);
checkIntAsDouble(-(1 << 29));
checkIntAsDouble(1 << 30);
checkIntAsDouble(-(1 << 30));
check(Symbol.iterator);

// regression test
check(Number.parseFloat("1392210229"));

/*
 * @tc.name:MapConstructor,species
 * @tc.desc: test MapConstructor,species
 * @tc.type: FUNC
 */
const maps = [
    new Map([["ark_stringKey", "ark_stringValue"]]),
    new Map([[1, "ark_numberValue"]]),
    new Map([[true, "ark_booleanValue"]]),
    new Map([[{}, "ark_objectValue"]]),
    new Map([[null, "ark_nullValue"]]),
    new Map([[undefined, "ark_undefinedValue"]]),
    new Map([[NaN, "ark_NaNValue"]]),
    new Map([[Infinity, "ark_infinityValue"]]),
    new Map([[-Infinity, "ark_negativeInfinityValue"]]),
    new Map([[RegExp("ark_regexKey"), "ark_regexValue"]]),
    new Map([[new Map(), "ark_mapValue"]]),
    new Map([[new Set(), "ark_setValue"]]),
    new Map([[Array.from([1, 2, 3]), "ark_arrayValue"]]),
    new Map([["ark_unicodeKey 🔑", "ark_unicodeValue 😀"]])
];

maps.forEach((map, index) => {
    print("Map " + (index + 1) + ":");
    map.forEach((value, key) => {
        print("Key: " + key + ", Value: " + value);
    });
});

let result =  Map[Symbol.species];
print(result);

/*
 * @tc.name:forEach,get,has,keys,set,clear
 * @tc.name:delete,values,size,entries
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

const newMap = new Map();

const keysArray = Array.from(combinedMap.keys());
keysArray.forEach(key => {
  print("Keys: " + key);
});

const valuesArray = Array.from(combinedMap.values());
valuesArray.forEach(value => {
  print("Value: " + value);
});

const entriesArray = Array.from(combinedMap.entries());
entriesArray.forEach(entry => {
  const [key, value] = entry;
  print("Key: " + key + ", Value: " + value);
});

combinedMap.forEach((value, key) => {
    const retrievedValue = combinedMap.get(key);
    const hasKey = combinedMap.has(key);
    newMap.set(key, value);
    print("Key: " + key + ", Retrieved Value: " + retrievedValue);
    print("Key: " + key + ", Exists: " + hasKey);
    combinedMap.delete(key);
});

print(combinedMap.size);
newMap.clear();
print(newMap.size);

const testMap = new Map([
  ["key1", "value1"],
  ["key2", "value2"],
]);
const emptyMap = new Map();

try {
  const abnormalMap = new Map(5);
} catch (error) {
  print("Caught an error: " + error);
}

try {
    const value = testMap.get(NaN);
    const result1 = testMap.has(NaN);
    const result2 = testMap.has();
    testMap.set(NaN, "value");
    testMap.set("key", "value1");
    testMap.set("key", "value2");
    emptyMap.clear();
    emptyMap.delete(NaN);
    print("Exception usage, but does not throw an error");
} catch (error) {
    print("Caught an error: " + error);
}

try {
  testMap.forEach(5);
} catch (error) {
  print("Caught an error: " + error);
}

try {
  testMap.forEach((value, key) => {
      if (key === "key2") {
          throw new Error("Encountered key2");
      }
      print("Key: " + key + ", Value: " + value);
  });
} catch (error) {
  print("Caught an error: " + error);
}

// Map.clear tests
map = new Map();
map.set(1, null);
map.set(2, null);
map.set(3, null);
let beginNotTought = map.entries();
let midNotTought = map.entries();
midNotTought.next();
let begin = map.entries(); // points to (1, null)
let mid = map.entries(); // points to (2, null)
mid.next();
let last = map.entries(); // points to (3, null)
last.next();
last.next();
let end = map.entries(); // points to the end
while (end.next().done) {
}
map.clear();
if (map.size != 0) {
    throw new Error("Map size must be 0");
}
if (!begin.next().done) {
    throw new Error("Invalid 'begin' iterator");
}
if (!mid.next().done) {
    throw new Error("Invalid 'mid' iterator");
}
if (!last.next().done) {
    throw new Error("Invalid 'last' iterator");
}
if (!end.next().done) {
    throw new Error("Invalid 'end' iterator");
}
map.set(-1, null);
map.set(-2, null);
map.set(-3, null);
let v = beginNotTought.next();
if (v.done) {
    throw new Error("Invalid 'beginNotTought' iterator");
}
if (v.value[0] != -1) {
    throw new Error("Invalid 'beginNotTought' iterator's value");
}
v = midNotTought.next();
if (v.done) {
    throw new Error("Invalid 'midNotTought' iterator");
}
if (v.value[0] != -1) {
    throw new Error("Invalid 'midNotTought' iterator's value");
}
if (!begin.next().done) {
    throw new Error("Invalid 'begin' iterator");
}
if (!mid.next().done) {
    throw new Error("Invalid 'mid' iterator");
}
if (!last.next().done) {
    throw new Error("Invalid 'last' iterator");
}
if (!end.next().done) {
    throw new Error("Invalid 'end' iterator");
}
