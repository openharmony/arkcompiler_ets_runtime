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

// === tests/cases/conformance/types/rest/objectRestForOf.ts ===
declare function AssertType(value:any, type:string):void;
let array: { x: number, y: string }[];
AssertType(array, "{ x: number; y: string; }[]");
AssertType(x, "number");
AssertType(y, "string");

for (let { x, ...restOf } of array) {
    [x, restOf];
AssertType([x, restOf], "(union)[]");
AssertType(x, "number");
AssertType(restOf, "{ y: string; }");
}
let xx: number;
AssertType(xx, "number");

let rrestOff: { y: string };
AssertType(rrestOff, "{ y: string; }");
AssertType(y, "string");

for ({ x: xx, ...rrestOff } of array ) {
    [xx, rrestOff];
AssertType([xx, rrestOff], "(union)[]");
AssertType(xx, "number");
AssertType(rrestOff, "{ y: string; }");
}
for (const norest of array.map(a => ({ ...a, x: 'a string' }))) {
    [norest.x, norest.y];
AssertType([norest.x, norest.y], "string[]");
AssertType(norest.x, "string");
AssertType(norest.y, "string");

    // x is now a string. who knows why.
}


