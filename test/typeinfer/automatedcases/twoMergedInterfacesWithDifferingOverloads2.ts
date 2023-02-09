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

// === tests/cases/conformance/interfaces/declarationMerging/twoMergedInterfacesWithDifferingOverloads2.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    (): string;
    (x: number): number;
}

interface A {
    (x: number, y: number): boolean;
}

let a: A;
AssertType(a, "A");

let r = a();
AssertType(r, "string");
AssertType(a(), "string");
AssertType(a, "A");

let r2 = a(1);
AssertType(r2, "number");
AssertType(a(1), "number");
AssertType(a, "A");
AssertType(1, "int");

let r3 = a(1, 2);
AssertType(r3, "boolean");
AssertType(a(1, 2), "boolean");
AssertType(a, "A");
AssertType(1, "int");
AssertType(2, "int");

module G {
    interface A<T> {
        (): string;
        (x: T): T;
    }

    interface A<T> {
        (x: T, y: number): T;
        <U>(x: U, y: T): U;
    }

    let a: A<boolean>;
    let r = a();
    let r2 = a(true);
    let r3 = a(true, 2);
    let r4 = a(1, true);
}

