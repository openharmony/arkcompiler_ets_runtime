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

// === tests/cases/conformance/types/union/unionTypeReduction.ts ===
declare function AssertType(value:any, type:string):void;
interface I2 {
    (): number;
    (q): boolean;
}

interface I3 {
    (): number;
}

let i2: I2, i3: I3;
AssertType(i2, "I2");
AssertType(i3, "I3");

let e1: I2 | I3;
AssertType(e1, "union");

let e2 = i2 || i3;  // Type of e2 immediately reduced to I3
AssertType(e2, "I3");
AssertType(i2 || i3, "I3");
AssertType(i2, "I2");
AssertType(i3, "I3");

let r1 = e1();  // Type of e1 reduced to I3 upon accessing property or signature
AssertType(r1, "number");
AssertType(e1(), "number");
AssertType(e1, "union");

let r2 = e2();
AssertType(r2, "number");
AssertType(e2(), "number");
AssertType(e2, "I3");


