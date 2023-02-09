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

// === tests/cases/compiler/freshLiteralInference.ts ===
declare function AssertType(value:any, type:string):void;
declare function f1<T extends "1" | "2" | "3">(x: T): T;

const value = f1("1");  // regular "1"
AssertType(value, "string");
AssertType(f1("1"), "string");
AssertType(f1, "<T extends union>(T) => T");
AssertType("1", "string");

let x1 = value;  // regular "1"
AssertType(x1, "string");
AssertType(value, "string");

declare function f2<T extends "1" | "2" | "3">(x: { value: T }): { value: T };

const obj2 = f2({ value: "1" });  // { value: regular "1" 
AssertType(obj2, "{ value: "1"; }");

AssertType(f2({ value: "1" }), "{ value: "1"; }");

AssertType(f2, "<T extends union>({ value: T; }) => { value: T; }");

AssertType({ value: "1" }, "{ value: "1"; }");

AssertType(value, "string");

AssertType("1", "string");
}

let x2 = obj2.value;  // regular "1"
AssertType(x2, "string");
AssertType(obj2.value, "string");

declare function f3<T extends { value: "1" | "2" | "3" }>(obj: T): T;

const obj3 = f3({ value: "1" });  // before: { value: fresh "1" 
AssertType(obj3, "{ value: "1"; }");

AssertType(f3({ value: "1" }), "{ value: "1"; }");

AssertType(f3, "<T extends { value: union; }>(T) => T");

AssertType({ value: "1" }, "{ value: "1"; }");

AssertType(value, "string");

AssertType("1", "string");
}

let x3 = obj3.value;  // before: string, after: "1"
AssertType(x3, "string");
AssertType(obj3.value, "string");


