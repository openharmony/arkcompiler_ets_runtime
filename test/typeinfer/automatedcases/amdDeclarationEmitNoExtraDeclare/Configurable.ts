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

// === tests/cases/compiler/Configurable.ts ===
declare function AssertType(value:any, type:string):void;
export type Constructor<T> = {
    new(...args: any[]): T;
}
export function Configurable<T extends Constructor<{}>>(base: T): T {
AssertType(class extends base {        constructor(...args: any[]) {            super(...args);        }    }, "{ new (...any[]): (Anonymous class); prototype: Configurable<any>.(Anonymous class); } & T");
AssertType(base, "{}");
    return class extends base {

        constructor(...args: any[]) {
AssertType(args, "any[]");

            super(...args);
AssertType(super(...args), "void");
AssertType(super, "T");
AssertType(...args, "any");
AssertType(args, "any[]");
        }

    };
}


