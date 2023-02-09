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

// === tests/cases/conformance/types/typeParameters/typeParameterLists/propertyAccessOnTypeParameterWithoutConstraints.ts ===
declare function AssertType(value:any, type:string):void;
class C<T> {
    f() {
        let x: T;
AssertType(x, "T");

        let a = x['toString'](); // should be string
AssertType(a, "string");
AssertType(x['toString'](), "string");
AssertType(x['toString'], "() => string");
AssertType(x, "T");
AssertType('toString', "string");

AssertType(a + x.toString(), "string");
AssertType(a, "string");
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");
        return a + x.toString();
    }
}

let r = (new C<number>()).f();
AssertType(r, "string");
AssertType((new C<number>()).f(), "string");
AssertType((new C<number>()).f, "() => string");

interface I<T> {
    foo: T;
}
let i: I<number>;
AssertType(i, "I<number>");

let r2 = i.foo.toString();
AssertType(r2, "string");
AssertType(i.foo.toString(), "string");
AssertType(i.foo.toString, "(?number) => string");

let r2b = i.foo['toString']();
AssertType(r2b, "string");
AssertType(i.foo['toString'](), "string");
AssertType(i.foo['toString'], "(?number) => string");
AssertType(i.foo, "number");
AssertType('toString', "string");

let a: {
AssertType(a, "<T>() => T");

    <T>(): T;
}
let r3: string = a().toString();
AssertType(r3, "string");
AssertType(a().toString(), "string");
AssertType(a().toString, "() => string");

let r3b: string = a()['toString']();
AssertType(r3b, "string");
AssertType(a()['toString'](), "string");
AssertType(a()['toString'], "() => string");
AssertType(a(), "unknown");
AssertType(a, "<T>() => T");
AssertType('toString', "string");

let b = {
AssertType(b, "{ foo: <T>(T) => string; }");
AssertType({    foo: <T>(x: T) => {        let a = x['toString'](); // should be string        return a + x.toString();    }}, "{ foo: <T>(T) => string; }");

    foo: <T>(x: T) => {
AssertType(foo, "<T>(T) => string");
AssertType(x, "T");
AssertType(<T>(x: T) => {        let a = x['toString'](); // should be string        return a + x.toString();    }, "<T>(T) => string");

        let a = x['toString'](); // should be string
AssertType(a, "string");
AssertType(x['toString'](), "string");
AssertType(x['toString'], "() => string");
AssertType(x, "T");
AssertType('toString', "string");

AssertType(a + x.toString(), "string");
AssertType(a, "string");
AssertType(x.toString(), "string");
AssertType(x.toString, "() => string");
        return a + x.toString();
    }
}

let r4 = b.foo(1);
AssertType(r4, "string");
AssertType(b.foo(1), "string");
AssertType(b.foo, "<T>(T) => string");
AssertType(1, "int");


