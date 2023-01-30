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

// === tests/cases/compiler/chainedAssignment2.ts ===
declare function AssertType(value:any, type:string):void;
let a: string;
AssertType(a, "string");

let b: number;
AssertType(b, "number");

let c: boolean;
AssertType(c, "boolean");

let d: Date;
AssertType(d, "Date");

let e: RegExp;
AssertType(e, "RegExp");

a = b = c = d = e = null;
AssertType(a = b = c = d = e = null, "null");
AssertType(a, "string");
AssertType(b = c = d = e = null, "null");
AssertType(b, "number");
AssertType(c = d = e = null, "null");
AssertType(c, "boolean");
AssertType(d = e = null, "null");
AssertType(d, "Date");
AssertType(e = null, "null");
AssertType(e, "RegExp");
AssertType(null, "null");



