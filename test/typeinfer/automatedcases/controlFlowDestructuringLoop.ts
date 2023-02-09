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

// === tests/cases/compiler/controlFlowDestructuringLoop.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #28758

interface NumVal { val: number; }
interface StrVal { val: string; }
type Val = NumVal | StrVal;

function isNumVal(x: Val): x is NumVal {
AssertType(typeof x.val === 'number', "boolean");
AssertType(typeof x.val, "union");
AssertType(x.val, "union");
AssertType('number', "string");
    return typeof x.val === 'number';
}

function foo(things: Val[]): void {
    for (const thing of things) {
AssertType(thing, "Val");
AssertType(things, "Val[]");

        if (isNumVal(thing)) {
AssertType(isNumVal(thing), "boolean");
AssertType(isNumVal, "(Val) => x is NumVal");
AssertType(thing, "Val");

            const { val } = thing;
AssertType(val, "number");
AssertType(thing, "NumVal");

            val.toFixed(2);
AssertType(val.toFixed(2), "string");
AssertType(val.toFixed, "(?union) => string");
AssertType(2, "int");
        }
        else {
            const { val } = thing;
AssertType(val, "string");
AssertType(thing, "StrVal");

            val.length;
AssertType(val.length, "number");
        }
    }
}

