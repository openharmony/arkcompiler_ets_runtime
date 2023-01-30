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

// === tests/cases/compiler/constEnums.ts ===
declare function AssertType(value:any, type:string):void;
const enum Enum1 {
   A0 = 100,
}

const enum Enum1 {
    // correct cases
    A,
    B,
    C = 10,
    D = A | B,
    E = A | 1,
    F = 1 | A,
    G = (1 & 1),
    H = ~(A | B),
    I = A >>> 1,
    J = 1 & A,
    K = ~(1 | 5),
    L = ~D,
    M = E << B,
    N = E << 1,
    O = E >> B,
    P = E >> 1,
    PQ = E ** 2,
    Q = -D,
    R = C & 5,
    S = 5 & C,
    T = C | D,
    U = C | 1,
    V = 10 | D,
    W = Enum1.V,

    // correct cases: reference to the enum member from different enum declaration
    W1 = A0,
    W2 = Enum1.A0,
    W3 = Enum1["A0"],
    W4 = Enum1["W"],
    W5 = Enum1[`V`],
}

const enum Comments {
    "//",
    "/*",
    "*/",
    "///",
    "#",
    "<!--",
    "-->",
}

module A {
    export module B {
        export module C {
            export const enum E {
                V1 = 1,
                V2 = A.B.C.E.V1 | 100
            }
        }
    }
}

module A {
    export module B {
        export module C {
            export const enum E {
                V3 = A.B.C.E["V2"] & 200,
                V4 = A.B.C.E[`V1`] << 1,
            }
        }
    }
}

module A1 {
    export module B {
        export module C {
            export const enum E {
                V1 = 10,
                V2 = 110,
            }
        }
    }
}

module A2 {
    export module B {
        export module C {
            export const enum E {
                V1 = 10,
                V2 = 110,
            }
        }
        // module C will be classified as value
        export module C {
            let x = 1
        }
    }
}

import I = A.B.C.E;
import I1 = A1.B;
import I2 = A2.B;

function foo0(e: I): void {
    if (e === I.V1) {
AssertType(e === I.V1, "boolean");
AssertType(e, "I");
AssertType(I.V1, "I.V1");
    }
    else if (e === I.V2) {
AssertType(e === I.V2, "boolean");
AssertType(e, "union");
AssertType(I.V2, "I.V2");
    }
}

function foo1(e: I1.C.E): void {
    if (e === I1.C.E.V1) {
AssertType(e === I1.C.E.V1, "boolean");
AssertType(e, "I1.C.E");
AssertType(I1.C.E.V1, "I1.C.E.V1");
AssertType(I1.C.E, "typeof I1.C.E");
AssertType(I1.C, "typeof I1.C");
    }
    else if (e === I1.C.E.V2) {
AssertType(e === I1.C.E.V2, "boolean");
AssertType(e, "I1.C.E.V2");
AssertType(I1.C.E.V2, "I1.C.E.V2");
AssertType(I1.C.E, "typeof I1.C.E");
AssertType(I1.C, "typeof I1.C");
    }
}

function foo2(e: I2.C.E): void {
    if (e === I2.C.E.V1) {
AssertType(e === I2.C.E.V1, "boolean");
AssertType(e, "I2.C.E");
AssertType(I2.C.E.V1, "I2.C.E.V1");
AssertType(I2.C.E, "typeof I2.C.E");
AssertType(I2.C, "typeof I2.C");
    }
    else if (e === I2.C.E.V2) {
AssertType(e === I2.C.E.V2, "boolean");
AssertType(e, "I2.C.E.V2");
AssertType(I2.C.E.V2, "I2.C.E.V2");
AssertType(I2.C.E, "typeof I2.C.E");
AssertType(I2.C, "typeof I2.C");
    }
}


function foo(x: Enum1) {
    switch (x) {
AssertType(x, "Enum1");

        case Enum1.A:
AssertType(Enum1.A, "Enum1.A");

        case Enum1.B:
AssertType(Enum1.B, "Enum1.B");

        case Enum1.C:
AssertType(Enum1.C, "Enum1.C");

        case Enum1.D:
AssertType(Enum1.D, "Enum1.B");

        case Enum1.E:
AssertType(Enum1.E, "Enum1.B");

        case Enum1.F:
AssertType(Enum1.F, "Enum1.B");

        case Enum1.G:
AssertType(Enum1.G, "Enum1.B");

        case Enum1.H:
AssertType(Enum1.H, "Enum1.H");

        case Enum1.I:
AssertType(Enum1.I, "Enum1.A");

        case Enum1.J:
AssertType(Enum1.J, "Enum1.A");

        case Enum1.K:
AssertType(Enum1.K, "Enum1.K");

        case Enum1.L:
AssertType(Enum1.L, "Enum1.H");

        case Enum1.M:
AssertType(Enum1.M, "Enum1.M");

        case Enum1.N:
AssertType(Enum1.N, "Enum1.M");

        case Enum1.O:
AssertType(Enum1.O, "Enum1.A");

        case Enum1.P:
AssertType(Enum1.P, "Enum1.A");

        case Enum1.PQ:
AssertType(Enum1.PQ, "Enum1.B");

        case Enum1.Q:
AssertType(Enum1.Q, "Enum1.Q");

        case Enum1.R:
AssertType(Enum1.R, "Enum1.A");

        case Enum1.S:
AssertType(Enum1.S, "Enum1.A");

        case Enum1["T"]:
AssertType(Enum1["T"], "Enum1.T");
AssertType(Enum1, "typeof Enum1");
AssertType("T", "string");

        case Enum1[`U`]:
AssertType(Enum1[`U`], "Enum1.T");
AssertType(Enum1, "typeof Enum1");
AssertType(`U`, "string");

        case Enum1.V:
AssertType(Enum1.V, "Enum1.T");

        case Enum1.W:
AssertType(Enum1.W, "Enum1.T");

        case Enum1.W1:
AssertType(Enum1.W1, "Enum1.A0");

        case Enum1.W2:
AssertType(Enum1.W2, "Enum1.A0");

        case Enum1.W3:
AssertType(Enum1.W3, "Enum1.A0");

        case Enum1.W4:
AssertType(Enum1.W4, "Enum1.T");

            break;
    }
}

function bar(e: A.B.C.E): number {
    switch (e) {
AssertType(e, "I");

        case A.B.C.E.V1: 
AssertType(A.B.C.E.V1, "I.V1");
AssertType(A.B.C.E, "typeof I");
AssertType(A.B.C, "typeof A.B.C");
AssertType(A.B, "typeof A.B");
AssertType(1, "int");
return 1;

        case A.B.C.E.V2: 
AssertType(A.B.C.E.V2, "I.V2");
AssertType(A.B.C.E, "typeof I");
AssertType(A.B.C, "typeof A.B.C");
AssertType(A.B, "typeof A.B");
AssertType(1, "int");
return 1;

        case A.B.C.E.V3: 
AssertType(A.B.C.E.V3, "I.V3");
AssertType(A.B.C.E, "typeof I");
AssertType(A.B.C, "typeof A.B.C");
AssertType(A.B, "typeof A.B");
AssertType(1, "int");
return 1;
    }
}

function baz(c: Comments) {
    switch (c) {
AssertType(c, "Comments");

        case Comments["//"]:
AssertType(Comments["//"], "(typeof Comments)["//"]");
AssertType(Comments, "typeof Comments");
AssertType("//", "string");

        case Comments["/*"]:
AssertType(Comments["/*"], "(typeof Comments)["/*"]");
AssertType(Comments, "typeof Comments");
AssertType("/*", "string");

        case Comments["*/"]:
AssertType(Comments["*/"], "(typeof Comments)["*/"]");
AssertType(Comments, "typeof Comments");
AssertType("*/", "string");

        case Comments["///"]:
AssertType(Comments["///"], "(typeof Comments)["///"]");
AssertType(Comments, "typeof Comments");
AssertType("///", "string");

        case Comments["#"]:
AssertType(Comments["#"], "(typeof Comments)["#"]");
AssertType(Comments, "typeof Comments");
AssertType("#", "string");

        case Comments["<!--"]:
AssertType(Comments["<!--"], "(typeof Comments)["<!--"]");
AssertType(Comments, "typeof Comments");
AssertType("<!--", "string");

        case Comments["-->"]:
AssertType(Comments["-->"], "(typeof Comments)["-->"]");
AssertType(Comments, "typeof Comments");
AssertType("-->", "string");

        break;
    }
}


