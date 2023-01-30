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

// === tests/cases/compiler/overloadCrash.ts ===
declare function AssertType(value:any, type:string):void;
interface I1 {a:number; b:number;};
interface I2 {c:number; d:number;};
interface I3 {a:number; b:number; c:number; d:number;};

declare function foo(...n:I1[]);
declare function foo(n1:I2, n3:I2);

let i3:I3;
AssertType(i3, "I3");

foo(i3, i3); // should not crash the compiler :)
AssertType(foo(i3, i3), "any");
AssertType(foo, "{ (...I1[]): any; (I2, I2): any; }");
AssertType(i3, "I3");
AssertType(i3, "I3");


