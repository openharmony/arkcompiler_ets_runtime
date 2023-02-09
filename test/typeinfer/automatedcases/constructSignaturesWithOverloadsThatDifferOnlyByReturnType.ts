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

// === tests/cases/conformance/types/objectTypeLiteral/constructSignatures/constructSignaturesWithOverloadsThatDifferOnlyByReturnType.ts ===
declare function AssertType(value:any, type:string):void;
// Error for construct signature overloads to differ only by return type

class C {
    constructor(x: number) { }
}

class C2<T> {
    constructor(x: T, y?: string) { }
}

interface I {
    new(x: number, y: string): C;
    new(x: number, y: string): C2<number>; // error
}

interface I2<T> {
    new (x: T, y: string): C2<number>;
    new (x: T, y: string): C; // error
    new <T>(x: T, y: string): C2<T>;
    new <T>(x: T, y: string): C; // error

}

let a: {
AssertType(a, "{ new (number, string): C2<number>; new (number, string): C; }");

    new (x: number, y: string): C2<number>;
AssertType(x, "number");
AssertType(y, "string");

    new (x: number, y: string): C; // error
AssertType(x, "number");
AssertType(y, "string");
}

let b: {
AssertType(b, "{ new <T>(T, string): C2<T>; new <T>(T, string): C; }");

    new <T>(x: T, y: string): C2<T>;
AssertType(x, "T");
AssertType(y, "string");

    new <T>(x: T, y: string): C; // error
AssertType(x, "T");
AssertType(y, "string");
}

