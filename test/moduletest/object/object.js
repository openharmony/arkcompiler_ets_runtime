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

class A {
    p1 = 1;
    p2 = 2;
    p3 = 3;
    constructor() {
        this.p4 = 4;
    }
}
var a1 = new A();
a1.jjj = 4;
print(ArkTools.getInlinedPropertiesCount(a1));
var a2 = new A();
a2.aaa1 = 5;
a2.aaa2 = 5;
a2.aaa3 = 5;
a2.aaa4 = 5;
a2.aaa5 = 5;
a2.aaa6 = 5;
print(ArkTools.getInlinedPropertiesCount(a2));
var a3 = new A();
var a4 = new A();
var a5 = new A();
var a6 = new A();
var a7 = new A();
print(ArkTools.getInlinedPropertiesCount(a7));
var a8 = new A();
print(a3.p2);
print(a7.p3);

for (let i = 0; i < 2000; i++) {
    var tmp = "jj" + i;
    a2[tmp] = i;
}
print(a2.aaa6);
print(a2.jj1000);

class B extends A {
    p5 = 5;
    constructor() {
        super();
        this.p6 = 6;
    }
}
var b1 = new B();
b1.jjj = 4;
print(ArkTools.getInlinedPropertiesCount(b1));
var b2 = new B();
var b3 = new B();
var b4 = new B();
var b5 = new B();
var b6 = new B();
var b7 = new B();
print(ArkTools.getInlinedPropertiesCount(b7));

class C extends Date {
    p5 = 5;
    constructor() {
        super();
        this.p6 = 6;
    }
}
var c1 = new C();
c1.jjj = 4;
print(ArkTools.getInlinedPropertiesCount(c1));
var c2 = new C();
var c3 = new C();
var c4 = new C();
var c5 = new C();
var c6 = new C();
var c7 = new C();
print(ArkTools.getInlinedPropertiesCount(c7));

function func1() {
    this.a1 = 1;
    this.a2 = 2;
    this.a3 = 2;
}
var f1 = new func1();
f1.jjj = 4;
print(ArkTools.getInlinedPropertiesCount(f1));
var f2 = new func1();
print(ArkTools.getInlinedPropertiesCount(f2));
var f3 = new func1();
f3.aaa1 = 5;
f3.aaa2 = 5;
f3.aaa3 = 5;
f3.aaa4 = 5;
f3.aaa5 = 5;
f3.aaa6 = 5;
var f4 = new func1();
var f5 = new func1();
var f6 = new func1();
var f7 = new func1();
print(ArkTools.getInlinedPropertiesCount(f7));

function func2() {
    this.a1 = 1;
    this.a2 = 2;
    this.a3 = 2;
}
var f8 = new func2();
print(ArkTools.getInlinedPropertiesCount(f8));
func2.prototype = A.prototype;
var f9 = new func2();
print(ArkTools.getInlinedPropertiesCount(f9));