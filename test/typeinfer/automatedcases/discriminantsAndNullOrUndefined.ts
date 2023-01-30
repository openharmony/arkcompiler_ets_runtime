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

// === tests/cases/compiler/discriminantsAndNullOrUndefined.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #10228

interface A { kind: 'A'; }
interface B { kind: 'B'; }

type C = A | B | undefined;

function never(_: never): never {
    throw new Error();
AssertType(new Error(), "Error");
AssertType(Error, "ErrorConstructor");
}

function useA(_: A): void { }
function useB(_: B): void { }

declare let c: C;
AssertType(c, "C");

if (c !== undefined) {
    switch (c.kind) {
AssertType(c.kind, "union");

        case 'A': useA(c); break;
AssertType('A', "string");
AssertType(useA(c), "void");
AssertType(useA, "(A) => void");
AssertType(c, "A");

        case 'B': useB(c); break;
AssertType('B', "string");
AssertType(useB(c), "void");
AssertType(useB, "(B) => void");
AssertType(c, "B");

        default: never(c);
AssertType(never(c), "never");
AssertType(never, "(never) => never");
AssertType(c, "never");
    }
}

