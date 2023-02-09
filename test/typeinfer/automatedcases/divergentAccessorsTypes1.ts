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

// === tests/cases/compiler/divergentAccessorsTypes1.ts ===
declare function AssertType(value:any, type:string):void;
class Test1 {
    get foo(): string { 
AssertType("", "string");
return "" 
}

    set foo(s: string | number) {
        let a = s as string;
AssertType(a, "string");
AssertType(s as string, "string");
AssertType(s, "union");

        let b = s as number;
AssertType(b, "number");
AssertType(s as number, "number");
AssertType(s, "union");
    }

    get bar(): string | number { 
AssertType("", "string");
return "" 
}

    set bar(s: string | number | boolean) {
    }
}

interface Test2 {
    get foo(): string;
    set foo(s: string | number);

    get bar(): string | number;
    set bar(s: string | number | boolean);
}

type Test3 = {
    get foo(): string;
    set foo(s: string | number);

    get bar(): string | number;
    set bar(s: string | number | boolean);
};

{
    const t = new Test1();
AssertType(t, "Test1");
AssertType(new Test1(), "Test1");
AssertType(Test1, "typeof Test1");

    t.foo = 32;
AssertType(t.foo = 32, "int");
AssertType(t.foo, "union");
AssertType(32, "int");

    let m: string = t.foo;
AssertType(m, "string");
AssertType(t.foo, "string");

    // See how CFA interacts with out-of-type writes
    t.bar = 42;
AssertType(t.bar = 42, "int");
AssertType(t.bar, "union");
AssertType(42, "int");

    let n: number = t.bar;
AssertType(n, "number");
AssertType(t.bar, "number");

    t.bar = false;
AssertType(t.bar = false, "boolean");
AssertType(t.bar, "union");
AssertType(false, "boolean");

    let o = t.bar;
AssertType(o, "union");
AssertType(t.bar, "union");
}

{
    const t = {} as Test2;
AssertType(t, "Test2");
AssertType({} as Test2, "Test2");
AssertType({}, "{}");

    t.foo = 32;
AssertType(t.foo = 32, "int");
AssertType(t.foo, "union");
AssertType(32, "int");

    let m: string = t.foo;
AssertType(m, "string");
AssertType(t.foo, "string");

    // See how CFA interacts with out-of-type writes
    t.bar = 42;
AssertType(t.bar = 42, "int");
AssertType(t.bar, "union");
AssertType(42, "int");

    let n: number = t.bar;
AssertType(n, "number");
AssertType(t.bar, "number");

    t.bar = false;
AssertType(t.bar = false, "boolean");
AssertType(t.bar, "union");
AssertType(false, "boolean");

    let o = t.bar;
AssertType(o, "union");
AssertType(t.bar, "union");
}

{
    const t = {} as Test3;
AssertType(t, "Test3");
AssertType({} as Test3, "Test3");
AssertType({}, "{}");

    t.foo = 32;
AssertType(t.foo = 32, "int");
AssertType(t.foo, "union");
AssertType(32, "int");

    let m: string = t.foo;
AssertType(m, "string");
AssertType(t.foo, "string");

    // See how CFA interacts with out-of-type writes
    t.bar = 42;
AssertType(t.bar = 42, "int");
AssertType(t.bar, "union");
AssertType(42, "int");

    let n: number = t.bar;
AssertType(n, "number");
AssertType(t.bar, "number");

    t.bar = false;
AssertType(t.bar = false, "boolean");
AssertType(t.bar, "union");
AssertType(false, "boolean");

    let o = t.bar;
AssertType(o, "union");
AssertType(t.bar, "union");
}

