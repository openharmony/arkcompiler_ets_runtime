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

// === tests/cases/conformance/types/typeRelationships/comparable/equalityWithUnionTypes01.ts ===
declare function AssertType(value:any, type:string):void;
interface I1 {
    p1: number
}

interface I2 extends I1 {
    p2: number;
}

let x = { p1: 10, p2: 20 };
AssertType(x, "{ p1: number; p2: number; }");
AssertType({ p1: 10, p2: 20 }, "{ p1: number; p2: number; }");
AssertType(p1, "number");
AssertType(10, "int");
AssertType(p2, "number");
AssertType(20, "int");

let y: number | I2 = x;
AssertType(y, "union");
AssertType(x, "{ p1: number; p2: number; }");

let z: I1 = x;
AssertType(z, "I1");
AssertType(x, "{ p1: number; p2: number; }");

if (y === z || z === y) {
}
else if (y !== z || z !== y) {
}
else if (y == z || z == y) {
}
else if (y != z || z != y) {
}

