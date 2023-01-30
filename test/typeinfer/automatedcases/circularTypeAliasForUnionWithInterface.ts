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

// === tests/cases/conformance/types/typeAliases/circularTypeAliasForUnionWithInterface.ts ===
declare function AssertType(value:any, type:string):void;
let v0: T0;
AssertType(v0, "T0");

type T0 = string | I0;
interface I0 {
    x: T0;
}

let v1: T1;
AssertType(v1, "T1");

type T1 = string | I1;
interface I1 {
    (): T1;
}

let v2: T2;
AssertType(v2, "T2");

type T2 = string | I2;
interface I2 {
    new (): T2;
}

let v3: T3;
AssertType(v3, "T3");

type T3 = string | I3;
interface I3 {
    [x: number]: T3;
}

let v4: T4;
AssertType(v4, "T4");

type T4 = string | I4;
interface I4 {
    [x: string]: T4;
}


