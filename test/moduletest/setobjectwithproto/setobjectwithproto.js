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

/*
 * @tc.name:setobjectwithproto
 * @tc.desc:test set object with proto
 * @tc.type: FUNC
 * @tc.require: issueI5NO8G
 */
var object = {
    __proto__: null
};

assert_equal(Object.getPrototypeOf(object),null);

class C32 extends String {

}
let obj1 = new C32();
assert_equal(obj1.__proto__ == C32.prototype,true);
assert_equal(C32.__proto__ == String,true);
C32.__proto__ = Array;
let obj2 = new C32();
assert_equal(obj2.__proto__ == C32.prototype,true);
assert_equal(C32.__proto__ == Array,true);

Float64Array.__proto__ = Float32Array
new Float64Array();

var normalValues = [
    1,
    true,
    'string',
    Symbol()
];

function getObjects() {
    function func() {}
    return [
      func,
      new func(),
      {x: 5},
      /regexp/,
      ['array'],
      new Date(),
      new Number(1),
      new Boolean(true),
      new String('str'),
      Object(Symbol())
    ];
}

function TestSetPrototypeOf1() {
    var object = {};
    var oldProto = {
      x: 'old x',
      y: 'old y'
    };
    Object.setPrototypeOf(object, oldProto);
    assert_equal(object.x,"old x");
    assert_equal(object.y,"old y");
    var newProto = {
      x: 'new x'
    };
    Object.setPrototypeOf(object, newProto);
    assert_equal(object.x,'new x');
}
TestSetPrototypeOf1();

function TestSetPrototypeOf2() {
    for (var i = 0; i < normalValues.length; i++) {
      var value = normalValues[i];
      var proto = Object.getPrototypeOf(value);
      if (Object.setPrototypeOf(value, {}) != value) {
        return false;
      }
      if (proto != Object.getPrototypeOf(value)) {
        return false;
      }
    }
    return true;
}

assert_equal(TestSetPrototypeOf2(),true);

function TestSetPrototypeOf(object, proto) {
    return (Object.setPrototypeOf(object, proto) === object) &&
           (Object.getPrototypeOf(object) === proto);
}

function TestSetPrototypeOf3() {
    var objects1 = getObjects();
    var objects2 = getObjects();
    for (var i = 0; i < objects1.length; i++) {
        for (var j = 0; j < objects2.length; j++) {
        if (!TestSetPrototypeOf(objects1[i], objects2[j])) {
            return false;
        }
        }
    }
    return true;
}
assert_equal(TestSetPrototypeOf3(),true);

function TestSetPrototypeOf4() {
    let log =  [];
    const proto = {
        _value: null,
        set [888](val) {
            log.push("set");
            this._value = val;
        },
        get [888]() {
            return this._value;
        }
    };
    const obj = Object.create(proto);
    obj[888] = 'inherited';
    assert_equal(log.length, 1);
    assert_equal(log[0], "set");
}
TestSetPrototypeOf4();


{
    // Test Map - value -0.0 should be stored as-is (not canonicalized)
    // Note: Only MAP KEYS are canonicalized (-0 → +0), values are stored as-is
    const map = new Map();
    map.set("key", -0.0);
    const mapValue = map.get("key");
    assert_equal(mapValue === 0, true);
    assert_equal(Object.is(mapValue, 0), false);
    assert_equal(Object.is(mapValue, -0), true);

    // Test Set - SameValueZero for -0.0 (SameValueZero treats 0 and -0 as equal)
    // Expected: Set([0, -0]) should have only 1 element because -0 is canonicalized to 0
    const set = new Set([0, -0]);
    const setVisited = [];
    set.forEach(value => setVisited.push(value));
    assert_equal(setVisited.length, 1);
    assert_equal(Object.is(setVisited[0], 0), true);

    // Test Map with regular elements
    const map2 = new Map();
    map2.set("a", 1);
    map2.set("b", 2);
    map2.set("c", 3);
    assert_equal(map2.get("a"), 1);
    assert_equal(map2.get("b"), 2);
    assert_equal(map2.get("c"), 3);

    // Test Map with NaN (SameValueZero treats NaN as equal to NaN)
    const map3 = new Map();
    map3.set("nan", NaN);
    assert_equal(Object.is(map3.get("nan"), NaN), true);

    // Test Map with Infinity
    const map4 = new Map();
    map4.set("inf", Infinity);
    map4.set("negInf", -Infinity);
    assert_equal(map4.get("inf"), Infinity);
    assert_equal(map4.get("negInf"), -Infinity);

    // Test Set with regular elements
    const set2 = new Set();
    set2.add(1);
    set2.add(2);
    set2.add(3);
    assert_equal(set2.has(1), true);
    assert_equal(set2.has(2), true);
    assert_equal(set2.has(3), true);

    // Test Set with NaN (SameValueZero treats NaN as equal to NaN)
    // Expected: Set([NaN, NaN]) should have only 1 element
    const set3 = new Set([NaN, NaN]);
    let set3Count = 0;
    set3.forEach(() => set3Count++);
    assert_equal(set3Count, 1);

    // Test Map with 0 and -0 as keys
    // After CanonicalizeKeyedCollectionKey(-0) = +0, SameValueZero(0, 0) = true
    // So map.set(0, "zero") then map.set(-0, "negZero") should UPDATE the same entry
    const map5 = new Map();
    map5.set(0, "zero");
    map5.set(-0, "negZero");
    assert_equal(map5.get(0), "negZero");
    assert_equal(map5.get(-0), "negZero");
    assert_equal(map5.size, 1);
    assert_equal([...map5.keys()].length, 1);

    // Test Set with 0 and -0
    // SameValueZero treats 0 and -0 as same value
    // Expected: Set([0, -0]) should have only 1 element
    const set4 = new Set([0, -0]);
    let set4Count = 0;
    set4.forEach(() => set4Count++);
    assert_equal(set4Count, 1);

    // Additional verification
    assert_equal(Object.is(-0, 0),false);
    assert_equal(-0 === 0, true);
    assert_equal(Object.is(NaN, NaN), true);
    assert_equal(NaN === NaN, false);

}

test_end();