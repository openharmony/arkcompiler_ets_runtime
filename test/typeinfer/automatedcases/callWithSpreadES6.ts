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

// === tests/cases/conformance/expressions/functionCalls/callWithSpreadES6.ts ===
declare function AssertType(value:any, type:string):void;
interface X {
    foo(x: number, y: number, ...z: string[]);
}

function foo(x: number, y: number, ...z: string[]) {
}

let a: string[];
AssertType(a, "string[]");

let z: number[];
AssertType(z, "number[]");

let obj: X;
AssertType(obj, "X");

let xa: X[];
AssertType(xa, "X[]");

foo(1, 2, "abc");
AssertType(foo(1, 2, "abc"), "void");
AssertType(foo, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType("abc", "string");

foo(1, 2, ...a);
AssertType(foo(1, 2, ...a), "void");
AssertType(foo, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

foo(1, 2, ...a, "abc");
AssertType(foo(1, 2, ...a, "abc"), "void");
AssertType(foo, "(number, number, ...string[]) => void");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("abc", "string");

obj.foo(1, 2, "abc");
AssertType(obj.foo(1, 2, "abc"), "any");
AssertType(obj.foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType("abc", "string");

obj.foo(1, 2, ...a);
AssertType(obj.foo(1, 2, ...a), "any");
AssertType(obj.foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

obj.foo(1, 2, ...a, "abc");
AssertType(obj.foo(1, 2, ...a, "abc"), "any");
AssertType(obj.foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("abc", "string");

(obj.foo)(1, 2, "abc");
AssertType((obj.foo)(1, 2, "abc"), "any");
AssertType((obj.foo), "(number, number, ...string[]) => any");
AssertType(obj.foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType("abc", "string");

(obj.foo)(1, 2, ...a);
AssertType((obj.foo)(1, 2, ...a), "any");
AssertType((obj.foo), "(number, number, ...string[]) => any");
AssertType(obj.foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

(obj.foo)(1, 2, ...a, "abc");
AssertType((obj.foo)(1, 2, ...a, "abc"), "any");
AssertType((obj.foo), "(number, number, ...string[]) => any");
AssertType(obj.foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("abc", "string");

xa[1].foo(1, 2, "abc");
AssertType(xa[1].foo(1, 2, "abc"), "any");
AssertType(xa[1].foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType("abc", "string");

xa[1].foo(1, 2, ...a);
AssertType(xa[1].foo(1, 2, ...a), "any");
AssertType(xa[1].foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");

xa[1].foo(1, 2, ...a, "abc");
AssertType(xa[1].foo(1, 2, ...a, "abc"), "any");
AssertType(xa[1].foo, "(number, number, ...string[]) => any");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
AssertType("abc", "string");

(<Function>xa[1].foo)(...[1, 2, "abc"]);
AssertType((<Function>xa[1].foo)(...[1, 2, "abc"]), "any");
AssertType((<Function>xa[1].foo), "Function");
AssertType(<Function>xa[1].foo, "Function");
AssertType(xa[1].foo, "(number, number, ...string[]) => any");
AssertType(...[1, 2, "abc"], "union");
AssertType([1, 2, "abc"], "(union)[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType("abc", "string");

class C {
    constructor(x: number, y: number, ...z: string[]) {
        this.foo(x, y);
AssertType(this.foo(x, y), "void");
AssertType(this.foo, "(number, number, ...string[]) => void");
AssertType(this, "this");
AssertType(x, "number");
AssertType(y, "number");

        this.foo(x, y, ...z);
AssertType(this.foo(x, y, ...z), "void");
AssertType(this.foo, "(number, number, ...string[]) => void");
AssertType(this, "this");
AssertType(x, "number");
AssertType(y, "number");
AssertType(...z, "string");
AssertType(z, "string[]");
    }
    foo(x: number, y: number, ...z: string[]) {
    }
}

class D extends C {
    constructor() {
        super(1, 2);
AssertType(super(1, 2), "void");
AssertType(super, "typeof C");
AssertType(1, "int");
AssertType(2, "int");

        super(1, 2, ...a);
AssertType(super(1, 2, ...a), "void");
AssertType(super, "typeof C");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
    }
    foo() {
        super.foo(1, 2);
AssertType(super.foo(1, 2), "void");
AssertType(super.foo, "(number, number, ...string[]) => void");
AssertType(super, "C");
AssertType(1, "int");
AssertType(2, "int");

        super.foo(1, 2, ...a);
AssertType(super.foo(1, 2, ...a), "void");
AssertType(super.foo, "(number, number, ...string[]) => void");
AssertType(super, "C");
AssertType(1, "int");
AssertType(2, "int");
AssertType(...a, "string");
AssertType(a, "string[]");
    }
}


