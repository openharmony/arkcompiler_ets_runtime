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

// === tests/cases/compiler/typeGuardNarrowsIndexedAccessOfKnownProperty6.ts ===
declare function AssertType(value:any, type:string):void;
declare const aIndex: "key";
AssertType(aIndex, "string");

const a: { key?: { x?: number } } = {};
AssertType(a, "{ key?: union; }");
AssertType(key, "union");
AssertType(x, "union");
AssertType({}, "{}");

if (a[aIndex] && a[aIndex].x) {
    a[aIndex].x // number
AssertType(a[aIndex].x, "number");
AssertType(a[aIndex], "{ x?: union; }");
AssertType(a, "{ key?: union; }");
AssertType(aIndex, "string");
}

declare const bIndex: "key";
AssertType(bIndex, "string");

const b: { key: { x?: number } } = { key: {} };
AssertType(b, "{ key: {    x?: number;}; }");
AssertType(key, "{ x?: union; }");
AssertType(x, "union");
AssertType({ key: {} }, "{ key: {}; }");
AssertType(key, "{}");
AssertType({}, "{}");

if (b[bIndex].x) {
    b[bIndex].x // number
AssertType(b[bIndex].x, "number");
AssertType(b[bIndex], "{ x?: union; }");
AssertType(b, "{ key: { x?: union; }; }");
AssertType(bIndex, "string");
}

declare const cIndex: 1;
AssertType(cIndex, "int");

interface Foo {
    x: number | undefined;
}
const c: Foo[] = [];
AssertType(c, "Foo[]");
AssertType([], "never[]");

if (c[cIndex].x) {
    c[cIndex].x // number
AssertType(c[cIndex].x, "number");
AssertType(c[cIndex], "Foo");
AssertType(c, "Foo[]");
AssertType(cIndex, "int");
}


