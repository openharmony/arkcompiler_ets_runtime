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

// === tests/cases/compiler/es6ModuleConst.ts ===
declare function AssertType(value:any, type:string):void;
export const a = "hello";
AssertType(a, "string");
AssertType("hello", "string");

export const x: string = a, y = x;
AssertType(x, "string");
AssertType(a, "string");
AssertType(y, "string");
AssertType(x, "string");

const b = y;
AssertType(b, "string");
AssertType(y, "string");

const c: string = b, d = c;
AssertType(c, "string");
AssertType(b, "string");
AssertType(d, "string");
AssertType(c, "string");

export module m1 {
    export const k = a;
    export const l: string = b, m = k;
    const n = m1.k;
    const o: string = n, p = k;
}
module m2 {
    export const k = a;
    export const l: string = b, m = k;
    const n = m1.k;
    const o: string = n, p = k;
}

