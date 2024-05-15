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
 * @tc.name:object
 * @tc.desc:test object
 * @tc.type: FUNC
 * @tc.require: issueI9DUIF
 */

// test case for getPrototypeOf
var _handler, _target;
var target = {};
var handler = {
  getPrototypeOf: function(t) {
    _handler = this;
    _target = t;
    return {};
  }
};

var prototypeObj = {};
var obj01 = Object.create(prototypeObj);
var obj02 = new Proxy(target, handler);

Object.getPrototypeOf(obj01);
print(_handler === handler);
print(_target === target);

Object.getPrototypeOf(obj02);
print(_handler === handler);
print(_target === target);

let obj1 = new Int8Array([-5, 10, 20, 30, 40, 50, 60.6]);
let obj2 = {
    a: 1,
    b: 2,
    c: 3,
};
Object.defineProperty(obj2, '23', {
    value: 31,
    enumerable: false,
})
Object.defineProperty(obj2, 'abc', {
    value: 31,
    enumerable: false,
})
let obj3 = {
    a: 1,
    b: 2,
    c: 3,
};
let obj4 = {
    1: 1,
    2: 2,
    3: 3,
};
let obj5 = {
    1: 1,
    2: 2,
    3: 3,
    a: 1,
    b: 2,
    c: 3,
};
let obj6 = {
    a: 1,
    b: 2,
    c: 3,
};
const a = Symbol('a');
const b = Symbol.for('b');
obj6[a] = 'aSymbol';
obj6[b] = 'bSymbol';
let obj7 = Object.create(
    {},
    {
        getBar: {
            value() {
                return 55;
            },
            writable: false,
            enumerable: false,
            configurable: false,
        },
    },
);
let obj11 = new Int8Array([-5, 10, 20, 30, 40, 50, 60.6]);
let obj12 = {
    a: 1,
    b: 2,
    c: 3,
};
Object.defineProperty(obj12, '23', {
    value: 31,
    enumerable: false,
})
Object.defineProperty(obj12, 'abc', {
    value: 31,
    enumerable: false,
})
let obj13 = {
    a: 1,
    b: 2,
    c: 3,
};
let obj14 = new Object();
Object.preventExtensions(obj14);
let obj15 = {
    1: 1,
    2: 2,
    3: 3,
};
Object.preventExtensions(obj15);
let obj16 = {
    1: 1,
    2: 2,
    3: 3,
    a: 1,
    b: 2,
    c: 3,
};
Object.preventExtensions(obj16);
let obj17 = Object.create(
    {},
    {
        getBar: {
            value() {
                return 55;
            },
            configurable: true,
            writable: true,
        },
    },
);
Object.preventExtensions(obj17);
let obj18 = Object.create(
    {},
    {
        getBar: {
            value() {
                return 55;
            },
            configurable: false,
            writable: true,
        },
    },
);
Object.preventExtensions(obj18);
let obj19 = Object.create(
    {},
    {
        getBar: {
            value() {
                return 55;
            },
            configurable: false,
            writable: false,
        },
    },
);
Object.preventExtensions(obj19);

// test for getOwnPropertyNames()
print(Object.getOwnPropertyNames(obj1));
print(Object.getOwnPropertyNames(obj2));
print(Object.getOwnPropertyNames(obj3));
print(Object.getOwnPropertyNames(obj4));
print(Object.getOwnPropertyNames(obj5));
print(Object.getOwnPropertyNames(obj6));
// test for getOwnPropertySymbols()
print(Object.getOwnPropertySymbols(obj1).length);
print(Object.getOwnPropertySymbols(obj2).length);
print(Object.getOwnPropertySymbols(obj3).length);
print(Object.getOwnPropertySymbols(obj4).length);
print(Object.getOwnPropertySymbols(obj5).length);
print(Object.getOwnPropertySymbols(obj6).length);
// test for isFrozen()
print(Object.isFrozen(obj11));
print(Object.isFrozen(obj12));
print(Object.isFrozen(obj13));
print(Object.isFrozen(obj14));
print(Object.isFrozen(obj15));
print(Object.isFrozen(obj16));
print(Object.isFrozen(obj17));
print(Object.isFrozen(obj18));
print(Object.isFrozen(obj19));
// test for isSealed()
print(Object.isSealed(obj11));
print(Object.isSealed(obj12));
print(Object.isSealed(obj13));
print(Object.isSealed(obj14));
print(Object.isSealed(obj15));
print(Object.isSealed(obj16));
print(Object.isSealed(obj17));
print(Object.isSealed(obj18));
print(Object.isSealed(obj19));
// test for getOwnPropertyDescriptors()
let desc1 = Object.getOwnPropertyDescriptors(obj1);
let desc2 = Object.getOwnPropertyDescriptors(obj2);
let desc3 = Object.getOwnPropertyDescriptors(obj3);
let desc4 = Object.getOwnPropertyDescriptors(obj4);
let desc5 = Object.getOwnPropertyDescriptors(obj5);
let desc6 = Object.getOwnPropertyDescriptors(obj6);
let desc7 = Object.getOwnPropertyDescriptors(obj7);
print(JSON.stringify(desc1[0]));
print(JSON.stringify(desc2.abc));
print(JSON.stringify(desc3.a));
print(JSON.stringify(desc4[1]));
print(JSON.stringify(desc5[3]));
print(JSON.stringify(desc6.c));
print(JSON.stringify(desc7.getBar));