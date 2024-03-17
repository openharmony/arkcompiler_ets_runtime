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
declare function assert_equal(a: Object, b: Object):void;
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

assert_equal(foo(), "pass");
assert_equal(foo1(1), 2);
assert_equal(foo1(1), 2);
assert_equal(foo1(1), 2);
assert_equal(foo2(1, 2), 3);
assert_equal(foo3(1, 2, 3), 6);
assert_equal(foo4(1, 2, 3, 4), 10);

var classTestNameA = "unassigned";
var classTestNumA1 = -1;
var classTestNumA2 = -1;
class A {
    public mode: number = 1;

    constructor(dt: number) {
        classTestNumA1 = dt;
        classTestNameA = new.target.name;
        const size = 50;
        this.mode = 4;
    }

    update (dt: number, dt1: number): void {
        classTestNumA1 = dt;
        classTestNumA2 = dt1;
    }
}

var classTestNameB = "unassigned";
var classTestNumB1 = -1;
var classTestNumB2 = -1;
var classTestNumB3 = -1;
class B {
    public mode: number = 1;

    constructor(dt: number, dt1: number, dt2: number) {
        classTestNumB1 = dt;
        classTestNumB2 = dt1;
        classTestNumB3 = dt2;
        classTestNameB = new.target.name;
        const size = 50;
        this.mode = 4;
    }
}

var classTestNameC = "unassigned";
var classTestNumC1 = -1;
var classTestNumC2 = -1;
var classTestNumC3 = -1;
var classTestNumC4 = -1;
var classTestNumC5 = -1;
var classTestNumC6 = -1;
var classTestNumC7 = -1;
var classTestNumC8 = -1;
class C {
    public mode: number = 1;

    constructor(dt: number, dt1: number, dt2: number, dt3: number, dt4: number, dt5: number, dt6: number, dt7: number) {
        classTestNameC = new.target.name;
        classTestNumC1 = dt;
        classTestNumC2 = dt1;
        classTestNumC5 = dt4;
        classTestNumC7 = dt6;
        classTestNumC8 = dt7;
        const size = 50;
        this.mode = 4;
    }
}

var funcvTestNum1 = -1;
var funcvTestNum2 = -1;
var funcvTestNum3 = -1;
var funcvTestNum4 = -1;
var funcvTestNum5 = -1;
var funcvTestNum6 = -1;
var funcvTestNum7 = -1;
var funcvTestNum8 = -1;
function funcv(value: number, value1: number, value2: number, value3: number, value4: number, value5: number, value6: number, value7: number): number {
    funcvTestNum1 = value;
    funcvTestNum2 = value1;
    funcvTestNum3 = value2;
    funcvTestNum4 = value3;
    funcvTestNum5 = value4;
    funcvTestNum6 = value5;
    funcvTestNum7 = value6;
    funcvTestNum8 = value7;
    return 100;
}

function func0(): number {
    return 110;
}

var func1TestNum = -1;
function func1(value: number): number {
    func1TestNum = value;
    return value;
}

var func2TestNum1 = -1;
var func2TestNum2 = -1;
function func2(value: number, value1: number): number {
    func2TestNum1 = value;
    func2TestNum2 = value1;
    return value;
}

var func3TestNum1 = -1;
var func3TestNum2 = -1;
var func3TestNum3 = -1;
function func3(value: number, value1: number, value2: number): number {
    func3TestNum1 = value;
    func3TestNum2 = value1;
    func3TestNum3 = value2;
    func1(value);
    return value;
}

var func4TestNum1 = -1;
var func4TestNum2 = -1;
var func4TestNum3 = -1;
var func4TestNum4 = -1;
function func4(value: number, value1: number, value2: number, value3: number): number {
    func4TestNum1 = value;
    func4TestNum2 = value1;
    func4TestNum3 = value2;
    func4TestNum4 = value3;
    return value;
}

var classTestName = "unassigned";
function testNewTarget(value: number): number {
    classTestName = new.target.name;
    return value;
}
var systems: A = new A(1);
assert_equal(classTestNumA1, 1);
assert_equal(classTestNameA, "A");
var systems1: B = new B(2, 3);
assert_equal(classTestNumB1, 2);
assert_equal(classTestNumB2, 3);
assert_equal(classTestNumB3, undefined);
assert_equal(classTestNameB, "B");
var systems2: C = new C(3, 4, 5, 6, 7, 8);
assert_equal(classTestNameC, "C");
assert_equal(classTestNumC1, 3);
assert_equal(classTestNumC2, 4);
assert_equal(classTestNumC5, 7);
assert_equal(classTestNumC7, undefined);
assert_equal(classTestNumC8, undefined);
assert_equal(func0(), 110);
func1();
assert_equal(func1TestNum, undefined);
func2(1);
assert_equal(func2TestNum1, 1);
assert_equal(func2TestNum2, undefined);
func3("mytest", 2);
assert_equal(func3TestNum1, "mytest");
assert_equal(func3TestNum2, 2);
assert_equal(func3TestNum3, undefined);
assert_equal(func1TestNum, "mytest");
func4(3, 4, 5);
assert_equal(func4TestNum1, 3);
assert_equal(func4TestNum2, 4);
assert_equal(func4TestNum3, 5);
assert_equal(func4TestNum4, undefined);
funcv(6, 7 , 8, 9);
assert_equal(funcvTestNum1, 6);
assert_equal(funcvTestNum2, 7);
assert_equal(funcvTestNum3, 8);
assert_equal(funcvTestNum4, 9)
assert_equal(funcvTestNum5, undefined);
assert_equal(funcvTestNum6, undefined);
assert_equal(funcvTestNum7, undefined);
assert_equal(funcvTestNum8, undefined)
systems.update(4);
assert_equal(classTestNumA1, 4)
assert_equal(classTestNumA2, undefined);
var k = new testNewTarget(1);
assert_equal(classTestName, "testNewTarget");

var funcAsmTestNum1 = -1;
var funcAsmTestNum2 = -1;
var funcAsmTestNum3 = -1;
function funcAsm(value: number, value1: number, value2: number): number {
    funcAsmTestNum1 = value;
    funcAsmTestNum2 = value1;
    funcAsmTestNum3 = value2;
    func2(value1, value2);
    assert_equal(func2TestNum1, 2);
    assert_equal(func2TestNum2, undefined);
    func3(value1, value2);
    assert_equal(func3TestNum1, 2);
    assert_equal(func3TestNum2, undefined);
    assert_equal(func3TestNum3, undefined);
    assert_equal(func1TestNum, 2);
    func4(value1);
    assert_equal(func4TestNum1, 2);
    assert_equal(func4TestNum2, undefined);
    assert_equal(func4TestNum3, undefined);
    assert_equal(func4TestNum4, undefined);
    funcv(value, value1, value2, value);
    assert_equal(funcvTestNum1, 1);
    assert_equal(funcvTestNum2, 2);
    assert_equal(funcvTestNum3, undefined);
    assert_equal(funcvTestNum4, 1);
    assert_equal(funcvTestNum5, undefined);
    assert_equal(funcvTestNum6, undefined);
    assert_equal(funcvTestNum7, undefined);
    assert_equal(funcvTestNum8, undefined);
    var s: A = new A(1, 4);
    assert_equal(classTestNumA1, 1);
    assert_equal(classTestNameA, "A");
    var s1: B = new B(2, 3);
    assert_equal(classTestNumB1, 2);
    assert_equal(classTestNumB2, 3);
    assert_equal(classTestNumB3, undefined);
    assert_equal(classTestNameB, "B");
    var s2: C = new C(3, 4, 5, 6, 7, 8);
    assert_equal(classTestNameC, "C");
    assert_equal(classTestNumC1, 3);
    assert_equal(classTestNumC2, 4);
    assert_equal(classTestNumC5, 7);
    assert_equal(classTestNumC7, undefined);
    assert_equal(classTestNumC8, undefined);
    var s3: C = new C(3, 4, 5, 6, 7, 8, 9, 10);
    assert_equal(classTestNameC, "C");
    assert_equal(classTestNumC1, 3);
    assert_equal(classTestNumC2, 4);
    assert_equal(classTestNumC5, 7);
    assert_equal(classTestNumC7, 9);
    assert_equal(classTestNumC8, 10);
    return value;
}
ArkTools.removeAOTFlag(funcAsm);
funcAsm(1, 2);
assert_equal(funcAsmTestNum1, 1);
assert_equal(funcAsmTestNum2, 2);
assert_equal(funcAsmTestNum3, undefined);

var TestCallArg = "unassigned";
class TestCallThis0 {
    foo(arg?: any) {
        TestCallArg = arg;
    }
}

function testCallThis0() {
    let t = new TestCallThis0();
    t.foo();
}

testCallThis0();
assert_equal(TestCallArg, undefined);
