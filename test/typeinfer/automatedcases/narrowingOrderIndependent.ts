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

// === tests/cases/compiler/narrowingOrderIndependent.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #36709

class A {
    constructor(public stringOrUndefined: string | undefined) {}
}

class B {
    constructor(public str: string) {}
}

const a = new A("123");
AssertType(a, "A");
AssertType(new A("123"), "A");
AssertType(A, "typeof A");
AssertType("123", "string");

if (a instanceof A && a.stringOrUndefined) {
    new B(a.stringOrUndefined)
AssertType(new B(a.stringOrUndefined), "B");
AssertType(B, "typeof B");
AssertType(a.stringOrUndefined, "string");
}

if (a.stringOrUndefined && a instanceof A) {
    new B(a.stringOrUndefined)
AssertType(new B(a.stringOrUndefined), "B");
AssertType(B, "typeof B");
AssertType(a.stringOrUndefined, "string");
}

if (a instanceof A) {
    if (a.stringOrUndefined) {
AssertType(a.stringOrUndefined, "union");

        new B(a.stringOrUndefined)
AssertType(new B(a.stringOrUndefined), "B");
AssertType(B, "typeof B");
AssertType(a.stringOrUndefined, "string");
    }
}

if (a.stringOrUndefined) {
    if (a instanceof A) {
AssertType(a instanceof A, "boolean");
AssertType(a, "A");
AssertType(A, "typeof A");

        new B(a.stringOrUndefined)
AssertType(new B(a.stringOrUndefined), "B");
AssertType(B, "typeof B");
AssertType(a.stringOrUndefined, "string");
    }
}


