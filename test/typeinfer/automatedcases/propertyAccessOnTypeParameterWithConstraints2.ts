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

// === tests/cases/conformance/types/typeParameters/typeParameterLists/propertyAccessOnTypeParameterWithConstraints2.ts ===
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

class C<U extends A, T extends A> {
    f() {
        let x: U;
AssertType(x, "U");

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

    g(x: U) {
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

let r1 = (new C<B, A>()).f();
AssertType(r1, "string");
AssertType((new C<B, A>()).f(), "string");
AssertType((new C<B, A>()).f, "() => string");

let r1b = (new C<B, A>()).g(new B());
AssertType(r1b, "string");
AssertType((new C<B, A>()).g(new B()), "string");
AssertType((new C<B, A>()).g, "(B) => string");
AssertType(new B(), "B");
AssertType(B, "typeof B");

interface I<U extends A, T extends A> {
    foo: U;
}
//interface I<U extends T, T extends A> {
//    foo: U;
//}
let i: I<B, A>;
AssertType(i, "I<B, A>");

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
AssertType(a, "{ <U extends A, T extends A>(): U; <U extends A, T extends A>(U): U; <U extends A, T extends A>(U, T): U; }");

    <U extends A, T extends A>(): U;
    <U extends A, T extends A>(x: U): U;
AssertType(x, "U");

    <U extends A, T extends A>(x: U, y: T): U;
AssertType(x, "U");
AssertType(y, "T");
}
//let a: {
//    <U extends T, T extends A>(): U;
//    <U extends T, T extends A>(x: U): U;
//    <U extends T, T extends A>(x: U, y: T): U;
//}
let r3 = a<A, A>().foo(); 
AssertType(r3, "string");
AssertType(a<A, A>().foo(), "string");
AssertType(a<A, A>().foo, "() => string");

let r3b = a()['foo']();
AssertType(r3b, "string");
AssertType(a()['foo'](), "string");
AssertType(a()['foo'], "() => string");
AssertType(a(), "A");
AssertType(a, "{ <U extends A, T extends A>(): U; <U extends A, T extends A>(U): U; <U extends A, T extends A>(U, T): U; }");
AssertType('foo', "string");

// parameter supplied for type argument inference to succeed
let aB = new B();
AssertType(aB, "B");
AssertType(new B(), "B");
AssertType(B, "typeof B");

let r3c = a(aB, aB).foo(); 
AssertType(r3c, "string");
AssertType(a(aB, aB).foo(), "string");
AssertType(a(aB, aB).foo, "() => string");

let r3d = a(aB, aB)['foo']();
AssertType(r3d, "string");
AssertType(a(aB, aB)['foo'](), "string");
AssertType(a(aB, aB)['foo'], "() => string");
AssertType(a(aB, aB), "B");
AssertType(a, "{ <U extends A, T extends A>(): U; <U extends A, T extends A>(U): U; <U extends A, T extends A>(U, T): U; }");
AssertType(aB, "B");
AssertType(aB, "B");
AssertType('foo', "string");

let b = {
AssertType(b, "{ foo: <U extends A, T extends A>(U, T) => string; }");
AssertType({    foo: <U extends A, T extends A>(x: U, y: T) => {        let a = x['foo'](); // should be string        return a + x.foo();    }}, "{ foo: <U extends A, T extends A>(U, T) => string; }");

    foo: <U extends A, T extends A>(x: U, y: T) => {
AssertType(foo, "<U extends A, T extends A>(U, T) => string");
AssertType(x, "U");
AssertType(y, "T");
AssertType(<U extends A, T extends A>(x: U, y: T) => {        let a = x['foo'](); // should be string        return a + x.foo();    }, "<U extends A, T extends A>(U, T) => string");

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

let r4 = b.foo(aB, aB); // no inferences for T so constraint isn't satisfied, error
AssertType(r4, "string");
AssertType(b.foo(aB, aB), "string");
AssertType(b.foo, "<U extends A, T extends A>(U, T) => string");
AssertType(aB, "B");
AssertType(aB, "B");


