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

// === tests/cases/conformance/expressions/nullishCoalescingOperator/nullishCoalescingOperator2.ts ===
declare function AssertType(value:any, type:string):void;
declare const a1: 'literal' | undefined | null
AssertType(a1, "union");
AssertType(null, "null");

declare const a2: '' | undefined | null
AssertType(a2, "union");
AssertType(null, "null");

declare const a3: 1 | undefined | null
AssertType(a3, "union");
AssertType(null, "null");

declare const a4: 0 | undefined | null
AssertType(a4, "union");
AssertType(null, "null");

declare const a5: true | undefined | null
AssertType(a5, "union");
AssertType(true, "boolean");
AssertType(null, "null");

declare const a6: false | undefined | null
AssertType(a6, "union");
AssertType(false, "boolean");
AssertType(null, "null");

declare const a7: unknown | null
AssertType(a7, "unknown");
AssertType(null, "null");

declare const a8: never | null
AssertType(a8, "null");
AssertType(null, "null");

declare const a9: any | null
AssertType(a9, "any");
AssertType(null, "null");


const aa1 = a1 ?? 'whatever'
AssertType(aa1, "union");
AssertType(a1 ?? 'whatever', "union");
AssertType(a1, "union");
AssertType('whatever', "string");

const aa2 = a2 ?? 'whatever'
AssertType(aa2, "union");
AssertType(a2 ?? 'whatever', "union");
AssertType(a2, "union");
AssertType('whatever', "string");

const aa3 = a3 ?? 'whatever'
AssertType(aa3, "union");
AssertType(a3 ?? 'whatever', "union");
AssertType(a3, "union");
AssertType('whatever', "string");

const aa4 = a4 ?? 'whatever'
AssertType(aa4, "union");
AssertType(a4 ?? 'whatever', "union");
AssertType(a4, "union");
AssertType('whatever', "string");

const aa5 = a5 ?? 'whatever'
AssertType(aa5, "union");
AssertType(a5 ?? 'whatever', "union");
AssertType(a5, "union");
AssertType('whatever', "string");

const aa6 = a6 ?? 'whatever'
AssertType(aa6, "union");
AssertType(a6 ?? 'whatever', "union");
AssertType(a6, "union");
AssertType('whatever', "string");

const aa7 = a7 ?? 'whatever'
AssertType(aa7, "{}");
AssertType(a7 ?? 'whatever', "{}");
AssertType(a7, "unknown");
AssertType('whatever', "string");

const aa8 = a8 ?? 'whatever'
AssertType(aa8, "string");
AssertType(a8 ?? 'whatever', "string");
AssertType(a8, "null");
AssertType('whatever', "string");

const aa9 = a9 ?? 'whatever'
AssertType(aa9, "any");
AssertType(a9 ?? 'whatever', "any");
AssertType(a9, "any");
AssertType('whatever', "string");


