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

// === tests/cases/conformance/classes/staticIndexSignature/staticIndexSignature6.ts ===
declare function AssertType(value:any, type:string):void;
function foo () {
AssertType(class<T> {        static [s: string]: number        static [s: number]: 42        foo(v: T) { return v }    }, "typeof (Anonymous class)");
    return class<T> {

        static [s: string]: number
AssertType(s, "string");

        static [s: number]: 42
AssertType(s, "number");

        foo(v: T) { 
AssertType(foo, "(T) => T");
return v 

AssertType(v, "T");

AssertType(v, "T");
}
    }
}

const C = foo()
AssertType(C, "typeof (Anonymous class)");
AssertType(foo(), "typeof (Anonymous class)");
AssertType(foo, "() => typeof (Anonymous class)");

C.a;
AssertType(C.a, "number");

C.a = 1;
AssertType(C.a = 1, "int");
AssertType(C.a, "number");
AssertType(1, "int");

C[2];
AssertType(C[2], "int");
AssertType(C, "typeof (Anonymous class)");
AssertType(2, "int");

C[2] = 42;
AssertType(C[2] = 42, "int");
AssertType(C[2], "int");
AssertType(C, "typeof (Anonymous class)");
AssertType(2, "int");
AssertType(42, "int");

const c = new C<number>();
AssertType(c, "(Anonymous class)<number>");
AssertType(new C<number>(), "(Anonymous class)<number>");
AssertType(C, "typeof (Anonymous class)");

c.foo(1);
AssertType(c.foo(1), "number");
AssertType(c.foo, "(number) => number");
AssertType(1, "int");


