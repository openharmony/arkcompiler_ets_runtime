/*
* Copyright (c) Microsoft Corporation. All rights reserved.
* Copyright (c) 2023 Huawei Device Co., Ltd.
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
*
* This file has been modified by Huawei to verify type inference by adding verification statements.
*/

// === tests/cases/compiler/constructorOverloads2.ts ===
declare function AssertType(value:any, type:string):void;
class FooBase {
    constructor(s: string);
    constructor(n: number);
    constructor(x: any) {
    }
    bar1() {  /*WScript.Echo("base bar1");*/ }
}

class Foo extends FooBase {
    constructor(s: string);
    constructor(n: number);
    constructor(a:any);
    constructor(x: any, y?: any) {
        super(x);
AssertType(super(x), "void");
AssertType(super, "typeof FooBase");
AssertType(x, "any");
    }
    bar1() {  /*WScript.Echo("bar1");*/ }
}

let f1 = new Foo("hey");
AssertType(f1, "Foo");
AssertType(new Foo("hey"), "Foo");
AssertType(Foo, "typeof Foo");
AssertType("hey", "string");

let f2 = new Foo(0);
AssertType(f2, "Foo");
AssertType(new Foo(0), "Foo");
AssertType(Foo, "typeof Foo");
AssertType(0, "int");

let f3 = new Foo(f1);
AssertType(f3, "Foo");
AssertType(new Foo(f1), "Foo");
AssertType(Foo, "typeof Foo");
AssertType(f1, "Foo");

let f4 = new Foo([f1,f2,f3]);
AssertType(f4, "Foo");
AssertType(new Foo([f1,f2,f3]), "Foo");
AssertType(Foo, "typeof Foo");
AssertType([f1,f2,f3], "Foo[]");
AssertType(f1, "Foo");
AssertType(f2, "Foo");
AssertType(f3, "Foo");

f1.bar1();
AssertType(f1.bar1(), "void");
AssertType(f1.bar1, "() => void");


