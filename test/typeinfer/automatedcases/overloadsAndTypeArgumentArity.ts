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

// === tests/cases/compiler/overloadsAndTypeArgumentArity.ts ===
declare function AssertType(value:any, type:string):void;
declare function Callbacks(flags?: string): void;
declare function Callbacks<T>(flags?: string): void;
declare function Callbacks<T1, T2>(flags?: string): void;
declare function Callbacks<T1, T2, T3>(flags?: string): void;

Callbacks<number, string, boolean>('s'); // no error
AssertType(Callbacks<number, string, boolean>('s'), "void");
AssertType(Callbacks, "{ (?string): void; <T>(?string): void; <T1, T2>(?string): void; <T1, T2, T3>(?string): void; }");
AssertType('s', "string");

new Callbacks<number, string, boolean>('s'); // no error
AssertType(new Callbacks<number, string, boolean>('s'), "any");
AssertType(Callbacks, "{ (?string): void; <T>(?string): void; <T1, T2>(?string): void; <T1, T2, T3>(?string): void; }");
AssertType('s', "string");


