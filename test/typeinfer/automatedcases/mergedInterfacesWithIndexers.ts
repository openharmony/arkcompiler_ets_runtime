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

// === tests/cases/conformance/interfaces/declarationMerging/mergedInterfacesWithIndexers.ts ===
declare function AssertType(value:any, type:string):void;
// indexers should behave like other members when merging interface declarations

interface A {
    [x: number]: string;
}


interface A {
    [x: string]: { length: number };
}

let a: A;
AssertType(a, "A");

let r = a[1];
AssertType(r, "string");
AssertType(a[1], "string");
AssertType(a, "A");
AssertType(1, "int");

let r2 = a['1'];
AssertType(r2, "string");
AssertType(a['1'], "string");
AssertType(a, "A");
AssertType('1', "string");

let r3 = a['hi'];
AssertType(r3, "{ length: number; }");
AssertType(a['hi'], "{ length: number; }");
AssertType(a, "A");
AssertType('hi', "string");


