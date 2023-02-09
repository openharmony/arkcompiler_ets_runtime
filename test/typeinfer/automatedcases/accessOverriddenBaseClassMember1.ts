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

// === tests/cases/compiler/accessOverriddenBaseClassMember1.ts ===
declare function AssertType(value:any, type:string):void;
class Point {
    constructor(public x: number, public y: number) { }
    public toString() {
AssertType("x=" + this.x + " y=" + this.y, "string");
AssertType("x=" + this.x + " y=", "string");
AssertType("x=" + this.x, "string");
AssertType("x=", "string");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(" y=", "string");
AssertType(this.y, "number");
AssertType(this, "this");
        return "x=" + this.x + " y=" + this.y;
    }
}
class ColoredPoint extends Point {
    constructor(x: number, y: number, public color: string) {
        super(x, y);
AssertType(super(x, y), "void");
AssertType(super, "typeof Point");
AssertType(x, "number");
AssertType(y, "number");
    }
    public toString() {
AssertType(super.toString() + " color=" + this.color, "string");
AssertType(super.toString() + " color=", "string");
AssertType(super.toString(), "string");
AssertType(super.toString, "() => string");
AssertType(super, "Point");
AssertType(" color=", "string");
AssertType(this.color, "string");
AssertType(this, "this");
        return super.toString() + " color=" + this.color;
    }
}


