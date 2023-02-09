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

// === tests/cases/conformance/internalModules/DeclarationMerging/TwoInternalModulesThatMergeEachWithExportedModulesOfTheSameName.ts ===
declare function AssertType(value:any, type:string):void;
module A.B {
    export let x: number;
}

module A{ 
    module B {
        export let x: string;
    }
}

// ensure the right let decl is exported
let x: number;
AssertType(x, "number");

let x = A.B.x;
AssertType(x, "number");
AssertType(A.B.x, "number");

module X.Y.Z {
    export class Line {
        length: number;
    }
}

module X {
    export module Y {
        module Z {
            export class Line {
                name: string;
            }
        }
    }
}

// make sure merging works as expected
let l: { length: number };
AssertType(l, "{ length: number; }");
AssertType(length, "number");

let l: X.Y.Z.Line;
AssertType(l, "{ length: number; }");
AssertType(X, "any");
AssertType(Y, "any");
AssertType(Z, "any");


