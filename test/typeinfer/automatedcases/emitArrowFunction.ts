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

// === tests/cases/conformance/es6/arrowFunction/emitArrowFunction.ts ===
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

let f4 = (x: string, y: number, z = 10) => { 
AssertType(f4, "(string, number, ?number) => void");

AssertType((x: string, y: number, z = 10) => { }, "(string, number, ?number) => void");

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


