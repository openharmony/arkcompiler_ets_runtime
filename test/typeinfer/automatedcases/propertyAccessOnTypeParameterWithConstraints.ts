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

// === tests/cases/conformance/types/typeParameters/typeParameterLists/propertyAccessOnTypeParameterWithConstraints.ts ===
declare function AssertType(value:any, type:string):void;
// generic types should behave as if they have properties of their constraint type
// no errors expected 

class C<T extends Date> {
    f() {
        let x: T;
AssertType(x, "T");

        let a = x['getDate'](); // number
AssertType(a, "number");
AssertType(x['getDate'](), "number");
AssertType(x['getDate'], "() => number");
AssertType(x, "T");
AssertType('getDate', "string");

AssertType(a + x.getDate(), "number");
AssertType(a, "number");
AssertType(x.getDate(), "number");
AssertType(x.getDate, "() => number");
        return a + x.getDate();
    }
}

let r = (new C<Date>()).f();
AssertType(r, "number");
AssertType((new C<Date>()).f(), "number");
AssertType((new C<Date>()).f, "() => number");

interface I<T extends Date> {
    foo: T;
}
let i: I<Date>;
AssertType(i, "I<Date>");

let r2 = i.foo.getDate();
AssertType(r2, "number");
AssertType(i.foo.getDate(), "number");
AssertType(i.foo.getDate, "() => number");

let r2b = i.foo['getDate']();
AssertType(r2b, "number");
AssertType(i.foo['getDate'](), "number");
AssertType(i.foo['getDate'], "() => number");
AssertType(i.foo, "Date");
AssertType('getDate', "string");

let a: {
AssertType(a, "<T extends Date>() => T");

    <T extends Date>(): T;
}
let r3 = a<Date>().getDate();
AssertType(r3, "number");
AssertType(a<Date>().getDate(), "number");
AssertType(a<Date>().getDate, "() => number");

let r3b = a()['getDate']();
AssertType(r3b, "number");
AssertType(a()['getDate'](), "number");
AssertType(a()['getDate'], "() => number");
AssertType(a(), "Date");
AssertType(a, "<T extends Date>() => T");
AssertType('getDate', "string");

let b = {
AssertType(b, "{ foo: <T extends Date>(T) => number; }");
AssertType({    foo: <T extends Date>(x: T) => {        let a = x['getDate'](); // number        return a + x.getDate();    }}, "{ foo: <T extends Date>(T) => number; }");

    foo: <T extends Date>(x: T) => {
AssertType(foo, "<T extends Date>(T) => number");
AssertType(x, "T");
AssertType(<T extends Date>(x: T) => {        let a = x['getDate'](); // number        return a + x.getDate();    }, "<T extends Date>(T) => number");

        let a = x['getDate'](); // number
AssertType(a, "number");
AssertType(x['getDate'](), "number");
AssertType(x['getDate'], "() => number");
AssertType(x, "T");
AssertType('getDate', "string");

AssertType(a + x.getDate(), "number");
AssertType(a, "number");
AssertType(x.getDate(), "number");
AssertType(x.getDate, "() => number");
        return a + x.getDate();
    }
}

let r4 = b.foo(new Date());
AssertType(r4, "number");
AssertType(b.foo(new Date()), "number");
AssertType(b.foo, "<T extends Date>(T) => number");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");


