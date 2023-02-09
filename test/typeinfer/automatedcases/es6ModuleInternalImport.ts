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

// === tests/cases/compiler/es6ModuleInternalImport.ts ===
declare function AssertType(value:any, type:string):void;
export module m {
    export let a = 10;
}
export import a1 = m.a;
import a2 = m.a;
let x = a1 + a2;
AssertType(x, "number");
AssertType(a1 + a2, "number");
AssertType(a1, "number");
AssertType(a2, "number");

export module m1 {
    export import a3 = m.a;
    import a4 = m.a;
    let x = a1 + a2;
    let x2 = a3 + a4;
}
module m2 {
    export import a3 = m.a;
    import a4 = m.a;
    let x = a1 + a2;
    let x2 = a3 + a4;
    let x4 = m1.a3 + m2.a3;
}

