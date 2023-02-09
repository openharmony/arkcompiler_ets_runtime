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

// === tests/cases/conformance/statements/continueStatements/forContinueStatements.ts ===
declare function AssertType(value:any, type:string):void;
for (; ;) {
    continue;
}

ONE:
for (; ;) {
    continue ONE;
AssertType(ONE, "any");
}

TWO:
THREE:
for (; ;) {
    continue THREE;
AssertType(THREE, "any");
}

FOUR:
for (; ;) {
    FIVE:
AssertType(FIVE, "any");

    for (; ;) {
        continue FOUR;
AssertType(FOUR, "any");
    }
}

for (; ;) {
    SIX:
AssertType(SIX, "any");

    for (; ;) continue SIX;
AssertType(SIX, "any");
}

SEVEN:
for (; ;) for (; ;) for (; ;) continue SEVEN;

EIGHT:
for (; ;) {
    let fn = function () { 
AssertType(fn, "() => void");

AssertType(function () { }, "() => void");
}

    continue EIGHT;
AssertType(EIGHT, "any");
}


