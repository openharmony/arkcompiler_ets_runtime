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

// === tests/cases/compiler/objectBindingPatternContextuallyTypesArgument.ts ===
declare function AssertType(value:any, type:string):void;
declare function id<T>(x: T): T;
const { f = (x: string) => x.length } = id({ f: x => x.charAt });
AssertType(f, "union");
AssertType((x: string) => x.length, "(string) => number");
AssertType(x, "string");
AssertType(x.length, "number");
AssertType(id({ f: x => x.charAt }), "{ f: (string) => (number) => string; }");
AssertType(id, "<T>(T) => T");
AssertType({ f: x => x.charAt }, "{ f: (string) => (number) => string; }");
AssertType(f, "(string) => (number) => string");
AssertType(x => x.charAt, "(string) => (number) => string");
AssertType(x, "string");
AssertType(x.charAt, "(number) => string");


