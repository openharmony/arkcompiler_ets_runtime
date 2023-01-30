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

// === tests/cases/compiler/topLevel.ts ===
declare function AssertType(value:any, type:string):void;
interface IPoint {
    x:number;
    y:number;
}

class Point implements IPoint {
    constructor(public x,public y){}
    public move(xo:number,yo:number) {
	this.x+=xo;
AssertType(this.x+=xo, "any");
AssertType(this.x, "any");
AssertType(this, "this");
AssertType(xo, "number");

	this.y+=yo;
AssertType(this.y+=yo, "any");
AssertType(this.y, "any");
AssertType(this, "this");
AssertType(yo, "number");

AssertType(this, "this");
	return this;
    }
    public toString() {
AssertType(("("+this.x+","+this.y+")"), "string");
AssertType("("+this.x+","+this.y+")", "string");
AssertType("("+this.x+","+this.y, "string");
AssertType("("+this.x+",", "string");
AssertType("("+this.x, "string");
AssertType("(", "string");
AssertType(this.x, "any");
AssertType(this, "this");
AssertType(",", "string");
AssertType(this.y, "any");
AssertType(this, "this");
AssertType(")", "string");
	return ("("+this.x+","+this.y+")");
    }
}

let result="";
AssertType(result, "string");
AssertType("", "string");

result+=(new Point(3,4).move(2,2));
AssertType(result+=(new Point(3,4).move(2,2)), "string");
AssertType(result, "string");
AssertType((new Point(3,4).move(2,2)), "Point");
AssertType(new Point(3,4).move(2,2), "Point");
AssertType(new Point(3,4).move, "(number, number) => Point");
AssertType(2, "int");
AssertType(2, "int");

module M {
    export let origin=new Point(0,0);
}

result+=(M.origin.move(1,1));
AssertType(result+=(M.origin.move(1,1)), "string");
AssertType(result, "string");
AssertType((M.origin.move(1,1)), "Point");
AssertType(M.origin.move(1,1), "Point");
AssertType(M.origin.move, "(number, number) => Point");
AssertType(1, "int");
AssertType(1, "int");



