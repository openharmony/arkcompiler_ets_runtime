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

// === tests/cases/compiler/castTest.ts ===
declare function AssertType(value:any, type:string):void;
let x : any = 0;
AssertType(x, "any");
AssertType(0, "int");

let z = <number> x;
AssertType(z, "number");
AssertType(<number> x, "number");
AssertType(x, "any");

let y = x + z;
AssertType(y, "any");
AssertType(x + z, "any");
AssertType(x, "any");
AssertType(z, "number");

let a = <any>0;
AssertType(a, "any");
AssertType(<any>0, "any");
AssertType(0, "int");

let b = <boolean>true;
AssertType(b, "boolean");
AssertType(<boolean>true, "boolean");
AssertType(true, "boolean");

let s = <string>"";
AssertType(s, "string");
AssertType(<string>"", "string");
AssertType("", "string");

let ar = <any[]>null;
AssertType(ar, "any[]");
AssertType(<any[]>null, "any[]");
AssertType(null, "null");

let f = <(res : number) => void>null;
AssertType(f, "(number) => void");
AssertType(<(res : number) => void>null, "(number) => void");
AssertType(res, "number");
AssertType(null, "null");

declare class Point
{
    x: number;
    y: number;
    add(dx: number, dy: number): Point;
    mult(p: Point): Point;
    constructor(x: number, y: number);
}

let p_cast = <Point> ({
AssertType(p_cast, "Point");
AssertType(({    x: 0,    y: 0,    add: function(dx, dy) {        return new Point(this.x + dx, this.y + dy);    },    mult: function(p) { return p; }}), "{ x: number; y: number; add: (number, number) => Point; mult: (Point) => Point; }");
AssertType({    x: 0,    y: 0,    add: function(dx, dy) {        return new Point(this.x + dx, this.y + dy);    },    mult: function(p) { return p; }}, "{ x: number; y: number; add: (number, number) => Point; mult: (Point) => Point; }");
AssertType(<Point> ({    x: 0,    y: 0,    add: function(dx, dy) {        return new Point(this.x + dx, this.y + dy);    },    mult: function(p) { return p; }}), "Point");

    x: 0,
AssertType(x, "number");
AssertType(0, "int");

    y: 0,
AssertType(y, "number");
AssertType(0, "int");

    add: function(dx, dy) {
AssertType(add, "(number, number) => Point");
AssertType(dx, "number");
AssertType(dy, "number");
AssertType(function(dx, dy) {        return new Point(this.x + dx, this.y + dy);    }, "(number, number) => Point");

AssertType(new Point(this.x + dx, this.y + dy), "Point");
AssertType(Point, "typeof Point");
AssertType(this.x + dx, "any");
AssertType(this.x, "any");
AssertType(this, "any");
AssertType(dx, "number");
AssertType(this.y + dy, "any");
AssertType(this.y, "any");
AssertType(this, "any");
AssertType(dy, "number");
        return new Point(this.x + dx, this.y + dy);

    },
    mult: function(p) { 
AssertType(mult, "(Point) => Point");
return p; 

AssertType(function(p) { return p; }, "(Point) => Point");

AssertType(p, "Point");

AssertType(p, "Point");
}

})



