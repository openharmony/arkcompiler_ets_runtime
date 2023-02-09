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

// === tests/cases/conformance/controlFlow/controlFlowOptionalChain2.ts ===
declare function AssertType(value:any, type:string):void;
type A = {
  type: 'A';
  name: string;
}

type B = {
  type: 'B';
}

function funcTwo(arg: A | B | undefined) {
  if (arg?.type === 'B') {
AssertType(arg?.type === 'B', "boolean");
AssertType(arg?.type, "union");
AssertType('B', "string");

    arg; // `B`
AssertType(arg, "B");

    return;
  }

  arg;
AssertType(arg, "union");

  arg?.name;
AssertType(arg?.name, "union");
}

function funcThree(arg: A | B | null) {
  if (arg?.type === 'B') {
AssertType(arg?.type === 'B', "boolean");
AssertType(arg?.type, "union");
AssertType('B', "string");

    arg; // `B`
AssertType(arg, "B");

    return;
  }

  arg;
AssertType(arg, "union");

  arg?.name;
AssertType(arg?.name, "union");
}

type U = { kind: undefined, u: 'u' }
type N = { kind: null, n: 'n' }
type X = { kind: 'X', x: 'x' }

function f1(x: X | U | undefined) {
    if (x?.kind === undefined) {
AssertType(x?.kind === undefined, "boolean");
AssertType(x?.kind, "union");
AssertType(undefined, "undefined");

        x; // U | undefined
AssertType(x, "union");
    }
    else {
        x; // X
AssertType(x, "X");
    }
}

function f2(x: X | N | undefined) {
    if (x?.kind === undefined) {
AssertType(x?.kind === undefined, "boolean");
AssertType(x?.kind, "union");
AssertType(undefined, "undefined");

        x; // undefined
AssertType(x, "undefined");
    }
    else {
        x; // X | N
AssertType(x, "union");
    }
}

function f3(x: X | U | null) {
    if (x?.kind === undefined) {
AssertType(x?.kind === undefined, "boolean");
AssertType(x?.kind, "union");
AssertType(undefined, "undefined");

        x; // U | null
AssertType(x, "union");
    }
    else {
        x; // X
AssertType(x, "X");
    }
}

function f4(x: X | N | null) {
    if (x?.kind === undefined) {
AssertType(x?.kind === undefined, "boolean");
AssertType(x?.kind, "union");
AssertType(undefined, "undefined");

        x; // null
AssertType(x, "null");
    }
    else {
        x; // X | N
AssertType(x, "union");
    }
}

function f5(x: X | U | undefined) {
    if (x?.kind === null) {
AssertType(x?.kind === null, "boolean");
AssertType(x?.kind, "union");
AssertType(null, "null");

        x; // never
AssertType(x, "never");
    }
    else {
        x; // X | U | undefined
AssertType(x, "union");
    }
}

function f6(x: X | N | undefined) {
    if (x?.kind === null) {
AssertType(x?.kind === null, "boolean");
AssertType(x?.kind, "union");
AssertType(null, "null");

        x; // N
AssertType(x, "N");
    }
    else {
        x; // X | undefined
AssertType(x, "union");
    }
}

function f7(x: X | U | null) {
    if (x?.kind === null) {
AssertType(x?.kind === null, "boolean");
AssertType(x?.kind, "union");
AssertType(null, "null");

        x; // never
AssertType(x, "never");
    }
    else {
        x; // X | U | null
AssertType(x, "union");
    }
}

function f8(x: X | N | null) {
    if (x?.kind === null) {
AssertType(x?.kind === null, "boolean");
AssertType(x?.kind, "union");
AssertType(null, "null");

        x; // N
AssertType(x, "N");
    }
    else {
        x; // X | null
AssertType(x, "union");
    }
}


