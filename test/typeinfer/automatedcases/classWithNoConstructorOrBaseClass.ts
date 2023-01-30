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

// === tests/cases/conformance/classes/members/constructorFunctionTypes/classWithNoConstructorOrBaseClass.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    x: string;
}

let c = new C();
AssertType(c, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

let r = C;
AssertType(r, "typeof C");
AssertType(C, "typeof C");

class D<T,U> {
    x: T;
    y: U;
}

let d = new D();
AssertType(d, "D<unknown, unknown>");
AssertType(new D(), "D<unknown, unknown>");
AssertType(D, "typeof D");

let d2 = new D<string, number>();
AssertType(d2, "D<string, number>");
AssertType(new D<string, number>(), "D<string, number>");
AssertType(D, "typeof D");

let r2 = D;
AssertType(r2, "typeof D");
AssertType(D, "typeof D");


