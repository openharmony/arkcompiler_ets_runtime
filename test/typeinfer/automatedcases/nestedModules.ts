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

// === tests/cases/conformance/internalModules/moduleDeclarations/nestedModules.ts ===
declare function AssertType(value:any, type:string):void;
module A.B.C {
    export interface Point {
        x: number;
        y: number;
    }
}

module A {
    export module B {
        let Point: C.Point = { x: 0, y: 0 }; // bug 832088: could not find module 'C'
    }
}

module M2.X {
    export interface Point {
        x: number; y: number;
    }
}

module M2 {
    export module X {
        export let Point: number;
    }
}

let m = M2.X;
AssertType(m, "typeof M2.X");
AssertType(M2.X, "typeof M2.X");

let point: number;
AssertType(point, "number");

let point = m.Point;
AssertType(point, "number");
AssertType(m.Point, "number");

let p: { x: number; y: number; 
AssertType(p, "{ x: number; y: number; }");

AssertType(x, "number");

AssertType(y, "number");
}

let p: M2.X.Point;
AssertType(p, "{ x: number; y: number; }");
AssertType(M2, "any");
AssertType(X, "any");


