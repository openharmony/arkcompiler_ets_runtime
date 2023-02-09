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

// === tests/cases/compiler/discriminantElementAccessCheck.ts ===
declare function AssertType(value:any, type:string):void;
type U = TypeA | TypeB;

interface TypeA {
    kind: 'A';
    a: number;
}
interface TypeB {
    kind: 'B';
    b: string;
}

function assertNever(x: never) {
AssertType(x, "never");
    return x;
}

function IfWithString(val: U) {
    if (val['kind'] === 'B') {
AssertType(val['kind'] === 'B', "boolean");
AssertType(val['kind'], "union");
AssertType(val, "U");
AssertType('kind', "string");
AssertType('B', "string");

AssertType(val.b, "string");
        return val.b;

    } else {
AssertType(val.a, "number");
        return val.a;
    }
}

function SwitchWithString(val: U) {
    switch (val['kind']) {
AssertType(val['kind'], "union");
AssertType(val, "U");
AssertType('kind', "string");

        case 'A':
AssertType('A', "string");

AssertType(val.a, "number");
            return val.a;

        case 'B':
AssertType('B', "string");

AssertType(val.b, "string");
            return val.b;

        default:
AssertType(assertNever(val), "never");
AssertType(assertNever, "(never) => never");
AssertType(val, "never");
            return assertNever(val);
    }
}

function IfWithTemplate(val: U) {
    if (val[`kind`] === 'B') {
AssertType(val[`kind`] === 'B', "boolean");
AssertType(val[`kind`], "union");
AssertType(val, "U");
AssertType(`kind`, "string");
AssertType('B', "string");

AssertType(val.b, "string");
        return val.b;

    } else {
AssertType(val.a, "number");
        return val.a;
    }
}

function SwitchWithTemplate(val: U) {
    switch (val[`kind`]) {
AssertType(val[`kind`], "union");
AssertType(val, "U");
AssertType(`kind`, "string");

        case 'A':
AssertType('A', "string");

AssertType(val.a, "number");
            return val.a;

        case 'B':
AssertType('B', "string");

AssertType(val.b, "string");
            return val.b;

        default:
AssertType(assertNever(val), "never");
AssertType(assertNever, "(never) => never");
AssertType(val, "never");
            return assertNever(val);
    }
}

