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

// === tests/cases/compiler/constEnumSyntheticNodesComments.ts ===
declare function AssertType(value:any, type:string):void;
const enum En { A, B, C, D }

function assert<T>(x: T) {
AssertType(x, "T");
    return x;
}

function verify(a: En) {
    switch (a) {
AssertType(a, "En");

        case En.A:
AssertType(En.A, "En.A");

AssertType(assert<0>(a), "int");
AssertType(assert, "<T>(T) => T");
AssertType(a, "En.A");
            return assert<0>(a);

        case En["B"]:
AssertType(En["B"], "En.B");
AssertType(En, "typeof En");
AssertType("B", "string");

AssertType(assert<1>(a), "int");
AssertType(assert, "<T>(T) => T");
AssertType(a, "En.B");
            return assert<1>(a);

        case En[`C`]:
AssertType(En[`C`], "En.C");
AssertType(En, "typeof En");
AssertType(`C`, "string");

AssertType(assert<2>(a), "int");
AssertType(assert, "<T>(T) => T");
AssertType(a, "En.C");
            return assert<2>(a);

        case En["\u{44}"]:
AssertType(En["\u{44}"], "En.D");
AssertType(En, "typeof En");
AssertType("\u{44}", "string");

AssertType(assert<3>(a), "int");
AssertType(assert, "<T>(T) => T");
AssertType(a, "En.D");
            return assert<3>(a);
    }
}

