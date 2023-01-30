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

// === tests/cases/conformance/statements/forStatements/forStatementsMultipleValidDecl.ts ===
declare function AssertType(value:any, type:string):void;
// all expected to be valid

for (let x: number; ;) { }
for (let x = 2; ;) { }

for (let x = <number>undefined; ;) { }
// new declaration space, making redeclaring x as a string valid
function declSpace() {
    for (let x = 'this is a string'; ;) { 
AssertType(x, "string");

AssertType('this is a string', "string");
}
}
interface Point { x: number; y: number; }

for (let p: Point; ;) { }
for (let p = { x: 1, y: 2 }; ;) { }
for (let p: Point = { x: 0, y: undefined }; ;) { }
for (let p = { x: 1, y: <number>undefined }; ;) { }
for (let p: { x: number; y: number; } = { x: 1, y: 2 }; ;) { }
for (let p = <{ x: number; y: number; }>{ x: 0, y: undefined }; ;) { }
for (let p: typeof p; ;) { }

for (let fn = function (s: string) { 
AssertType(42, "int");
return 42; }; ;) { 
}

for (let fn = (s: string) => 3; ;) { }
for (let fn: (s: string) => number; ;) { }
for (let fn: { (s: string): number }; ;) { }
for (let fn = <(s: string) => number> null; ;) { }
for (let fn: typeof fn; ;) { }

for (let a: string[]; ;) { }
for (let a = ['a', 'b']; ;) { }
for (let a = <string[]>[]; ;) { }
for (let a: string[] = []; ;) { }
for (let a = new Array<string>(); ;) { }
for (let a: typeof a; ;) { }

