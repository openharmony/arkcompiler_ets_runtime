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

// === tests/cases/compiler/multiModuleFundule1.ts ===
declare function AssertType(value:any, type:string):void;
function C(x: number) { }

module C {
    export let x = 1;
}
module C {
    export function foo() { }
}

let r = C(2);
AssertType(r, "void");
AssertType(C(2), "void");
AssertType(C, "typeof C");
AssertType(2, "int");

let r2 = new C(2); // using void 
AssertType(r2, "any");
AssertType(new C(2), "any");
AssertType(C, "typeof C");
AssertType(2, "int");
returning function as constructor

let r3 = C.foo();
AssertType(r3, "void");
AssertType(C.foo(), "void");
AssertType(C.foo, "() => void");


