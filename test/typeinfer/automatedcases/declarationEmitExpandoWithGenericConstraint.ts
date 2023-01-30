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

// === tests/cases/compiler/declarationEmitExpandoWithGenericConstraint.ts ===
declare function AssertType(value:any, type:string):void;
export interface Point {
    readonly x: number;
    readonly y: number;
}

export interface Rect<p extends Point> {
    readonly a: p;
    readonly b: p;
}

export const Point = (x: number, y: number): Point => ({ x, y });
AssertType(Point, "{ (number, number): Point; zero(): Point; }");
AssertType((x: number, y: number): Point => ({ x, y }), "{ (number, number): Point; zero(): Point; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType(({ x, y }), "{ x: number; y: number; }");
AssertType({ x, y }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(y, "number");

export const Rect = <p extends Point>(a: p, b: p): Rect<p> => ({ a, b });
AssertType(Rect, "<p extends Point>(p, p) => Rect<p>");
AssertType(<p extends Point>(a: p, b: p): Rect<p> => ({ a, b }), "<p extends Point>(p, p) => Rect<p>");
AssertType(a, "p");
AssertType(b, "p");
AssertType(({ a, b }), "{ a: p; b: p; }");
AssertType({ a, b }, "{ a: p; b: p; }");
AssertType(a, "p");
AssertType(b, "p");

Point.zero = (): Point => Point(0, 0);
AssertType(Point.zero = (): Point => Point(0, 0), "() => Point");
AssertType(Point.zero, "() => Point");
AssertType((): Point => Point(0, 0), "() => Point");
AssertType(Point(0, 0), "Point");
AssertType(Point, "{ (number, number): Point; zero(): Point; }");
AssertType(0, "int");
AssertType(0, "int");


