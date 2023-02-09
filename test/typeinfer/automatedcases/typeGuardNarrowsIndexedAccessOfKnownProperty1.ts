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

// === tests/cases/compiler/typeGuardNarrowsIndexedAccessOfKnownProperty1.ts ===
declare function AssertType(value:any, type:string):void;
interface Square {
    ["dash-ok"]: "square";
    ["square-size"]: number;
}
 interface Rectangle {
    ["dash-ok"]: "rectangle";
    width: number;
    height: number;
}
 interface Circle {
    ["dash-ok"]: "circle";
    radius: number;
}
 type Shape = Square | Rectangle | Circle;
interface Subshape {
    "0": {
        sub: {
            under: {
                shape: Shape;
            }
        }
    }
}
function area(s: Shape): number {
    switch(s['dash-ok']) {
AssertType(s['dash-ok'], "union");
AssertType(s, "Shape");
AssertType('dash-ok', "string");

        case "square": 
AssertType("square", "string");
AssertType(s['square-size'] * s['square-size'], "number");
AssertType(s['square-size'], "number");
AssertType(s, "Square");
AssertType('square-size', "string");
AssertType(s['square-size'], "number");
AssertType(s, "Square");
AssertType('square-size', "string");
return s['square-size'] * s['square-size'];

        case "rectangle": 
AssertType("rectangle", "string");
AssertType(s.width * s['height'], "number");
AssertType(s.width, "number");
AssertType(s['height'], "number");
AssertType(s, "Rectangle");
AssertType('height', "string");
return s.width * s['height'];

        case "circle": 
AssertType("circle", "string");
AssertType(Math.PI * s['radius'] * s.radius, "number");
AssertType(Math.PI * s['radius'], "number");
AssertType(Math.PI, "number");
AssertType(s['radius'], "number");
AssertType(s, "Circle");
AssertType('radius', "string");
AssertType(s.radius, "number");
return Math.PI * s['radius'] * s.radius;
    }
}
function subarea(s: Subshape): number {
    switch(s[0]["sub"].under["shape"]["dash-ok"]) {
AssertType(s[0]["sub"].under["shape"]["dash-ok"], "union");
AssertType(s[0]["sub"].under["shape"], "Shape");
AssertType(s[0]["sub"].under, "{ shape: Shape; }");
AssertType(s[0]["sub"], "{ under: { shape: Shape; }; }");
AssertType(s[0], "{ sub: { under: { shape: Shape; }; }; }");
AssertType(s, "Subshape");
AssertType(0, "int");
AssertType("sub", "string");
AssertType("shape", "string");
AssertType("dash-ok", "string");

        case "square": 
AssertType("square", "string");
AssertType(s[0].sub.under.shape["square-size"] * s[0].sub.under.shape["square-size"], "number");
AssertType(s[0].sub.under.shape["square-size"], "number");
AssertType(s[0].sub.under.shape, "Square");
AssertType(s[0].sub.under, "{ shape: Shape; }");
AssertType(s[0].sub, "{ under: { shape: Shape; }; }");
AssertType(s[0], "{ sub: { under: { shape: Shape; }; }; }");
AssertType(s, "Subshape");
AssertType(0, "int");
AssertType("square-size", "string");
AssertType(s[0].sub.under.shape["square-size"], "number");
AssertType(s[0].sub.under.shape, "Square");
AssertType(s[0].sub.under, "{ shape: Shape; }");
AssertType(s[0].sub, "{ under: { shape: Shape; }; }");
AssertType(s[0], "{ sub: { under: { shape: Shape; }; }; }");
AssertType(s, "Subshape");
AssertType(0, "int");
AssertType("square-size", "string");
return s[0].sub.under.shape["square-size"] * s[0].sub.under.shape["square-size"];

        case "rectangle": 
AssertType("rectangle", "string");
AssertType(s[0]["sub"]["under"]["shape"]["width"] * s[0]["sub"]["under"]["shape"].height, "number");
AssertType(s[0]["sub"]["under"]["shape"]["width"], "number");
AssertType(s[0]["sub"]["under"]["shape"], "Rectangle");
AssertType(s[0]["sub"]["under"], "{ shape: Shape; }");
AssertType(s[0]["sub"], "{ under: { shape: Shape; }; }");
AssertType(s[0], "{ sub: { under: { shape: Shape; }; }; }");
AssertType(s, "Subshape");
AssertType(0, "int");
AssertType("sub", "string");
AssertType("under", "string");
AssertType("shape", "string");
AssertType("width", "string");
AssertType(s[0]["sub"]["under"]["shape"].height, "number");
AssertType(s[0]["sub"]["under"]["shape"], "Rectangle");
AssertType(s[0]["sub"]["under"], "{ shape: Shape; }");
AssertType(s[0]["sub"], "{ under: { shape: Shape; }; }");
AssertType(s[0], "{ sub: { under: { shape: Shape; }; }; }");
AssertType(s, "Subshape");
AssertType(0, "int");
AssertType("sub", "string");
AssertType("under", "string");
AssertType("shape", "string");
return s[0]["sub"]["under"]["shape"]["width"] * s[0]["sub"]["under"]["shape"].height;

        case "circle": 
AssertType("circle", "string");
AssertType(Math.PI * s[0].sub.under["shape"].radius * s[0]["sub"].under.shape["radius"], "number");
AssertType(Math.PI * s[0].sub.under["shape"].radius, "number");
AssertType(Math.PI, "number");
AssertType(s[0].sub.under["shape"].radius, "number");
AssertType(s[0].sub.under["shape"], "Circle");
AssertType(s[0].sub.under, "{ shape: Shape; }");
AssertType(s[0].sub, "{ under: { shape: Shape; }; }");
AssertType(s[0], "{ sub: { under: { shape: Shape; }; }; }");
AssertType(s, "Subshape");
AssertType(0, "int");
AssertType("shape", "string");
AssertType(s[0]["sub"].under.shape["radius"], "number");
AssertType(s[0]["sub"].under.shape, "Circle");
AssertType(s[0]["sub"].under, "{ shape: Shape; }");
AssertType(s[0]["sub"], "{ under: { shape: Shape; }; }");
AssertType(s[0], "{ sub: { under: { shape: Shape; }; }; }");
AssertType(s, "Subshape");
AssertType(0, "int");
AssertType("sub", "string");
AssertType("radius", "string");
return Math.PI * s[0].sub.under["shape"].radius * s[0]["sub"].under.shape["radius"];
    }
}

interface X {
    0: "xx",
    1: number
}

interface Y {
    0: "yy",
    1: string
}

type A = ["aa", number];
type B = ["bb", string];

type Z = X | Y;

type C = A | B;

function check(z: Z, c: C) {
    z[0] // fine, typescript sees "xx" | "yy"
AssertType(z[0], "union");
AssertType(z, "Z");
AssertType(0, "int");

    switch (z[0]) {
AssertType(z[0], "union");
AssertType(z, "Z");
AssertType(0, "int");

        case "xx":
AssertType("xx", "string");

            let xx: number = z[1] // should be number
AssertType(xx, "number");
AssertType(z[1], "number");
AssertType(z, "X");
AssertType(1, "int");

            break;
        case "yy":
AssertType("yy", "string");

            let yy: string = z[1] // should be string
AssertType(yy, "string");
AssertType(z[1], "string");
AssertType(z, "Y");
AssertType(1, "int");

            break;
    }
    c[0] // fine, typescript sees "xx" | "yy"
AssertType(c[0], "union");
AssertType(c, "C");
AssertType(0, "int");

    switch (c[0]) {
AssertType(c[0], "union");
AssertType(c, "C");
AssertType(0, "int");

        case "aa":
AssertType("aa", "string");

            let aa: number = c[1] // should be number
AssertType(aa, "number");
AssertType(c[1], "number");
AssertType(c, "A");
AssertType(1, "int");

            break;
        case "bb":
AssertType("bb", "string");

            let bb: string = c[1] // should be string
AssertType(bb, "string");
AssertType(c[1], "string");
AssertType(c, "B");
AssertType(1, "int");

            break;
    }
}

export function g(pair: [number, string?]): string {
AssertType(pair[1] ? pair[1] : 'nope', "string");
AssertType(pair[1], "union");
AssertType(pair, "[number, (union)?]");
AssertType(1, "int");
AssertType(pair[1], "string");
AssertType(pair, "[number, (union)?]");
AssertType(1, "int");
AssertType('nope', "string");
    return pair[1] ? pair[1] : 'nope';
}


