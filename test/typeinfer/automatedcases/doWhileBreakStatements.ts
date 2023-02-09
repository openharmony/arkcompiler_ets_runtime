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

// === tests/cases/conformance/statements/breakStatements/doWhileBreakStatements.ts ===
declare function AssertType(value:any, type:string):void;
do {
    break;
} while(true)

ONE:
do {
    break ONE;
AssertType(ONE, "any");
}
while (true)

TWO:
THREE:
do {
    break THREE;
AssertType(THREE, "any");

}while (true)

FOUR:
do {
    FIVE:
AssertType(FIVE, "any");

    do {
        break FOUR;
AssertType(FOUR, "any");

    }while (true)
AssertType(true, "boolean");

}while (true)

do {
    SIX:
AssertType(SIX, "any");

    do break SIX; while(true)
AssertType(SIX, "any");
AssertType(true, "boolean");

}while (true)

SEVEN:
do do do break SEVEN; while (true) while (true)  while (true)

EIGHT:
do{
    let fn = function () { 
AssertType(fn, "() => void");

AssertType(function () { }, "() => void");
}

    break EIGHT;
AssertType(EIGHT, "any");

}while(true)


