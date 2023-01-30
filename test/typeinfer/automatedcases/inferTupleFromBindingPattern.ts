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

// === tests/cases/compiler/inferTupleFromBindingPattern.ts ===
declare function AssertType(value:any, type:string):void;
declare function f<T>(cb: () => T): T;
const [e1, e2, e3] = f(() => [1, "hi", true]);
AssertType(e1, "number");
AssertType(e2, "string");
AssertType(e3, "boolean");
AssertType(f(() => [1, "hi", true]), "[number, string, boolean]");
AssertType(f, "<T>(() => T) => T");
AssertType(() => [1, "hi", true], "() => [number, string, boolean]");
AssertType([1, "hi", true], "[number, string, true]");
AssertType(1, "int");
AssertType("hi", "string");
AssertType(true, "boolean");


