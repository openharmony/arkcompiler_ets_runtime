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

// === tests/cases/compiler/mergedDeclarations1.ts ===
declare function AssertType(value:any, type:string):void;
interface Point {
    x: number;
    y: number;
}
function point(x: number, y: number): Point {
AssertType({ x: x, y: y }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(x, "number");
AssertType(y, "number");
AssertType(y, "number");
    return { x: x, y: y };
}
module point {
    export let origin = point(0, 0);
    export function equals(p1: Point, p2: Point) {
AssertType(p1.x == p2.x && p1.y == p2.y, "boolean");
AssertType(p1.x == p2.x, "boolean");
AssertType(p1.x, "number");
AssertType(p2.x, "number");
AssertType(p1.y == p2.y, "boolean");
AssertType(p1.y, "number");
AssertType(p2.y, "number");
        return p1.x == p2.x && p1.y == p2.y;
    }
}
let p1 = point(0, 0);
AssertType(p1, "Point");
AssertType(point(0, 0), "Point");
AssertType(point, "typeof point");
AssertType(0, "int");
AssertType(0, "int");

let p2 = point.origin;
AssertType(p2, "Point");
AssertType(point.origin, "Point");

let b = point.equals(p1, p2);
AssertType(b, "boolean");
AssertType(point.equals(p1, p2), "boolean");
AssertType(point.equals, "(Point, Point) => boolean");
AssertType(p1, "Point");
AssertType(p2, "Point");


