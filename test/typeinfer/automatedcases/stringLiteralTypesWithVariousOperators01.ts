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

// === tests/cases/conformance/types/stringLiteral/stringLiteralTypesWithVariousOperators01.ts ===
declare function AssertType(value:any, type:string):void;
let abc: "ABC" = "ABC";
AssertType(abc, "string");
AssertType("ABC", "string");

let xyz: "XYZ" = "XYZ";
AssertType(xyz, "string");
AssertType("XYZ", "string");

let abcOrXyz: "ABC" | "XYZ" = abc || xyz;
AssertType(abcOrXyz, "union");
AssertType(abc || xyz, "union");
AssertType(abc, "string");
AssertType(xyz, "string");

let abcOrXyzOrNumber: "ABC" | "XYZ" | number = abcOrXyz || 100;
AssertType(abcOrXyzOrNumber, "union");
AssertType(abcOrXyz || 100, "union");
AssertType(abcOrXyz, "union");
AssertType(100, "int");

let a = "" + abc;
AssertType(a, "string");
AssertType("" + abc, "string");
AssertType("", "string");
AssertType(abc, "string");

let b = abc + "";
AssertType(b, "string");
AssertType(abc + "", "string");
AssertType(abc, "string");
AssertType("", "string");

let c = 10 + abc;
AssertType(c, "string");
AssertType(10 + abc, "string");
AssertType(10, "int");
AssertType(abc, "string");

let d = abc + 10;
AssertType(d, "string");
AssertType(abc + 10, "string");
AssertType(abc, "string");
AssertType(10, "int");

let e = xyz + abc;
AssertType(e, "string");
AssertType(xyz + abc, "string");
AssertType(xyz, "string");
AssertType(abc, "string");

let f = abc + xyz;
AssertType(f, "string");
AssertType(abc + xyz, "string");
AssertType(abc, "string");
AssertType(xyz, "string");

let g = true + abc;
AssertType(g, "string");
AssertType(true + abc, "string");
AssertType(true, "boolean");
AssertType(abc, "string");

let h = abc + true;
AssertType(h, "string");
AssertType(abc + true, "string");
AssertType(abc, "string");
AssertType(true, "boolean");

let i = abc + abcOrXyz + xyz;
AssertType(i, "string");
AssertType(abc + abcOrXyz + xyz, "string");
AssertType(abc + abcOrXyz, "string");
AssertType(abc, "string");
AssertType(abcOrXyz, "union");
AssertType(xyz, "string");

let j = abcOrXyz + abcOrXyz;
AssertType(j, "string");
AssertType(abcOrXyz + abcOrXyz, "string");
AssertType(abcOrXyz, "union");
AssertType(abcOrXyz, "union");

let k = +abcOrXyz;
AssertType(k, "number");
AssertType(+abcOrXyz, "number");
AssertType(abcOrXyz, "union");

let l = -abcOrXyz;
AssertType(l, "number");
AssertType(-abcOrXyz, "number");
AssertType(abcOrXyz, "union");

let m = abcOrXyzOrNumber + "";
AssertType(m, "string");
AssertType(abcOrXyzOrNumber + "", "string");
AssertType(abcOrXyzOrNumber, "union");
AssertType("", "string");

let n = "" + abcOrXyzOrNumber;
AssertType(n, "string");
AssertType("" + abcOrXyzOrNumber, "string");
AssertType("", "string");
AssertType(abcOrXyzOrNumber, "union");

let o = abcOrXyzOrNumber + abcOrXyz;
AssertType(o, "string");
AssertType(abcOrXyzOrNumber + abcOrXyz, "string");
AssertType(abcOrXyzOrNumber, "union");
AssertType(abcOrXyz, "union");

let p = abcOrXyz + abcOrXyzOrNumber;
AssertType(p, "string");
AssertType(abcOrXyz + abcOrXyzOrNumber, "string");
AssertType(abcOrXyz, "union");
AssertType(abcOrXyzOrNumber, "union");

let q = !abcOrXyzOrNumber;
AssertType(q, "boolean");
AssertType(!abcOrXyzOrNumber, "boolean");
AssertType(abcOrXyzOrNumber, "union");

let r = ~abcOrXyzOrNumber;
AssertType(r, "number");
AssertType(~abcOrXyzOrNumber, "number");
AssertType(abcOrXyzOrNumber, "union");

let s = abcOrXyzOrNumber < abcOrXyzOrNumber;
AssertType(s, "boolean");
AssertType(abcOrXyzOrNumber < abcOrXyzOrNumber, "boolean");
AssertType(abcOrXyzOrNumber, "union");
AssertType(abcOrXyzOrNumber, "union");

let t = abcOrXyzOrNumber >= abcOrXyz;
AssertType(t, "boolean");
AssertType(abcOrXyzOrNumber >= abcOrXyz, "boolean");
AssertType(abcOrXyzOrNumber, "union");
AssertType(abcOrXyz, "union");

let u = abc === abcOrXyz;
AssertType(u, "boolean");
AssertType(abc === abcOrXyz, "boolean");
AssertType(abc, "string");
AssertType(abcOrXyz, "union");

let v = abcOrXyz === abcOrXyzOrNumber;
AssertType(v, "boolean");
AssertType(abcOrXyz === abcOrXyzOrNumber, "boolean");
AssertType(abcOrXyz, "union");
AssertType(abcOrXyzOrNumber, "union");


