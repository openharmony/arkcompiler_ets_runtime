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

// === tests/cases/compiler/enumAssignmentCompat4.ts ===
declare function AssertType(value:any, type:string):void;
namespace M {
    export enum MyEnum {
        BAR
    }
    export let object2 = {
        foo: MyEnum.BAR
    };
}

namespace N {
    export enum MyEnum {
        FOO
    };
    export let object1 = {
        foo: MyEnum.FOO
    };
}

let broken = [
AssertType(broken, "(union)[]");
AssertType([    N.object1,    M.object2], "(union)[]");

    N.object1,
AssertType(N.object1, "{ foo: N.MyEnum; }");

    M.object2
AssertType(M.object2, "{ foo: M.MyEnum; }");

];


