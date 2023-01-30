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

// === tests/cases/compiler/voidUndefinedReduction.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #42786

function isDefined<T>(value: T | undefined | null | void): value is T {
AssertType(value !== undefined && value !== null, "boolean");
AssertType(value !== undefined, "boolean");
AssertType(value, "union");
AssertType(undefined, "undefined");
AssertType(value !== null, "boolean");
AssertType(value, "union");
AssertType(null, "null");
  return value !== undefined && value !== null;
}

declare const foo: string | undefined;
AssertType(foo, "union");

if (isDefined(foo)) {
  console.log(foo.toUpperCase()); 
AssertType(console.log(foo.toUpperCase()), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(foo.toUpperCase(), "string");
AssertType(foo.toUpperCase, "() => string");
}


