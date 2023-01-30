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

// === tests/cases/compiler/overrideBaseIntersectionMethod.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #14615

type Constructor<T> = new (...args: any[]) => T;

const WithLocation = <T extends Constructor<Point>>(Base: T) => class extends Base {
AssertType(WithLocation, "<T extends Constructor<Point>>(T) => { new (...any[]): (Anonymous class); prototype: WithLocation<any>.(Anonymous class); } & T");
AssertType(Base, "T");
AssertType(class extends Base {  getLocation(): [number, number] {    const [x,y] = super.getLocation();    return [this.x | x, this.y | y];  }}, "{ new (...any[]): (Anonymous class); prototype: WithLocation<any>.(Anonymous class); } & T");
AssertType(<T extends Constructor<Point>>(Base: T) => class extends Base {  getLocation(): [number, number] {    const [x,y] = super.getLocation();    return [this.x | x, this.y | y];  }}, "<T extends Constructor<Point>>(T) => { new (...any[]): (Anonymous class); prototype: WithLocation<any>.(Anonymous class); } & T");

  getLocation(): [number, number] {
AssertType(getLocation, "() => [number, number]");

    const [x,y] = super.getLocation();
AssertType(x, "number");
AssertType(y, "number");
AssertType(super.getLocation(), "[number, number]");
AssertType(super.getLocation, "() => [number, number]");
AssertType(super, "Point");

AssertType([this.x | x, this.y | y], "[number, number]");
AssertType(this.x | x, "number");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(x, "number");
AssertType(this.y | y, "number");
AssertType(this.y, "number");
AssertType(this, "this");
AssertType(y, "number");
    return [this.x | x, this.y | y];
  }
}

class Point {
  constructor(public x: number, public y: number) { }
  getLocation(): [number, number] {
AssertType([0,0], "[number, number]");
AssertType(0, "int");
AssertType(0, "int");
    return [0,0];
  }
}

class Foo extends WithLocation(Point) {
  calculate() {
AssertType(this.x + this.y, "number");
AssertType(this.x, "number");
AssertType(this, "this");
AssertType(this.y, "number");
AssertType(this, "this");
    return this.x + this.y;
  }
  getLocation() {
AssertType(super.getLocation(), "[number, number]");
AssertType(super.getLocation, "(() => [number, number]) & (() => [number, number])");
AssertType(super, "WithLocation<typeof Point>.(Anonymous class) & Point");
    return super.getLocation()
  }
  whereAmI() {
AssertType(this.getLocation(), "[number, number]");
AssertType(this.getLocation, "() => [number, number]");
AssertType(this, "this");
    return this.getLocation();
  }
}


