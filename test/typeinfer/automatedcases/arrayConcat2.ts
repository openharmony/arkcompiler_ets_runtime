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

// === tests/cases/compiler/arrayConcat2.ts ===
declare function AssertType(value:any, type:string):void;
let a: string[] = [];
AssertType(a, "string[]");
AssertType([], "undefined[]");

a.concat("hello", 'world');
AssertType(a.concat("hello", 'world'), "string[]");
AssertType(a.concat, "{ (...ConcatArray<string>[]): string[]; (...(union)[]): string[]; }");
AssertType("hello", "string");
AssertType('world', "string");

a.concat('Hello');
AssertType(a.concat('Hello'), "string[]");
AssertType(a.concat, "{ (...ConcatArray<string>[]): string[]; (...(union)[]): string[]; }");
AssertType('Hello', "string");

let b = new Array<string>();
AssertType(b, "string[]");
AssertType(new Array<string>(), "string[]");
AssertType(Array, "ArrayConstructor");

b.concat('hello');
AssertType(b.concat('hello'), "string[]");
AssertType(b.concat, "{ (...ConcatArray<string>[]): string[]; (...(union)[]): string[]; }");
AssertType('hello', "string");


