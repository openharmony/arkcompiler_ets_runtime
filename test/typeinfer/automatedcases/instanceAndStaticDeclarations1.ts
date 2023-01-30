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

// === tests/cases/compiler/instanceAndStaticDeclarations1.ts ===
declare function AssertType(value:any, type:string):void;
// from spec

class Point {
    constructor(public x: number, public y: number) { }
    public distance(p: Point) {
        let dx = this.x - p.x;
AssertType(dx, "number");
AssertType(this.x - p.x, "number");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(p.x, "number");

        let dy = this.y - p.y;
AssertType(dy, "number");
AssertType(this.y - p.y, "number");
AssertType(this.y, "number");
AssertType(this, "this");
AssertType(p.y, "number");

AssertType(Math.sqrt(dx * dx + dy * dy), "number");
AssertType(Math.sqrt, "(number) => number");
AssertType(dx * dx + dy * dy, "number");
AssertType(dx * dx, "number");
AssertType(dx, "number");
AssertType(dx, "number");
AssertType(dy * dy, "number");
AssertType(dy, "number");
AssertType(dy, "number");
        return Math.sqrt(dx * dx + dy * dy);
    }
    static origin = new Point(0, 0);
    static distance(p1: Point, p2: Point) { 
AssertType(p1.distance(p2), "number");
return p1.distance(p2); 

AssertType(p1.distance, "(Point) => number");

AssertType(p2, "Point");
}
}

