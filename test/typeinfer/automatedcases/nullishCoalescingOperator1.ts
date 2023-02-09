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

// === tests/cases/conformance/expressions/nullishCoalescingOperator/nullishCoalescingOperator1.ts ===
declare function AssertType(value:any, type:string):void;
declare const a1: string | undefined | null
AssertType(a1, "union");
AssertType(null, "null");

declare const a2: string | undefined | null
AssertType(a2, "union");
AssertType(null, "null");

declare const a3: string | undefined | null
AssertType(a3, "union");
AssertType(null, "null");

declare const a4: string | undefined | null
AssertType(a4, "union");
AssertType(null, "null");

declare const b1: number | undefined | null
AssertType(b1, "union");
AssertType(null, "null");

declare const b2: number | undefined | null
AssertType(b2, "union");
AssertType(null, "null");

declare const b3: number | undefined | null
AssertType(b3, "union");
AssertType(null, "null");

declare const b4: number | undefined | null
AssertType(b4, "union");
AssertType(null, "null");

declare const c1: boolean | undefined | null
AssertType(c1, "union");
AssertType(null, "null");

declare const c2: boolean | undefined | null
AssertType(c2, "union");
AssertType(null, "null");

declare const c3: boolean | undefined | null
AssertType(c3, "union");
AssertType(null, "null");

declare const c4: boolean | undefined | null
AssertType(c4, "union");
AssertType(null, "null");

interface I { a: string }
declare const d1: I | undefined | null
AssertType(d1, "union");
AssertType(null, "null");

declare const d2: I | undefined | null
AssertType(d2, "union");
AssertType(null, "null");

declare const d3: I | undefined | null
AssertType(d3, "union");
AssertType(null, "null");

declare const d4: I | undefined | null
AssertType(d4, "union");
AssertType(null, "null");

const aa1 = a1 ?? 'whatever';
AssertType(aa1, "string");
AssertType(a1 ?? 'whatever', "string");
AssertType(a1, "union");
AssertType('whatever', "string");

const aa2 = a2 ?? 'whatever';
AssertType(aa2, "string");
AssertType(a2 ?? 'whatever', "string");
AssertType(a2, "union");
AssertType('whatever', "string");

const aa3 = a3 ?? 'whatever';
AssertType(aa3, "string");
AssertType(a3 ?? 'whatever', "string");
AssertType(a3, "union");
AssertType('whatever', "string");

const aa4 = a4 ?? 'whatever';
AssertType(aa4, "string");
AssertType(a4 ?? 'whatever', "string");
AssertType(a4, "union");
AssertType('whatever', "string");

const bb1 = b1 ?? 1;
AssertType(bb1, "number");
AssertType(b1 ?? 1, "number");
AssertType(b1, "union");
AssertType(1, "int");

const bb2 = b2 ?? 1;
AssertType(bb2, "number");
AssertType(b2 ?? 1, "number");
AssertType(b2, "union");
AssertType(1, "int");

const bb3 = b3 ?? 1;
AssertType(bb3, "number");
AssertType(b3 ?? 1, "number");
AssertType(b3, "union");
AssertType(1, "int");

const bb4 = b4 ?? 1;
AssertType(bb4, "number");
AssertType(b4 ?? 1, "number");
AssertType(b4, "union");
AssertType(1, "int");

const cc1 = c1 ?? true;
AssertType(cc1, "boolean");
AssertType(c1 ?? true, "boolean");
AssertType(c1, "union");
AssertType(true, "boolean");

const cc2 = c2 ?? true;
AssertType(cc2, "boolean");
AssertType(c2 ?? true, "boolean");
AssertType(c2, "union");
AssertType(true, "boolean");

const cc3 = c3 ?? true;
AssertType(cc3, "boolean");
AssertType(c3 ?? true, "boolean");
AssertType(c3, "union");
AssertType(true, "boolean");

const cc4 = c4 ?? true;
AssertType(cc4, "boolean");
AssertType(c4 ?? true, "boolean");
AssertType(c4, "union");
AssertType(true, "boolean");

const dd1 = d1 ?? {b: 1};
AssertType(dd1, "union");
AssertType(d1 ?? {b: 1}, "union");
AssertType(d1, "union");
AssertType({b: 1}, "{ b: number; }");
AssertType(b, "number");
AssertType(1, "int");

const dd2 = d2 ?? {b: 1};
AssertType(dd2, "union");
AssertType(d2 ?? {b: 1}, "union");
AssertType(d2, "union");
AssertType({b: 1}, "{ b: number; }");
AssertType(b, "number");
AssertType(1, "int");

const dd3 = d3 ?? {b: 1};
AssertType(dd3, "union");
AssertType(d3 ?? {b: 1}, "union");
AssertType(d3, "union");
AssertType({b: 1}, "{ b: number; }");
AssertType(b, "number");
AssertType(1, "int");

const dd4 = d4 ?? {b: 1};
AssertType(dd4, "union");
AssertType(d4 ?? {b: 1}, "union");
AssertType(d4, "union");
AssertType({b: 1}, "{ b: number; }");
AssertType(b, "number");
AssertType(1, "int");

// Repro from #34635

declare function foo(): void;

const maybeBool = false;
AssertType(maybeBool, "boolean");
AssertType(false, "boolean");

if (!(maybeBool ?? true)) {
    foo();
AssertType(foo(), "void");
AssertType(foo, "() => void");
}

if (maybeBool ?? true) {
    foo();
AssertType(foo(), "void");
AssertType(foo, "() => void");
}
else {
    foo();
AssertType(foo(), "void");
AssertType(foo, "() => void");
}

if (false ?? true) {
    foo();
AssertType(foo(), "void");
AssertType(foo, "() => void");
}
else {
    foo();
AssertType(foo(), "void");
AssertType(foo, "() => void");
}


