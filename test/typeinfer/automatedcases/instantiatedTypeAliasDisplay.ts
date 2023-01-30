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

// === tests/cases/compiler/instantiatedTypeAliasDisplay.ts ===
declare function AssertType(value:any, type:string):void;
// Repros from #12066

interface X<A> {
    a: A;
}
interface Y<B> {
    b: B;
}
type Z<A, B> = X<A> | Y<B>;

declare function f1<A>(): Z<A, number>;
declare function f2<A, B, C, D, E>(a: A, b: B, c: C, d: D): Z<A, string[]>;

const x1 = f1<string>();  // Z<string, number>
AssertType(x1, "Z<string, number>");
AssertType(f1<string>(), "Z<string, number>");
AssertType(f1, "<A>() => Z<A, number>");

const x2 = f2({}, {}, {}, {});  // Z<{}, string[]>
AssertType(x2, "Z<{}, string[]>");
AssertType(f2({}, {}, {}, {}), "Z<{}, string[]>");
AssertType(f2, "<A, B, C, D, E>(A, B, C, D) => Z<A, string[]>");
AssertType({}, "{}");
AssertType({}, "{}");
AssertType({}, "{}");
AssertType({}, "{}");


