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

// === tests/cases/conformance/types/localTypes/localTypes3.ts ===
declare function AssertType(value:any, type:string):void;
function f1() {
    function f() {
AssertType(f, "() => typeof C");

        class C<X, Y> {
            constructor(public x: X, public y: Y) { }
        }
AssertType(C, "typeof C");
        return C;
    }
    let C = f();
AssertType(C, "typeof C");
AssertType(f(), "typeof C");
AssertType(f, "() => typeof C");

    let v = new C(10, "hello");
AssertType(v, "C<number, string>");
AssertType(new C(10, "hello"), "C<number, string>");
AssertType(C, "typeof C");
AssertType(10, "int");
AssertType("hello", "string");

    let x = v.x;
AssertType(x, "number");
AssertType(v.x, "number");

    let y = v.y;
AssertType(y, "string");
AssertType(v.y, "string");
}

function f2() {
    function f<X>(x: X) {
AssertType(f, "<X>(X) => typeof C");
AssertType(x, "X");

        class C<Y> {
            public x = x;
            constructor(public y: Y) { }
        }
AssertType(C, "typeof C");
        return C;
    }
    let C = f(10);
AssertType(C, "typeof C");
AssertType(f(10), "typeof C");
AssertType(f, "<X>(X) => typeof C");
AssertType(10, "int");

    let v = new C("hello");
AssertType(v, "f<number>.C<string>");
AssertType(new C("hello"), "f<number>.C<string>");
AssertType(C, "typeof C");
AssertType("hello", "string");

    let x = v.x;
AssertType(x, "number");
AssertType(v.x, "number");

    let y = v.y;
AssertType(y, "string");
AssertType(v.y, "string");
}

function f3() {
    function f<X, Y>(x: X, y: Y) {
AssertType(f, "<X, Y>(X, Y) => typeof C");
AssertType(x, "X");
AssertType(y, "Y");

        class C {
            public x = x;
            public y = y;
        }
AssertType(C, "typeof C");
        return C;
    }
    let C = f(10, "hello");
AssertType(C, "typeof C");
AssertType(f(10, "hello"), "typeof C");
AssertType(f, "<X, Y>(X, Y) => typeof C");
AssertType(10, "int");
AssertType("hello", "string");

    let v = new C();
AssertType(v, "f<number, string>.C");
AssertType(new C(), "f<number, string>.C");
AssertType(C, "typeof C");

    let x = v.x;
AssertType(x, "number");
AssertType(v.x, "number");

    let y = v.y;
AssertType(y, "string");
AssertType(v.y, "string");
}


