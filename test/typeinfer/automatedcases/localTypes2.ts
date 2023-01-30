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

// === tests/cases/conformance/types/localTypes/localTypes2.ts ===
declare function AssertType(value:any, type:string):void;
function f1() {
    function f() {
AssertType(f, "() => typeof C");

        class C {
            constructor(public x: number, public y: number) { }
        }
AssertType(C, "typeof C");
        return C;
    }
    let C = f();
AssertType(C, "typeof C");
AssertType(f(), "typeof C");
AssertType(f, "() => typeof C");

    let v = new C(10, 20);
AssertType(v, "C");
AssertType(new C(10, 20), "C");
AssertType(C, "typeof C");
AssertType(10, "int");
AssertType(20, "int");

    let x = v.x;
AssertType(x, "number");
AssertType(v.x, "number");

    let y = v.y;
AssertType(y, "number");
AssertType(v.y, "number");
}

function f2() {
    function f(x: number) {
AssertType(f, "(number) => typeof C");
AssertType(x, "number");

        class C {
            public x = x;
            constructor(public y: number) { }
        }
AssertType(C, "typeof C");
        return C;
    }
    let C = f(10);
AssertType(C, "typeof C");
AssertType(f(10), "typeof C");
AssertType(f, "(number) => typeof C");
AssertType(10, "int");

    let v = new C(20);
AssertType(v, "C");
AssertType(new C(20), "C");
AssertType(C, "typeof C");
AssertType(20, "int");

    let x = v.x;
AssertType(x, "number");
AssertType(v.x, "number");

    let y = v.y;
AssertType(y, "number");
AssertType(v.y, "number");
}

function f3() {
    function f(x: number, y: number) {
AssertType(f, "(number, number) => typeof C");
AssertType(x, "number");
AssertType(y, "number");

        class C {
            public x = x;
            public y = y;
        }
AssertType(C, "typeof C");
        return C;
    }
    let C = f(10, 20);
AssertType(C, "typeof C");
AssertType(f(10, 20), "typeof C");
AssertType(f, "(number, number) => typeof C");
AssertType(10, "int");
AssertType(20, "int");

    let v = new C();
AssertType(v, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

    let x = v.x;
AssertType(x, "number");
AssertType(v.x, "number");

    let y = v.y;
AssertType(y, "number");
AssertType(v.y, "number");
}


