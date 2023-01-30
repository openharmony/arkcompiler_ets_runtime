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

// === tests/cases/conformance/parser/ecmascript5/ErrorRecovery/IncompleteMemberVariables/parserErrorRecovery_IncompleteMemberVariable1.ts ===
declare function AssertType(value:any, type:string):void;
// Interface
interface IPoint {
    getDist(): number;
}

// Module
module Shapes {

    // Class
    export class Point implements IPoint {

        public con: "hello";
        // Constructor
        constructor (public x: number, public y: number) { }

        // Instance member
        getDist() { 
AssertType(Math.sqrt(this.x * this.x + this.y * this.y), "number");
return Math.sqrt(this.x * this.x + this.y * this.y); 

AssertType(Math.sqrt, "(number) => number");

AssertType(this.x * this.x + this.y * this.y, "number");

AssertType(this.x * this.x, "number");

AssertType(this.x, "number");

AssertType(this, "this");

AssertType(this.x, "number");

AssertType(this, "this");

AssertType(this.y * this.y, "number");

AssertType(this.y, "number");

AssertType(this, "this");

AssertType(this.y, "number");

AssertType(this, "this");
}

        // Static member
        static origin = new Point(0, 0);
    }

}

// Local letiables
let p: IPoint = new Shapes.Point(3, 4);
AssertType(p, "IPoint");
AssertType(new Shapes.Point(3, 4), "Shapes.Point");
AssertType(Shapes.Point, "typeof Shapes.Point");
AssertType(3, "int");
AssertType(4, "int");

let dist = p.getDist();
AssertType(dist, "number");
AssertType(p.getDist(), "number");
AssertType(p.getDist, "() => number");


