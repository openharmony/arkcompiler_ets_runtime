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

// === tests/cases/conformance/types/typeParameters/typeArgumentLists/wrappedAndRecursiveConstraints.ts ===
declare function AssertType(value:any, type:string):void;
// no errors expected

class C<T extends Date> {
    constructor(public data: T) { }
    foo<U extends T>(x: U) {
AssertType(x, "U");
        return x;
    }
}

interface Foo extends Date {
    foo: string;
}

let y: Foo = null;
AssertType(y, "Foo");
AssertType(null, "null");

let c = new C(y);
AssertType(c, "C<Foo>");
AssertType(new C(y), "C<Foo>");
AssertType(C, "typeof C");
AssertType(y, "Foo");

let r = c.foo(y);
AssertType(r, "Foo");
AssertType(c.foo(y), "Foo");
AssertType(c.foo, "<U extends Foo>(U) => U");
AssertType(y, "Foo");


