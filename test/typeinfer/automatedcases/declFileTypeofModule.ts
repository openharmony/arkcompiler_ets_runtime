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

// === tests/cases/compiler/declFileTypeofModule.ts ===
declare function AssertType(value:any, type:string):void;
module m1 {
    export let c: string;
}
let m1_1 = m1;
AssertType(m1_1, "typeof m1");
AssertType(m1, "typeof m1");

let m1_2: typeof m1;
AssertType(m1_2, "typeof m1");
AssertType(m1, "typeof m1");

module m2 {
    export let d: typeof m2;
}

let m2_1 = m2;
AssertType(m2_1, "typeof m2");
AssertType(m2, "typeof m2");

let m2_2: typeof m2;
AssertType(m2_2, "typeof m2");
AssertType(m2, "typeof m2");


