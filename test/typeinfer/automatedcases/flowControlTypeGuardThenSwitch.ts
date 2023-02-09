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

// === tests/cases/compiler/flowControlTypeGuardThenSwitch.ts ===
declare function AssertType(value:any, type:string):void;
enum Kind {
    A,
    B,
}

interface Base {
    kind: Kind;
}

interface A extends Base {
    kind: Kind.A;
    yar: any;
}

interface B extends Base {
    kind: Kind.B;
    gar: any;
}

type Both = A | B;
function isBoth(x: Base): x is Both {
AssertType(true, "boolean");
    return true;
}

let foo: Base = undefined;
AssertType(foo, "Base");
AssertType(undefined, "undefined");

if (isBoth(foo)) {
    switch (foo.kind) {
AssertType(foo.kind, "Kind");

        case Kind.A:
AssertType(Kind.A, "Kind.A");

            const myA: A = foo; // Should not be an error
AssertType(myA, "A");
AssertType(foo, "A");

            break;
        case Kind.B:
AssertType(Kind.B, "Kind.B");

            const myB: B = foo;
AssertType(myB, "B");
AssertType(foo, "B");

            break;
    }
}


