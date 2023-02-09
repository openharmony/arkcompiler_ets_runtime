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

// === tests/cases/compiler/indexerAssignability.ts ===
declare function AssertType(value:any, type:string):void;
let a: { [s: string]: string; };
AssertType(a, "{ [string]: string; }");
AssertType(s, "string");

let b: { [n: number]: string; };
AssertType(b, "{ [number]: string; }");
AssertType(n, "number");

let c: {};
AssertType(c, "{}");

a = b;
AssertType(a = b, "{ [number]: string; }");
AssertType(a, "{ [string]: string; }");
AssertType(b, "{ [number]: string; }");

a = c;
AssertType(a = c, "{}");
AssertType(a, "{ [string]: string; }");
AssertType(c, "{}");

b = a;
AssertType(b = a, "{ [string]: string; }");
AssertType(b, "{ [number]: string; }");
AssertType(a, "{ [string]: string; }");

b = c;
AssertType(b = c, "{}");
AssertType(b, "{ [number]: string; }");
AssertType(c, "{}");

c = a;
AssertType(c = a, "{ [string]: string; }");
AssertType(c, "{}");
AssertType(a, "{ [string]: string; }");

c = b;
AssertType(c = b, "{ [number]: string; }");
AssertType(c, "{}");
AssertType(b, "{ [number]: string; }");


