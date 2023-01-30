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

// === tests/cases/compiler/mergedDeclarations4.ts ===
declare function AssertType(value:any, type:string):void;
module M {
    export function f() { }
    f();
    M.f();    
    let r = f.hello;
}
 
module M {
    export module f {
        export let hello = 1;
    }
    f();
    M.f();
    let r = f.hello;
}
 
M.f();
AssertType(M.f(), "void");
AssertType(M.f, "typeof M.f");

M.f.hello;
AssertType(M.f.hello, "number");


