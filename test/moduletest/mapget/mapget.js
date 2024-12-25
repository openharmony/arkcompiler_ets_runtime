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
assert_equal(map.get('key'),"value");

for (let i = 0; i < 3; ++i) {
    map.set(i, -i);
}
let objkeys1 = [];
for (let i = 0; i < 4; ++i) {
    let value = map.get(i);
    objkeys1.push(value);
}
assert_equal(objkeys1,[-0, -1, -2, undefined]);

map = new Map();
let key = Number.parseFloat("1392210229");
map.set(key, "success");
let value = map.get(key);
assert_equal(value,"success");


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

let objkeys2 = [];
maps.forEach((map, index) => {
    objkeys2.push("Map " + (index + 1) + ":");
    map.forEach((value, key) => {
        objkeys2.push("Key: " + key + ", Value: " + value);
    });
});
assert_equal(objkeys2,["Map 1:", "Key: ark_stringKey, Value: ark_stringValue", "Map 2:", "Key: 1, Value: ark_numberValue", "Map 3:", "Key: true, Value: ark_booleanValue", "Map 4:", "Key: [object Object], Value: ark_objectValue", "Map 5:", "Key: null, Value: ark_nullValue", "Map 6:", "Key: undefined, Value: ark_undefinedValue", "Map 7:", "Key: NaN, Value: ark_NaNValue", "Map 8:", "Key: Infinity, Value: ark_infinityValue", "Map 9:", "Key: -Infinity, Value: ark_negativeInfinityValue", "Map 10:", "Key: /ark_regexKey/, Value: ark_regexValue", "Map 11:", "Key: [object Map], Value: ark_mapValue", "Map 12:", "Key: [object Set], Value: ark_setValue", "Map 13:", "Key: 1,2,3, Value: ark_arrayValue", "Map 14:", "Key: ark_unicodeKey 🔑, Value: ark_unicodeValue 😀"]);

let result =  Map[Symbol.species];
assert_equal(result.toString(),'function Map() { [native code] }');

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
let objkeys3= [];
const keysArray = Array.from(combinedMap.keys());
keysArray.forEach(key => {
    objkeys3.push("Keys: " + key);
});
assert_equal(objkeys3,["Keys: ark_stringKey", "Keys: 1", "Keys: true", "Keys: [object Object]", "Keys: null", "Keys: undefined", "Keys: NaN", "Keys: Infinity", "Keys: -Infinity", "Keys: /ark_regexKey/", "Keys: [object Map]", "Keys: [object Set]", "Keys: 1,2,3", "Keys: ark_unicodeKey 🔑"]);

const valuesArray = Array.from(combinedMap.values());
let objkeys4 = [];
valuesArray.forEach(value => {
    objkeys4.push("Value: " + value);
});
assert_equal(objkeys4,["Value: ark_stringValue", "Value: ark_numberValue", "Value: ark_booleanValue", "Value: ark_objectValue", "Value: ark_nullValue", "Value: ark_undefinedValue", "Value: ark_NaNValue", "Value: ark_infinityValue", "Value: ark_negativeInfinityValue", "Value: ark_regexValue", "Value: ark_mapValue", "Value: ark_setValue", "Value: ark_arrayValue", "Value: ark_unicodeValue 😀"]);

const entriesArray = Array.from(combinedMap.entries());
let objkeys5 = [];
entriesArray.forEach(entry => {
  const [key, value] = entry;
  objkeys5.push("Key: " + key + ", Value: " + value);
});
assert_equal(objkeys5,["Key: ark_stringKey, Value: ark_stringValue", "Key: 1, Value: ark_numberValue", "Key: true, Value: ark_booleanValue", "Key: [object Object], Value: ark_objectValue", "Key: null, Value: ark_nullValue", "Key: undefined, Value: ark_undefinedValue", "Key: NaN, Value: ark_NaNValue", "Key: Infinity, Value: ark_infinityValue", "Key: -Infinity, Value: ark_negativeInfinityValue", "Key: /ark_regexKey/, Value: ark_regexValue", "Key: [object Map], Value: ark_mapValue", "Key: [object Set], Value: ark_setValue", "Key: 1,2,3, Value: ark_arrayValue", "Key: ark_unicodeKey 🔑, Value: ark_unicodeValue 😀"]);

let objkeys6= [];
combinedMap.forEach((value, key) => {
    const retrievedValue = combinedMap.get(key);
    const hasKey = combinedMap.has(key);
    newMap.set(key, value);
    objkeys6.push("Key: " + key + ", Retrieved Value: " + retrievedValue);
    objkeys6.push("Key: " + key + ", Exists: " + hasKey);
    combinedMap.delete(key);
});
assert_equal(objkeys6,["Key: ark_stringKey, Retrieved Value: ark_stringValue", "Key: ark_stringKey, Exists: true", "Key: 1, Retrieved Value: ark_numberValue", "Key: 1, Exists: true", "Key: true, Retrieved Value: ark_booleanValue", "Key: true, Exists: true", "Key: [object Object], Retrieved Value: ark_objectValue", "Key: [object Object], Exists: true", "Key: null, Retrieved Value: ark_nullValue", "Key: null, Exists: true", "Key: undefined, Retrieved Value: ark_undefinedValue", "Key: undefined, Exists: true", "Key: NaN, Retrieved Value: ark_NaNValue", "Key: NaN, Exists: true", "Key: Infinity, Retrieved Value: ark_infinityValue", "Key: Infinity, Exists: true", "Key: -Infinity, Retrieved Value: ark_negativeInfinityValue", "Key: -Infinity, Exists: true", "Key: /ark_regexKey/, Retrieved Value: ark_regexValue", "Key: /ark_regexKey/, Exists: true", "Key: [object Map], Retrieved Value: ark_mapValue", "Key: [object Map], Exists: true", "Key: [object Set], Retrieved Value: ark_setValue", "Key: [object Set], Exists: true", "Key: 1,2,3, Retrieved Value: ark_arrayValue", "Key: 1,2,3, Exists: true", "Key: ark_unicodeKey 🔑, Retrieved Value: ark_unicodeValue 😀", "Key: ark_unicodeKey 🔑, Exists: true"]);

assert_equal(combinedMap.size,0);
newMap.clear();
assert_equal(newMap.size,0);

const testMap = new Map([
  ["key1", "value1"],
  ["key2", "value2"],
]);
const emptyMap = new Map();

try {
  const abnormalMap = new Map(5);
  assert_unreachable();
} catch (error) {
  assert_equal("Caught an error: "+ error, "Caught an error: TypeError: iterable is not object");
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
} catch (error) {
    assert_unreachable();
}

try {
  testMap.forEach(5);
  assert_unreachable();
} catch (error) {
  assert_equal("Caught an error: "+ error, "Caught an error: TypeError: obj is not Callable");
}

try {
  testMap.forEach((value, key) => {
      if (key === "key2") {
          throw new Error("Encountered key2");
      }
  });
} catch (error) {
  assert_equal("Caught an error: "+ error, "Caught an error: Error: Encountered key2");
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

test_end();