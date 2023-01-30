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

// === tests/cases/compiler/exportImportAndClodule.ts ===
declare function AssertType(value:any, type:string):void;
module K {
    export class L {
        constructor(public name: string) { }
    }
    export module L {
        export let y = 12;
        export interface Point {
            x: number;
            y: number;
        }
    }
}
module M {
    export import D = K.L;
}
let o: { name: string };
AssertType(o, "{ name: string; }");
AssertType(name, "string");

let o = new M.D('Hello');
AssertType(o, "{ name: string; }");
AssertType(new M.D('Hello'), "K.L");
AssertType(M.D, "typeof K.L");
AssertType('Hello', "string");

let p: { x: number; y: number; 
AssertType(p, "{ x: number; y: number; }");

AssertType(x, "number");

AssertType(y, "number");
}

let p: M.D.Point;
AssertType(p, "{ x: number; y: number; }");
AssertType(M, "any");
AssertType(D, "any");


