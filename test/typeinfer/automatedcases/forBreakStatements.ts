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

// === tests/cases/conformance/statements/breakStatements/forBreakStatements.ts ===
declare function AssertType(value:any, type:string):void;
for (; ;) {
    break;
}

ONE:
for (; ;) {
    break ONE;
AssertType(ONE, "any");
}

TWO:
THREE:
for (; ;) {
    break THREE;
AssertType(THREE, "any");
}

FOUR:
for (; ;) {
    FIVE:
AssertType(FIVE, "any");

    for (; ;) {
        break FOUR;
AssertType(FOUR, "any");
    }
}

for (; ;) {
    SIX:
AssertType(SIX, "any");

    for (; ;) break SIX;
AssertType(SIX, "any");
}

SEVEN:
for (; ;) for (; ;) for (; ;) break SEVEN;

EIGHT:
for (; ;) {
    let fn = function () { 
AssertType(fn, "() => void");

AssertType(function () { }, "() => void");
}

    break EIGHT;
AssertType(EIGHT, "any");
}


