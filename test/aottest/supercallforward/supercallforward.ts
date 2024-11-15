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

declare function print(str:any):string;

function assert_true(condition: boolean): void
{
    if (!condition) {
        print("assert fail");
    }
}

function run(cb: () => void) {
    let cnt: number = 20;
    for (let i = 0; i < cnt; ++i) {
        cb();
    }
}

class Base {
    name: string;
    value: number;
    constructor(name: string, value: number) {
        this.name = name;
        this.value = value;
    }
}

class Child extends Base {}

function testChild0Arg() {
    let c = new Child();
    assert_true(c.name === undefined);
    assert_true(c.value === undefined);
}

function testChild1Arg() {
    let c = new Child("one");
    assert_true(c.name === "one");
    assert_true(c.value === undefined);
}

function testChild2Arg() {
    let c = new Child("two", 2);
    assert_true(c.name === "two");
    assert_true(c.value === 2);
}

function testChild3Arg() {
    let c = new Child("three", 3, "extra");
    assert_true(c.name === "three");
    assert_true(c.value === 3);
}

class GrandChild extends Child {}

function testGrandChild0Arg() {
    let c = new GrandChild();
    assert_true(c.name === undefined);
    assert_true(c.value === undefined);
}

function testGrandChild1Arg() {
    let c = new GrandChild("one");
    assert_true(c.name === "one");
    assert_true(c.value === undefined);
}

function testGrandChild2Arg() {
    let c = new GrandChild("two", 2);
    assert_true(c.name === "two");
    assert_true(c.value === 2);
}

function testGrandChild3Arg() {
    let c = new GrandChild("three", 3, "extra");
    assert_true(c.name === "three");
    assert_true(c.value === 3);
}

class BaseRestArgs {
    args: Object[] = [];
    constructor(...args) {
        for (const arg of args) {
            this.args.push(arg);
        }
    }
}

class ChildRestArgs extends BaseRestArgs {}

function testChildRestArgs0Arg() {
    let c = new ChildRestArgs();
    assert_true(c.args.length === 0);
}

function testChildRestArgs1Arg() {
    let c = new ChildRestArgs("one");
    assert_true(c.args.length === 1);
    assert_true(c.args[0] === "one");
}

function testChildRestArgs2Arg() {
    let c = new ChildRestArgs("two", 2);
    assert_true(c.args.length === 2);
    assert_true(c.args[0] === "two");
    assert_true(c.args[1] === 2);
}

function testChildRestArgs3Arg() {
    let c = new ChildRestArgs("three", 3, "extra");
    assert_true(c.args.length === 3);
    assert_true(c.args[0] === "three");
    assert_true(c.args[1] === 3);
    assert_true(c.args[2] === "extra");
}

class GrandChildRestArgs extends ChildRestArgs {}

function testGrandChildRestArgs0Arg() {
    let c = new GrandChildRestArgs();
    assert_true(c.args.length === 0);
}

function testGrandChildRestArgs1Arg() {
    let c = new GrandChildRestArgs("one");
    assert_true(c.args.length === 1);
    assert_true(c.args[0] === "one");
}

function testGrandChildRestArgs2Arg() {
    let c = new GrandChildRestArgs("two", 2);
    assert_true(c.args.length === 2);
    assert_true(c.args[0] === "two");
    assert_true(c.args[1] === 2);
}

function testGrandChildRestArgs3Arg() {
    let c = new GrandChildRestArgs("three", 3, "extra");
    assert_true(c.args.length === 3);
    assert_true(c.args[0] === "three");
    assert_true(c.args[1] === 3);
    assert_true(c.args[2] === "extra");
}

class BaseArguments {
    args: Object[] = [];
    constructor() {
        for (const arg of arguments) {
            this.args.push(arg);
        }
    }
}

class ChildArguments extends BaseArguments {}

function testChildArguments0Arg() {
    let c = new ChildArguments();
    assert_true(c.args.length === 0);
}

function testChildArguments1Arg() {
    let c = new ChildArguments("one");
    assert_true(c.args.length === 1);
    assert_true(c.args[0] === "one");
}

function testChildArguments2Arg() {
    let c = new ChildArguments("two", 2);
    assert_true(c.args.length === 2);
    assert_true(c.args[0] === "two");
    assert_true(c.args[1] === 2);
}

function testChildArguments3Arg() {
    let c = new ChildArguments("three", 3, "extra");
    assert_true(c.args.length === 3);
    assert_true(c.args[0] === "three");
    assert_true(c.args[1] === 3);
    assert_true(c.args[2] === "extra");
}

class GrandChildArguments extends ChildArguments {
}

function testGrandChildArguments0Arg() {
    let c = new GrandChildArguments();
    assert_true(c.args.length === 0);
}

function testGrandChildArguments1Arg() {
    let c = new GrandChildArguments("one");
    assert_true(c.args.length === 1);
    assert_true(c.args[0] === "one");
}

function testGrandChildArguments2Arg() {
    let c = new GrandChildArguments("two", 2);
    assert_true(c.args.length === 2);
    assert_true(c.args[0] === "two");
    assert_true(c.args[1] === 2);
}

function testGrandChildArguments3Arg() {
    let c = new GrandChildArguments("three", 3, "extra");
    assert_true(c.args.length === 3);
    assert_true(c.args[0] === "three");
    assert_true(c.args[1] === 3);
    assert_true(c.args[2] === "extra");
}

class BaseCornerCase {
    name: string = ""
    value: number = 0;
}

class ChildUndefined extends BaseCornerCase {}
class ChildNull extends BaseCornerCase {}
class ChildObj extends BaseCornerCase {}
class ChildNumber extends BaseCornerCase {}
class ChildString extends BaseCornerCase {}
class ChildArrow extends BaseCornerCase {}
class ChildFunction extends BaseCornerCase {}

function runChildCornerCase(ctor: Function, replacement: any) {
    let cnt = 20;
    for (let i = 0; i < cnt; ++i) {
        if (i == cnt - 1) {
            ctor.__proto__ = replacement;  // at the last iteration, change super ctor
        }
        try {
            new ctor();
        } catch (e) {
            print(ctor.name);
            print(e.name);
        }
    }
    assert_true(ArkTools.isAOTCompiled(ctor));
}

function testChildCornerCases() {
    runChildCornerCase(ChildUndefined, undefined);      // normal
    runChildCornerCase(ChildNull, null);                // error: super is not ctor
    runChildCornerCase(ChildObj, {xxx: "yyy"});         // error: super is not ctor
    runChildCornerCase(ChildNumber, 111);               // normal
    runChildCornerCase(ChildString, "hello");           // normal
    runChildCornerCase(ChildArrow, () => {});           // error: super is not ctor
    runChildCornerCase(ChildFunction, function () {});  // normal
    assert_true(ArkTools.isAOTCompiled(BaseCornerCase));
}


run(testChild0Arg);
run(testChild1Arg);
run(testChild2Arg);
run(testChild3Arg);
run(testGrandChild0Arg);
run(testGrandChild1Arg);
run(testGrandChild2Arg);
run(testGrandChild3Arg);
assert_true(ArkTools.isAOTCompiled(Base));
assert_true(ArkTools.isAOTCompiled(Child));
assert_true(ArkTools.isAOTCompiled(GrandChild));

run(testChildRestArgs0Arg);
run(testChildRestArgs1Arg);
run(testChildRestArgs2Arg);
run(testChildRestArgs3Arg);
run(testGrandChildRestArgs0Arg);
run(testGrandChildRestArgs1Arg);
run(testGrandChildRestArgs2Arg);
run(testGrandChildRestArgs3Arg);
assert_true(ArkTools.isAOTCompiled(BaseRestArgs));
assert_true(ArkTools.isAOTCompiled(ChildRestArgs));
assert_true(ArkTools.isAOTCompiled(GrandChildRestArgs));

run(testChildArguments0Arg);
run(testChildArguments1Arg);
run(testChildArguments2Arg);
run(testChildArguments3Arg);
run(testGrandChildArguments0Arg);
run(testGrandChildArguments1Arg);
run(testGrandChildArguments2Arg);
run(testGrandChildArguments3Arg);
assert_true(ArkTools.isAOTCompiled(BaseArguments));
assert_true(ArkTools.isAOTCompiled(ChildArguments));
assert_true(ArkTools.isAOTCompiled(GrandChildArguments));

testChildCornerCases();

print("success");
