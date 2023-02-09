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

// === tests/cases/conformance/es2020/bigintMissingES2020.ts ===
declare function AssertType(value:any, type:string):void;
declare function test<A, B extends A>(): void;

test<{t?: string}, object>();
AssertType(test<{t?: string}, object>(), "void");
AssertType(test, "<A, B extends A>() => void");
AssertType(t, "string");

test<{t?: string}, bigint>();
AssertType(test<{t?: string}, bigint>(), "void");
AssertType(test, "<A, B extends A>() => void");
AssertType(t, "string");

// no error when bigint is used even when ES2020 lib is not present


