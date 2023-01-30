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

// === tests/cases/conformance/es6/arrowFunction/emitArrowFunctionES6.ts ===
declare function AssertType(value:any, type:string):void;
let f1 = () => { 
AssertType(f1, "() => void");

AssertType(() => { }, "() => void");
}

let f2 = (x: string, y: string) => { 
AssertType(f2, "(string, string) => void");

AssertType((x: string, y: string) => { }, "(string, string) => void");

AssertType(x, "string");

AssertType(y, "string");
}

let f3 = (x: string, y: number, ...rest) => { 
AssertType(f3, "(string, number, ...any[]) => void");

AssertType((x: string, y: number, ...rest) => { }, "(string, number, ...any[]) => void");

AssertType(x, "string");

AssertType(y, "number");

AssertType(rest, "any[]");
}

let f4 = (x: string, y: number, z=10) => { 
AssertType(f4, "(string, number, ?number) => void");

AssertType((x: string, y: number, z=10) => { }, "(string, number, ?number) => void");

AssertType(x, "string");

AssertType(y, "number");

AssertType(z, "number");

AssertType(10, "int");
}

function foo(func: () => boolean) { }
foo(() => true);
AssertType(foo(() => true), "void");
AssertType(foo, "(() => boolean) => void");
AssertType(() => true, "() => true");
AssertType(true, "boolean");

foo(() => { 
AssertType(foo(() => { return false; }), "void");
AssertType(foo, "(() => boolean) => void");
AssertType(() => { return false; }, "() => false");
AssertType(false, "boolean");
return false; });

// Binding patterns in arrow functions
let p1 = ([a]) => { };
AssertType(p1, "([any]) => void");
AssertType(([a]) => { }, "([any]) => void");
AssertType(a, "any");

let p2 = ([...a]) => { };
AssertType(p2, "(Iterable<any>) => void");
AssertType(([...a]) => { }, "(Iterable<any>) => void");
AssertType(a, "any[]");

let p3 = ([, a]) => { };
AssertType(p3, "([any, any]) => void");
AssertType(([, a]) => { }, "([any, any]) => void");
AssertType(, "undefined");
AssertType(a, "any");

let p4 = ([, ...a]) => { };
AssertType(p4, "([any?, ...any[]]) => void");
AssertType(([, ...a]) => { }, "([any?, ...any[]]) => void");
AssertType(, "undefined");
AssertType(a, "any[]");

let p5 = ([a = 1]) => { };
AssertType(p5, "([number?]) => void");
AssertType(([a = 1]) => { }, "([number?]) => void");
AssertType(a, "number");
AssertType(1, "int");

let p6 = ({ a }) => { };
AssertType(p6, "({ a: any; }) => void");
AssertType(({ a }) => { }, "({ a: any; }) => void");
AssertType(a, "any");

let p7 = ({ a: { b } }) => { };
AssertType(p7, "({ a: { b: any; }; }) => void");
AssertType(({ a: { b } }) => { }, "({ a: { b: any; }; }) => void");
AssertType(a, "any");
AssertType(b, "any");

let p8 = ({ a = 1 }) => { };
AssertType(p8, "({ a?: number; }) => void");
AssertType(({ a = 1 }) => { }, "({ a?: number; }) => void");
AssertType(a, "number");
AssertType(1, "int");

let p9 = ({ a: { b = 1 } = { b: 1 } }) => { };
AssertType(p9, "({ a?: { b?: number; }; }) => void");
AssertType(({ a: { b = 1 } = { b: 1 } }) => { }, "({ a?: { b?: number; }; }) => void");
AssertType(a, "any");
AssertType(b, "number");
AssertType(1, "int");
AssertType({ b: 1 }, "{ b?: number; }");
AssertType(b, "number");
AssertType(1, "int");

let p10 = ([{ value, done }]) => { };
AssertType(p10, "([{ value: any; done: any; }]) => void");
AssertType(([{ value, done }]) => { }, "([{ value: any; done: any; }]) => void");
AssertType(value, "any");
AssertType(done, "any");


