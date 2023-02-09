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

// === tests/cases/compiler/discriminantsAndTypePredicates.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #10145

interface A { type: 'A' }
interface B { type: 'B' }

function isA(x: A | B): x is A { 
AssertType(x.type === 'A', "boolean");
return x.type === 'A'; 

AssertType(x.type, "union");

AssertType('A', "string");
}

function isB(x: A | B): x is B { 
AssertType(x.type === 'B', "boolean");
return x.type === 'B'; 

AssertType(x.type, "union");

AssertType('B', "string");
}

function foo1(x: A | B): any {
    x;  // A | B
AssertType(x, "union");

    if (isA(x)) {
AssertType(isA(x), "boolean");
AssertType(isA, "(union) => x is A");
AssertType(x, "union");

AssertType(x, "A");
        return x;  // A
    }
    x;  // B
AssertType(x, "B");

    if (isB(x)) {
AssertType(isB(x), "boolean");
AssertType(isB, "(union) => x is B");
AssertType(x, "B");

AssertType(x, "B");
        return x;  // B
    }
    x;  // never
AssertType(x, "never");
}

function foo2(x: A | B): any {
    x;  // A | B
AssertType(x, "union");

    if (x.type === 'A') {
AssertType(x.type === 'A', "boolean");
AssertType(x.type, "union");
AssertType('A', "string");

AssertType(x, "A");
        return x;  // A
    }
    x;  // B
AssertType(x, "B");

    if (x.type === 'B') {
AssertType(x.type === 'B', "boolean");
AssertType(x.type, "string");
AssertType('B', "string");

AssertType(x, "B");
        return x;  // B
    }
    x;  // never
AssertType(x, "never");
}

