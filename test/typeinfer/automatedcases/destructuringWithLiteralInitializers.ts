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

// === tests/cases/conformance/es6/destructuring/destructuringWithLiteralInitializers.ts ===
declare function AssertType(value:any, type:string):void;
// (arg: { x: any, y: any }) => void
function f1({ x, y }) { }
f1({ x: 1, y: 1 });
AssertType(f1({ x: 1, y: 1 }), "void");
AssertType(f1, "({ x: any; y: any; }) => void");
AssertType({ x: 1, y: 1 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(1, "int");

// (arg: { x: any, y?: number }) => void
function f2({ x, y = 0 }) { }
f2({ x: 1 });
AssertType(f2({ x: 1 }), "void");
AssertType(f2, "({ x: any; y?: number; }) => void");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

f2({ x: 1, y: 1 });
AssertType(f2({ x: 1, y: 1 }), "void");
AssertType(f2, "({ x: any; y?: number; }) => void");
AssertType({ x: 1, y: 1 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(1, "int");

// (arg: { x?: number, y?: number }) => void
function f3({ x = 0, y = 0 }) { }
f3({});
AssertType(f3({}), "void");
AssertType(f3, "({ x?: number; y?: number; }) => void");
AssertType({}, "{}");

f3({ x: 1 });
AssertType(f3({ x: 1 }), "void");
AssertType(f3, "({ x?: number; y?: number; }) => void");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

f3({ y: 1 });
AssertType(f3({ y: 1 }), "void");
AssertType(f3, "({ x?: number; y?: number; }) => void");
AssertType({ y: 1 }, "{ y: number; }");
AssertType(y, "number");
AssertType(1, "int");

f3({ x: 1, y: 1 });
AssertType(f3({ x: 1, y: 1 }), "void");
AssertType(f3, "({ x?: number; y?: number; }) => void");
AssertType({ x: 1, y: 1 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(1, "int");

// (arg?: { x: number, y: number }) => void
function f4({ x, y } = { x: 0, y: 0 }) { }
f4();
AssertType(f4(), "void");
AssertType(f4, "(?{ x: number; y: number; }) => void");

f4({ x: 1, y: 1 });
AssertType(f4({ x: 1, y: 1 }), "void");
AssertType(f4, "(?{ x: number; y: number; }) => void");
AssertType({ x: 1, y: 1 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(1, "int");

// (arg?: { x: number, y?: number }) => void
function f5({ x, y = 0 } = { x: 0 }) { }
f5();
AssertType(f5(), "void");
AssertType(f5, "(?{ x: number; y?: number; }) => void");

f5({ x: 1 });
AssertType(f5({ x: 1 }), "void");
AssertType(f5, "(?{ x: number; y?: number; }) => void");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

f5({ x: 1, y: 1 });
AssertType(f5({ x: 1, y: 1 }), "void");
AssertType(f5, "(?{ x: number; y?: number; }) => void");
AssertType({ x: 1, y: 1 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(1, "int");

// (arg?: { x?: number, y?: number }) => void
function f6({ x = 0, y = 0 } = {}) { }
f6();
AssertType(f6(), "void");
AssertType(f6, "(?{ x?: number; y?: number; }) => void");

f6({});
AssertType(f6({}), "void");
AssertType(f6, "(?{ x?: number; y?: number; }) => void");
AssertType({}, "{}");

f6({ x: 1 });
AssertType(f6({ x: 1 }), "void");
AssertType(f6, "(?{ x?: number; y?: number; }) => void");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

f6({ y: 1 });
AssertType(f6({ y: 1 }), "void");
AssertType(f6, "(?{ x?: number; y?: number; }) => void");
AssertType({ y: 1 }, "{ y: number; }");
AssertType(y, "number");
AssertType(1, "int");

f6({ x: 1, y: 1 });
AssertType(f6({ x: 1, y: 1 }), "void");
AssertType(f6, "(?{ x?: number; y?: number; }) => void");
AssertType({ x: 1, y: 1 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(1, "int");

// (arg?: { a: { x?: number, y?: number } }) => void
function f7({ a: { x = 0, y = 0 } } = { a: {} }) { }
f7();
AssertType(f7(), "void");
AssertType(f7, "(?{ a: { x?: number; y?: number; }; }) => void");

f7({ a: {} });
AssertType(f7({ a: {} }), "void");
AssertType(f7, "(?{ a: { x?: number; y?: number; }; }) => void");
AssertType({ a: {} }, "{ a: {}; }");
AssertType(a, "{}");
AssertType({}, "{}");

f7({ a: { x: 1 } });
AssertType(f7({ a: { x: 1 } }), "void");
AssertType(f7, "(?{ a: { x?: number; y?: number; }; }) => void");
AssertType({ a: { x: 1 } }, "{ a: { x: number; }; }");
AssertType(a, "{ x: number; }");
AssertType({ x: 1 }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");

f7({ a: { y: 1 } });
AssertType(f7({ a: { y: 1 } }), "void");
AssertType(f7, "(?{ a: { x?: number; y?: number; }; }) => void");
AssertType({ a: { y: 1 } }, "{ a: { y: number; }; }");
AssertType(a, "{ y: number; }");
AssertType({ y: 1 }, "{ y: number; }");
AssertType(y, "number");
AssertType(1, "int");

f7({ a: { x: 1, y: 1 } });
AssertType(f7({ a: { x: 1, y: 1 } }), "void");
AssertType(f7, "(?{ a: { x?: number; y?: number; }; }) => void");
AssertType({ a: { x: 1, y: 1 } }, "{ a: { x: number; y: number; }; }");
AssertType(a, "{ x: number; y: number; }");
AssertType({ x: 1, y: 1 }, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(y, "number");
AssertType(1, "int");

// (arg: [any, any]) => void
function g1([x, y]) { }
g1([1, 1]);
AssertType(g1([1, 1]), "void");
AssertType(g1, "([any, any]) => void");
AssertType([1, 1], "[number, number]");
AssertType(1, "int");
AssertType(1, "int");

// (arg: [number, number]) => void
function g2([x = 0, y = 0]) { }
g2([1, 1]);
AssertType(g2([1, 1]), "void");
AssertType(g2, "([number?, number?]) => void");
AssertType([1, 1], "[number, number]");
AssertType(1, "int");
AssertType(1, "int");

// (arg?: [number, number]) => void
function g3([x, y] = [0, 0]) { }
g3();
AssertType(g3(), "void");
AssertType(g3, "(?[number, number]) => void");

g3([1, 1]);
AssertType(g3([1, 1]), "void");
AssertType(g3, "(?[number, number]) => void");
AssertType([1, 1], "[number, number]");
AssertType(1, "int");
AssertType(1, "int");

// (arg?: [number, number]) => void
function g4([x, y = 0] = [0]) { }
g4();
AssertType(g4(), "void");
AssertType(g4, "(?[number, number?]) => void");

g4([1, 1]);
AssertType(g4([1, 1]), "void");
AssertType(g4, "(?[number, number?]) => void");
AssertType([1, 1], "[number, number]");
AssertType(1, "int");
AssertType(1, "int");

// (arg?: [number, number]) => void
function g5([x = 0, y = 0] = []) { }
g5();
AssertType(g5(), "void");
AssertType(g5, "(?[number?, number?]) => void");

g5([1, 1]);
AssertType(g5([1, 1]), "void");
AssertType(g5, "(?[number?, number?]) => void");
AssertType([1, 1], "[number, number]");
AssertType(1, "int");
AssertType(1, "int");


