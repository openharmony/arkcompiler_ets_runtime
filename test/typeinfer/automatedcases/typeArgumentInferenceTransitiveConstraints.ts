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

// === tests/cases/conformance/expressions/functionCalls/typeArgumentInferenceTransitiveConstraints.ts ===
declare function AssertType(value:any, type:string):void;
function fn<A extends Date, B extends A, C extends B>(a: A, b: B, c: C) {
AssertType([a, b, c], "A[]");
AssertType(a, "A");
AssertType(b, "B");
AssertType(c, "C");
    return [a, b, c];
}

let d = fn(new Date(), new Date(), new Date());
AssertType(d, "Date[]");
AssertType(fn(new Date(), new Date(), new Date()), "Date[]");
AssertType(fn, "<A extends Date, B extends A, C extends B>(A, B, C) => A[]");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");
AssertType(new Date(), "Date");
AssertType(Date, "DateConstructor");

let d: Date[]; // Should be OK (d should be Date[])
AssertType(d, "Date[]");


