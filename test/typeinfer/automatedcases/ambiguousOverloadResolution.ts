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

// === tests/cases/compiler/ambiguousOverloadResolution.ts ===
declare function AssertType(value:any, type:string):void;
class A { }
class B extends A { x: number; }

declare function f(p: A, q: B): number;
declare function f(p: B, q: A): string;

let x: B;
AssertType(x, "B");

let t: number = f(x, x); // Not an error
AssertType(t, "number");
AssertType(f(x, x), "number");
AssertType(f, "{ (A, B): number; (B, A): string; }");
AssertType(x, "B");
AssertType(x, "B");


