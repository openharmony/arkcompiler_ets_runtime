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

// === tests/cases/compiler/usingModuleWithExportImportInValuePosition.ts ===
declare function AssertType(value:any, type:string):void;
module A {
export let x = 'hello world'
export class Point {
        constructor(public x: number, public y: number) { }
    }
    export module B {
        export interface Id {
            name: string;
        }
    }
}
module C {
    export import a = A;
}

let a: string = C.a.x;
AssertType(a, "string");
AssertType(C.a.x, "string");

let b: { x: number; y: number; } = new C.a.Point(0, 0);
AssertType(b, "{ x: number; y: number; }");
AssertType(x, "number");
AssertType(y, "number");
AssertType(new C.a.Point(0, 0), "A.Point");
AssertType(C.a.Point, "typeof A.Point");
AssertType(0, "int");
AssertType(0, "int");

let c: { name: string };
AssertType(c, "{ name: string; }");
AssertType(name, "string");

let c: C.a.B.Id;
AssertType(c, "{ name: string; }");
AssertType(C, "any");
AssertType(a, "any");
AssertType(B, "any");


