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

// === tests/cases/conformance/types/specifyingTypes/typeLiterals/functionLiteralForOverloads2.ts ===
declare function AssertType(value:any, type:string):void;
// basic uses of function literals with constructor overloads

class C {
    constructor(x: string);
    constructor(x: number);
    constructor(x) { }
}

class D<T> {
    constructor(x: string);
    constructor(x: number);
    constructor(x) { }
}

let f: {
AssertType(f, "{ new (string): C; new (number): C; }");

    new(x: string): C;
AssertType(x, "string");

    new(x: number): C;
AssertType(x, "number");

} = C;
AssertType(C, "typeof C");

let f2: {
AssertType(f2, "{ new <T>(string): C; new <T>(number): C; }");

    new<T>(x: string): C;
AssertType(x, "string");

    new<T>(x: number): C;
AssertType(x, "number");

} = C;
AssertType(C, "typeof C");

let f3: {
AssertType(f3, "{ new <T>(string): D<T>; new <T>(number): D<T>; }");

    new<T>(x: string): D<T>;
AssertType(x, "string");

    new<T>(x: number): D<T>;
AssertType(x, "number");

} = D;
AssertType(D, "typeof D");


