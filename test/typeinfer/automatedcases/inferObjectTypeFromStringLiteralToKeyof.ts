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

// === tests/cases/compiler/inferObjectTypeFromStringLiteralToKeyof.ts ===
declare function AssertType(value:any, type:string):void;
declare function inference1<T>(name: keyof T): T;
declare function inference2<T>(target: T, name: keyof T): T;
declare let two: "a" | "d";
AssertType(two, "union");

const x = inference1(two);
AssertType(x, "{ a: any; d: any; }");
AssertType(inference1(two), "{ a: any; d: any; }");
AssertType(inference1, "<T>(keyof T) => T");
AssertType(two, "union");

const y = inference2({ a: 1, b: 2, c: 3, d(n) { 
AssertType(y, "{ a: number; b: number; c: number; d(any): any; }");
AssertType(inference2({ a: 1, b: 2, c: 3, d(n) { return n } }, two), "{ a: number; b: number; c: number; d(any): any; }");
AssertType(inference2, "<T>(T, keyof T) => T");
AssertType({ a: 1, b: 2, c: 3, d(n) { return n } }, "{ a: number; b: number; c: number; d(any): any; }");
AssertType(a, "number");
AssertType(1, "int");
AssertType(b, "number");
AssertType(2, "int");
AssertType(c, "number");
AssertType(3, "int");
AssertType(d, "(any) => any");
AssertType(n, "any");
AssertType(n, "any");
AssertType(two, "union");
return n } }, two);


