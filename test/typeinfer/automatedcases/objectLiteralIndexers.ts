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

// === tests/cases/compiler/objectLiteralIndexers.ts ===
declare function AssertType(value:any, type:string):void;
interface A {
    x: number;
}

interface B extends A {
    y: string;
}

let a: A;
AssertType(a, "A");

let b: B;
AssertType(b, "B");

let c: any;
AssertType(c, "any");

let o1: { [s: string]: A;[n: number]: B; } = { x: a, 0: b }; // string indexer is A, number indexer is B
AssertType(o1, "{ [string]: A; [number]: B; }");
AssertType(s, "string");
AssertType(n, "number");
AssertType({ x: a, 0: b }, "{ x: A; 0: B; }");
AssertType(x, "A");
AssertType(a, "A");
AssertType(0, "B");
AssertType(b, "B");

o1 = { x: b, 0: c }; // both indexers are any
AssertType(o1 = { x: b, 0: c }, "{ x: B; 0: any; }");
AssertType(o1, "{ [string]: A; [number]: B; }");
AssertType({ x: b, 0: c }, "{ x: B; 0: any; }");
AssertType(x, "B");
AssertType(b, "B");
AssertType(0, "any");
AssertType(c, "any");

o1 = { x: c, 0: b }; // string indexer is any, number indexer is B
AssertType(o1 = { x: c, 0: b }, "{ x: any; 0: B; }");
AssertType(o1, "{ [string]: A; [number]: B; }");
AssertType({ x: c, 0: b }, "{ x: any; 0: B; }");
AssertType(x, "any");
AssertType(c, "any");
AssertType(0, "B");
AssertType(b, "B");


