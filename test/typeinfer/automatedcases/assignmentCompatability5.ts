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

// === tests/cases/compiler/assignmentCompatability5.ts ===
declare function AssertType(value:any, type:string):void;
module __test1__ {
    export interface interfaceWithPublicAndOptional<T,U> { one: T; two?: U; };  let obj4: interfaceWithPublicAndOptional<number,string> = { one: 1 };;
    export let __val__obj4 = obj4;
}
module __test2__ {
    export                   interface interfaceOne<T> { one: T; };                let obj1: interfaceOne<number> = { one: 1 };;
    export let __val__obj1 = obj1;
}
__test2__.__val__obj1 = __test1__.__val__obj4
AssertType(__test2__.__val__obj1 = __test1__.__val__obj4, "__test1__.interfaceWithPublicAndOptional<number, string>");
AssertType(__test2__.__val__obj1, "__test2__.interfaceOne<number>");
AssertType(__test1__.__val__obj4, "__test1__.interfaceWithPublicAndOptional<number, string>");


