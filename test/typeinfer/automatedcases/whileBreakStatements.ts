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

// === tests/cases/conformance/statements/breakStatements/whileBreakStatements.ts ===
declare function AssertType(value:any, type:string):void;
while(true) {
    break;
} 

ONE:

while (true) {
    break ONE;
AssertType(ONE, "any");
}

TWO:
THREE:
while (true) {
    break THREE;
AssertType(THREE, "any");
}

FOUR:
while (true) {
    FIVE:
AssertType(FIVE, "any");

    while (true) {
AssertType(true, "boolean");

        break FOUR;
AssertType(FOUR, "any");
    }
}

while (true) {
    SIX:
AssertType(SIX, "any");

    while (true)
AssertType(true, "boolean");

        break SIX;
AssertType(SIX, "any");
}

SEVEN:
while (true)
    while (true)
        while (true)
            break SEVEN;

EIGHT:
while (true) {
    let fn = function () { 
AssertType(fn, "() => void");

AssertType(function () { }, "() => void");
}

    break EIGHT;
AssertType(EIGHT, "any");
}


