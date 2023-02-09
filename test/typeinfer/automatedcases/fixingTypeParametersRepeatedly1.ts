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

// === tests/cases/compiler/fixingTypeParametersRepeatedly1.ts ===
declare function AssertType(value:any, type:string):void;
declare function f<T>(x: T, y: (p: T) => T, z: (p: T) => T): T;
f("", x => null, x => x.toLowerCase());
AssertType(f("", x => null, x => x.toLowerCase()), "string");
AssertType(f, "<T>(T, (T) => T, (T) => T) => T");
AssertType("", "string");
AssertType(x => null, "(string) => any");
AssertType(x, "string");
AssertType(null, "null");
AssertType(x => x.toLowerCase(), "(string) => string");
AssertType(x, "string");
AssertType(x.toLowerCase(), "string");
AssertType(x.toLowerCase, "() => string");

// First overload of g should type check just like f
declare function g<T>(x: T, y: (p: T) => T, z: (p: T) => T): T;
declare function g();
g("", x => null, x => x.toLowerCase());
AssertType(g("", x => null, x => x.toLowerCase()), "any");
AssertType(g, "{ <T>(T, (T) => T, (T) => T): T; (): any; }");
AssertType("", "string");
AssertType(x => null, "(string) => any");
AssertType(x, "string");
AssertType(null, "null");
AssertType(x => x.toLowerCase(), "(string) => string");
AssertType(x, "string");
AssertType(x.toLowerCase(), "string");
AssertType(x.toLowerCase, "() => string");


