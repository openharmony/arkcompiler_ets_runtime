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

// === tests/cases/conformance/types/typeParameters/typeParameterLists/propertyAccessOnTypeParameterWithConstraints3.ts ===
declare function AssertType(value:any, type:string):void;
// generic types should behave as if they have properties of their constraint type

class A {
    foo(): string { 
AssertType('', "string");
return ''; 
}
}

class B extends A {
    bar(): string {
AssertType('', "string");
        return '';
    }
}

class C<U extends A, T extends U> {
    f() {
        let x: T;
AssertType(x, "T");

        // BUG 823818
        let a = x['foo'](); // should be string
AssertType(a, "string");
AssertType(x['foo'](), "string");
AssertType(x['foo'], "() => string");
AssertType(x, "T");
AssertType('foo', "string");

AssertType(a + x.foo(), "string");
AssertType(a, "string");
AssertType(x.foo(), "string");
AssertType(x.foo, "() => string");
        return a + x.foo();
    }

    g(x: U) {
        // BUG 823818
        let a = x['foo'](); // should be string
AssertType(a, "string");
AssertType(x['foo'](), "string");
AssertType(x['foo'], "() => string");
AssertType(x, "U");
AssertType('foo', "string");

AssertType(a + x.foo(), "string");
AssertType(a, "string");
AssertType(x.foo(), "string");
AssertType(x.foo, "() => string");
        return a + x.foo();
    }
}

let r1a = (new C<A, B>()).f();
AssertType(r1a, "string");
AssertType((new C<A, B>()).f(), "string");
AssertType((new C<A, B>()).f, "() => string");

let r1b = (new C<A, B>()).g(new B());
AssertType(r1b, "string");
AssertType((new C<A, B>()).g(new B()), "string");
AssertType((new C<A, B>()).g, "(A) => string");
AssertType(new B(), "B");
AssertType(B, "typeof B");

interface I<U extends A, T extends U> {
    foo: T;
}
let i: I<A, B>;
AssertType(i, "I<A, B>");

let r2 = i.foo.foo();
AssertType(r2, "string");
AssertType(i.foo.foo(), "string");
AssertType(i.foo.foo, "() => string");

let r2b = i.foo['foo']();
AssertType(r2b, "string");
AssertType(i.foo['foo'](), "string");
AssertType(i.foo['foo'], "() => string");
AssertType(i.foo, "B");
AssertType('foo', "string");

let a: {
AssertType(a, "{ <U extends A, T extends U>(): T; <U extends T, T extends A>(U): U; }");

    <U extends A, T extends U>(): T;
    <U extends T, T extends A>(x: U): U;
AssertType(x, "U");
}
let r3 = a().foo(); // error, no inferences for U so it doesn't satisfy constraint
AssertType(r3, "string");
AssertType(a().foo(), "string");
AssertType(a().foo, "() => string");

let r3b = a()['foo']();
AssertType(r3b, "string");
AssertType(a()['foo'](), "string");
AssertType(a()['foo'], "() => string");
AssertType(a(), "A");
AssertType(a, "{ <U extends A, T extends U>(): T; <U extends T, T extends A>(U): U; }");
AssertType('foo', "string");

// parameter supplied for type argument inference for U
let r3c = a(new B()).foo(); // valid call to an invalid function, U is inferred as B, which has a foo
AssertType(r3c, "string");
AssertType(a(new B()).foo(), "string");
AssertType(a(new B()).foo, "() => string");

let r3d = a(new B())['foo'](); // valid call to an invalid function, U is inferred as B, which has a foo
AssertType(r3d, "string");
AssertType(a(new B())['foo'](), "string");
AssertType(a(new B())['foo'], "() => string");
AssertType(a(new B()), "B");
AssertType(a, "{ <U extends A, T extends U>(): T; <U extends T, T extends A>(U): U; }");
AssertType(new B(), "B");
AssertType(B, "typeof B");
AssertType('foo', "string");

let b = {
AssertType(b, "{ foo: <U extends A, T extends U>(T) => string; }");
AssertType({    foo: <U extends A, T extends U>(x: T) => {        // BUG 823818        let a = x['foo'](); // should be string        return a + x.foo();    }}, "{ foo: <U extends A, T extends U>(T) => string; }");

    foo: <U extends A, T extends U>(x: T) => {
AssertType(foo, "<U extends A, T extends U>(T) => string");
AssertType(x, "T");
AssertType(<U extends A, T extends U>(x: T) => {        // BUG 823818        let a = x['foo'](); // should be string        return a + x.foo();    }, "<U extends A, T extends U>(T) => string");

        // BUG 823818
        let a = x['foo'](); // should be string
AssertType(a, "string");
AssertType(x['foo'](), "string");
AssertType(x['foo'], "() => string");
AssertType(x, "T");
AssertType('foo', "string");

AssertType(a + x.foo(), "string");
AssertType(a, "string");
AssertType(x.foo(), "string");
AssertType(x.foo, "() => string");
        return a + x.foo();
    }
}

let r4 = b.foo(new B()); // valid call to an invalid function
AssertType(r4, "string");
AssertType(b.foo(new B()), "string");
AssertType(b.foo, "<U extends A, T extends U>(T) => string");
AssertType(new B(), "B");
AssertType(B, "typeof B");


