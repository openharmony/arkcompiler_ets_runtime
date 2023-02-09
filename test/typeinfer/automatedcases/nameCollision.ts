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

// === tests/cases/conformance/internalModules/codeGeneration/nameCollision.ts ===
declare function AssertType(value:any, type:string):void;

module A {
    // these 2 statements force an underscore before the 'A' 
    // in the generated function call.
    let A = 12;
    let _A = '';
}

module B {
    let A = 12;
}

module B {
    // re-opened module with colliding name
    // this should add an underscore.
    class B {
        name: string;
    }
}

module X {
    let X = 13;
    export module Y {
        let Y = 13;
        export module Z {
            let X = 12;
            let Y = 12;
            let Z = 12;
        }
    }
}

module Y.Y {
    export enum Y {
        Red, Blue
    }
}

// no collision, since interface doesn't
// generate code.
module D {
    export interface D {
        id: number;
    }

    export let E = 'hello';
}

