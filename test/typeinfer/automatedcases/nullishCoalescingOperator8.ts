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

// === tests/cases/conformance/expressions/nullishCoalescingOperator/nullishCoalescingOperator8.ts ===
declare function AssertType(value:any, type:string):void;
declare const a: { p: string | undefined, m(): string | undefined };
AssertType(a, "{ p: union; m(): union; }");
AssertType(p, "union");
AssertType(m, "() => union");

declare const b: { p: string | undefined, m(): string | undefined };
AssertType(b, "{ p: union; m(): union; }");
AssertType(p, "union");
AssertType(m, "() => union");

const n1 = a.p ?? "default";
AssertType(n1, "string");
AssertType(a.p ?? "default", "string");
AssertType(a.p, "union");
AssertType("default", "string");

const n2 = a.m() ?? "default";
AssertType(n2, "string");
AssertType(a.m() ?? "default", "string");
AssertType(a.m(), "union");
AssertType(a.m, "() => union");
AssertType("default", "string");

const n3 = a.m() ?? b.p ?? b.m() ?? "default";;
AssertType(n3, "string");
AssertType(a.m() ?? b.p ?? b.m() ?? "default", "string");
AssertType(a.m() ?? b.p ?? b.m(), "union");
AssertType(a.m() ?? b.p, "union");
AssertType(a.m(), "union");
AssertType(a.m, "() => union");
AssertType(b.p, "union");
AssertType(b.m(), "union");
AssertType(b.m, "() => union");
AssertType("default", "string");


