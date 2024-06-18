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
declare function print(str: any): string;
declare var ArkTools:any;
function foo() {
    return "pass";
}

function foo1(a: any) {
    return a + 1;
}

function foo2(a: any, b: any) {
    return a + b;
}

function foo3(a: any, b: any, c: any) {
    return a + b + c;
}

function foo4(a: any, b: any, c: any, d: any) {
    return a + b + c + d;
}

print(foo());
print(foo1(1));
print(foo1(1));
print(foo1(1));
print(foo2(1, 2));
print(foo3(1, 2, 3));
print(foo4(1, 2, 3, 4));

class A {
    public mode: number = 1;

    constructor(dt: number) {
        print(dt);
        print(new.target.name);
        const size = 50;
        this.mode = 4;
    }

    update (dt: number, dt1: number): void {
        print(dt);
        print(dt1);
    }
}

class B {
    public mode: number = 1;

    constructor(dt: number, dt1: number, dt2: number) {
        print(dt);
        print(dt1);
        print(dt2);
        print(new.target.name);
        const size = 50;
        this.mode = 4;
    }
}

class C {
    public mode: number = 1;

    constructor(dt: number, dt1: number, dt2: number, dt3: number, dt4: number, dt5: number, dt6: number, dt7: number) {
        print(new.target.name);
        print(dt);
        print(dt1);
        print(dt4);
        print(dt6);
        print(dt7);
        const size = 50;
        this.mode = 4;
    }
}

function funcv(value: number, value1: number, value2: number, value3: number, value4: number, value5: number, value6: number, value7: number): number {
    print(value);
    print(value1);
    print(value2);
    print(value3);
    print(value4);
    print(value5);
    print(value6);
    print(value7);
    return 100;
}

function func0(): number {
    return 110;
}

function func1(value: number): number {
    print(value);
    return value;
}

function func2(value: number, value1: number): number {
    print(value);
    print(value1);
    return value;
}

function func3(value: number, value1: number, value2: number): number {
    print(value);
    print(value1);
    print(value2);
    func1(value);
    return value;
}

function func4(value: number, value1: number, value2: number, value3: number): number {
    print(value);
    print(value1);
    print(value2);
    print(value3);
    return value;
}

function testNewTarget(value: number): number {
    print(new.target.name);
    return value;
}
var systems: A = new A(1);
var systems1: B = new B(2, 3);
var systems2: C = new C(3, 4, 5, 6, 7, 8);
print(func0());
func1();
func2(1);
func3("mytest", 2);
func4(3, 4, 5);
funcv(6, 7 , 8, 9);
systems.update(4);
var k = new testNewTarget(1);

function funcAsm(value: number, value1: number, value2: number): number {
    print(value);
    print(value1);
    print(value2);
    func2(value1, value2);
    func3(value1, value2);
    func4(value1);
    funcv(value, value1, value2, value);
    var s: A = new A(1, 4);
    var s1: B = new B(2, 3);
    var s2: C = new C(3, 4, 5, 6, 7, 8);
    var s3: C = new C(3, 4, 5, 6, 7, 8, 9, 10);
    return value;
}
ArkTools.removeAOTFlag(funcAsm);
funcAsm(1, 2);

class TestCallThis0 {
    foo(arg?: any) {
        print(arg);
    }
}

function testCallThis0() {
    let t = new TestCallThis0();
    t.foo();
}

testCallThis0();

class AA {
    constructor(owningView, propertyName) {
        owningView.id__();
    }
    id__() {
        return this.id_;
    }
}

class BB extends AA {
}

class Index {
    constructor() {
        // super(parent, __localStorage, elmtId);
        this.__message = new BB(this, 777);
        this.__showLazy = new BB(this, 888);
        this.__showNoLazy = new BB(this, 999);
    }
    id__() {
        return this.id_;
    }
}

new Index()

function fooo(a, b, ...args) {
    arguments.length;
}

fooo(1);
fooo(1, 2);
fooo(1, 2, 3);
fooo(1, 2, 3, 4, 5);

async function f206(a207, a208, a209) {
    let v210;
    try {v210 = a208(a207, a209, 2); } catch (e) { print(e instanceof TypeError) }
    await v210;
    return v210;
}
// @ts-ignore
f206(1, f206);
