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

// === tests/cases/conformance/es2021/logicalAssignment/logicalAssignment3.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    baz: 0 | 1 | 42 | undefined | ''
}

declare const result: A;
AssertType(result, "A");

declare const a: A;
AssertType(a, "A");

declare const b: A;
AssertType(b, "A");

declare const c: A;
AssertType(c, "A");

(a.baz) &&= result.baz;
AssertType((a.baz) &&= result.baz, "union");
AssertType((a.baz), "union");
AssertType(a.baz, "union");
AssertType(result.baz, "union");

(b.baz) ||= result.baz;
AssertType((b.baz) ||= result.baz, "union");
AssertType((b.baz), "union");
AssertType(b.baz, "union");
AssertType(result.baz, "union");

(c.baz) ??= result.baz;
AssertType((c.baz) ??= result.baz, "union");
AssertType((c.baz), "union");
AssertType(c.baz, "union");
AssertType(result.baz, "union");



