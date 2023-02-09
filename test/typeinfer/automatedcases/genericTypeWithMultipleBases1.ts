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

// === tests/cases/compiler/genericTypeWithMultipleBases1.ts ===
declare function AssertType(value:any, type:string):void;
export interface I1 {
    m1: () => void;
}
 
export interface I2 {
    m2: () => void;
}
 
export interface I3<T> extends I1, I2 {
//export interface I3<T> extends I2, I1 {
    p1: T;
}
 
let x: I3<number>;
AssertType(x, "I3<number>");

x.p1;
AssertType(x.p1, "number");

x.m1();
AssertType(x.m1(), "void");
AssertType(x.m1, "() => void");

x.m2();
AssertType(x.m2(), "void");
AssertType(x.m2, "() => void");



