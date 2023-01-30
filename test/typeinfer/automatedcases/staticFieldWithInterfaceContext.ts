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

// === tests/cases/compiler/staticFieldWithInterfaceContext.ts ===
declare function AssertType(value:any, type:string):void;
interface I {
    x: { a: "a" };
}
let c: I = class {
AssertType(c, "I");
AssertType(class {    // should typecheck the same as the last line    static x = { a: "a" };}, "typeof c");

    // should typecheck the same as the last line
    static x = { a: "a" };
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

};
c.x = { a: "a" };
AssertType(c.x = { a: "a" }, "{ a: "a"; }");
AssertType(c.x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

const ex = "x";
AssertType(ex, "string");
AssertType("x", "string");

let c2: I = class { static [ex] = { a: "a" }; };
AssertType(c2, "I");
AssertType(class { static [ex] = { a: "a" }; }, "typeof c2");
AssertType([ex], "{ a: "a"; }");
AssertType(ex, "string");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

c[ex] = { a: "a" };
AssertType(c[ex] = { a: "a" }, "{ a: "a"; }");
AssertType(c[ex], "{ a: "a"; }");
AssertType(c, "I");
AssertType(ex, "string");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

function f(c: I = class { static x = { a: "a" } }) { }

let { c: c3 }: { c: I } = { c: class { static x = { a: "a" } } };
AssertType(c, "any");
AssertType(c3, "I");
AssertType(c, "I");
AssertType({ c: class { static x = { a: "a" } } }, "{ c: typeof c; }");
AssertType(c, "typeof c");
AssertType(class { static x = { a: "a" } }, "typeof c");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

let { c: c4 = class { static x = { a: "a" } }}: { c?: I } = { };
AssertType(c, "any");
AssertType(c4, "I");
AssertType(class { static x = { a: "a" } }, "typeof c4");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");
AssertType(c, "I");
AssertType({ }, "{}");

let { c: c5 = class { static x = { a: "a" } }}: { c?: I } = { c: class { static x = { a: "a" } } };
AssertType(c, "any");
AssertType(c5, "I");
AssertType(class { static x = { a: "a" } }, "typeof c5");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");
AssertType(c, "I");
AssertType({ c: class { static x = { a: "a" } } }, "{ c: typeof c; }");
AssertType(c, "typeof c");
AssertType(class { static x = { a: "a" } }, "typeof c");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

let [ c6 ]: [I] = [class { static x = { a: "a" } }];
AssertType(c6, "I");
AssertType([class { static x = { a: "a" } }], "[typeof (Anonymous class)]");
AssertType(class { static x = { a: "a" } }, "typeof (Anonymous class)");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

let [ c7 ]: I[] = [class { static x = { a: "a" } }];
AssertType(c7, "I");
AssertType([class { static x = { a: "a" } }], "(typeof (Anonymous class))[]");
AssertType(class { static x = { a: "a" } }, "typeof (Anonymous class)");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

let [ c8 = class { static x = { a: "a" } } ]: [I?] = [];
AssertType(c8, "I");
AssertType(class { static x = { a: "a" } }, "typeof c8");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");
AssertType([], "[]");

let [ c9 = class { static x = { a: "a" } } ]: I[] = [];
AssertType(c9, "I");
AssertType(class { static x = { a: "a" } }, "typeof c9");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");
AssertType([], "undefined[]");

let [ c10 = class { static x = { a: "a" } } ]: [I?] = [class { static x = { a: "a" } }];
AssertType(c10, "I");
AssertType(class { static x = { a: "a" } }, "typeof c10");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");
AssertType([class { static x = { a: "a" } }], "[typeof (Anonymous class)]");
AssertType(class { static x = { a: "a" } }, "typeof (Anonymous class)");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");

let [ c11 = class { static x = { a: "a" } } ]: I[] = [class { static x = { a: "a" } }];
AssertType(c11, "I");
AssertType(class { static x = { a: "a" } }, "typeof c11");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");
AssertType([class { static x = { a: "a" } }], "(typeof (Anonymous class))[]");
AssertType(class { static x = { a: "a" } }, "typeof (Anonymous class)");
AssertType(x, "{ a: "a"; }");
AssertType({ a: "a" }, "{ a: "a"; }");
AssertType(a, "string");
AssertType("a", "string");


