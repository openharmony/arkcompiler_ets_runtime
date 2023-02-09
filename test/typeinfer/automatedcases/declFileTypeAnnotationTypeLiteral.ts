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

// === tests/cases/compiler/declFileTypeAnnotationTypeLiteral.ts ===
declare function AssertType(value:any, type:string):void;
class c {
}
class g<T> {
}
module m {
    export class c {
    }
}

// Object literal with everything
let x: {
AssertType(x, "{ (number): c; (string): g<string>; new (number): c; new (string): m.c; [number]: c; [string]: c; a: c; b: g<string>; m1(): g<number>; m2(string, ?number, ...c[]): string; }");

    // Call signatures
    (a: number): c;
AssertType(a, "number");

    (a: string): g<string>;
AssertType(a, "string");

    // Construct signatures
    new (a: number): c;
AssertType(a, "number");

    new (a: string): m.c;
AssertType(a, "string");
AssertType(m, "any");

    // Indexers
    [n: number]: c;
AssertType(n, "number");

    [n: string]: c;
AssertType(n, "string");

    // Properties
    a: c;
AssertType(a, "c");

    b: g<string>;
AssertType(b, "g<string>");

    // methods
    m1(): g<number>;
AssertType(m1, "() => g<number>");

    m2(a: string, b?: number, ...c: c[]): string;
AssertType(m2, "(string, ?number, ...c[]) => string");
AssertType(a, "string");
AssertType(b, "number");
AssertType(c, "c[]");

};


// Function type
let y: (a: string) => string;
AssertType(y, "(string) => string");
AssertType(a, "string");

// constructor type
let z: new (a: string) => m.c;
AssertType(z, "new (string) => m.c");
AssertType(a, "string");
AssertType(m, "any");


