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

// === tests/cases/conformance/es6/destructuring/destructuringCatch.ts ===
declare function AssertType(value:any, type:string):void;
try {
    throw [0, 1];
AssertType([0, 1], "number[]");
AssertType(0, "int");
AssertType(1, "int");
}
catch ([a, b]) {
    a + b;
AssertType(a + b, "any");
AssertType(a, "any");
AssertType(b, "any");
}

try {
    throw { a: 0, b: 1 };
AssertType({ a: 0, b: 1 }, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(0, "int");
AssertType(b, "number");
AssertType(1, "int");
}
catch ({a, b}) {
    a + b;
AssertType(a + b, "any");
AssertType(a, "any");
AssertType(b, "any");
}

try {
    throw [{ x: [0], z: 1 }];
AssertType([{ x: [0], z: 1 }], "{ x: number[]; z: number; }[]");
AssertType({ x: [0], z: 1 }, "{ x: number[]; z: number; }");
AssertType(x, "number[]");
AssertType([0], "number[]");
AssertType(0, "int");
AssertType(z, "number");
AssertType(1, "int");
}
catch ([{x: [y], z}]) {
    y + z;
AssertType(y + z, "any");
AssertType(y, "any");
AssertType(z, "any");
}

// Test of comment ranges. A fix to GH#11755 should update this.
try {
}
catch (/*Test comment ranges*/[/*a*/a]) {

}


