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

// === tests/cases/compiler/restIntersection.ts ===
declare function AssertType(value:any, type:string):void;
let intersection: { x: number, y: number } & { w: string, z: string };
AssertType(intersection, "{ x: number; y: number; } & { w: string; z: string; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType(w, "string");
AssertType(z, "string");

let rest1: { y: number, w: string, z: string };
AssertType(rest1, "{ y: number; w: string; z: string; }");
AssertType(y, "number");
AssertType(w, "string");
AssertType(z, "string");

let {x, ...rest1 } = intersection;
AssertType(x, "number");
AssertType(rest1, "{ y: number; w: string; z: string; }");
AssertType(intersection, "{ x: number; y: number; } & { w: string; z: string; }");


