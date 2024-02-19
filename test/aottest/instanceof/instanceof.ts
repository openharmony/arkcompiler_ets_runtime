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

declare interface ArkTools {
    isAOTCompiled(args: any): boolean;
}

declare function print(str:any):string;
class A{};
class B{};
var a = new A();
var b = new B();
print(a instanceof A);
print(b instanceof A);
print(a instanceof B);
print(b instanceof B);

class C {}

Object.defineProperty(C, Symbol.hasInstance, {
  value(instance) {
    return false;
  },
});

class D extends A {}

function test1 () {
    let a = new A();
    print(a instanceof A); // true
}
test1();

function test2 () {
    let a = new D();
    print(a instanceof D); // true
}
test2();

function test3 () {
    let a = new C();
    print(a instanceof C); // false
}
test3();

function test4() {
    let a = new Array();
    print(a instanceof Array); // true
}
test4();

function test5() {
    let a = new Array();
    print(a instanceof A); // false
}
test5();

function foo() {};
function bar() {};
const proxy = new Proxy(foo, {});

let f = new foo();

print(f instanceof foo); // true
print(f instanceof proxy); // true
print(f instanceof bar); // false

print(ArkTools.isAOTCompiled(test1));
print(ArkTools.isAOTCompiled(test2));
print(ArkTools.isAOTCompiled(test3));
print(ArkTools.isAOTCompiled(test4));
print(ArkTools.isAOTCompiled(test5));

function test6() {
    const target = new A();
    const proxy = new Proxy( target, {});
    print(proxy instanceof A);
}
test6();
print(ArkTools.isAOTCompiled(test6));
