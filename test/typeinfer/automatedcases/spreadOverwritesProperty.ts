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

// === tests/cases/conformance/types/spread/spreadOverwritesProperty.ts ===
declare function AssertType(value:any, type:string):void;
// without strict null checks, none of these should be an error
declare let ab: { a: number, b: number };
AssertType(ab, "{ a: number; b: number; }");
AssertType(a, "number");
AssertType(b, "number");

declare let abq: { a: number, b?: number };
AssertType(abq, "{ a: number; b?: number; }");
AssertType(a, "number");
AssertType(b, "number");

let unused1 = { b: 1, ...ab 
AssertType(unused1, "{ a: number; b: number; }");

AssertType({ b: 1, ...ab }, "{ a: number; b: number; }");

AssertType(b, "number");

AssertType(1, "int");

AssertType(ab, "{ a: number; b: number; }");
}

let unused2 = { ...ab, ...ab 
AssertType(unused2, "{ a: number; b: number; }");

AssertType({ ...ab, ...ab }, "{ a: number; b: number; }");

AssertType(ab, "{ a: number; b: number; }");

AssertType(ab, "{ a: number; b: number; }");
}

let unused3 = { b: 1, ...abq 
AssertType(unused3, "{ a: number; b: number; }");

AssertType({ b: 1, ...abq }, "{ a: number; b: number; }");

AssertType(b, "number");

AssertType(1, "int");

AssertType(abq, "{ a: number; b?: number; }");
}

function g(obj: { x: number | undefined }) {
AssertType({ x: 1, ...obj }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(obj, "{ x: number; }");
    return { x: 1, ...obj };
}
function h(obj: { x: number }) {
AssertType({ x: 1, ...obj }, "{ x: number; }");
AssertType(x, "number");
AssertType(1, "int");
AssertType(obj, "{ x: number; }");
    return { x: 1, ...obj };
}


