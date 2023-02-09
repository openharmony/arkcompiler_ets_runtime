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

// === tests/cases/compiler/declarationEmitOverloadedPrivateInference.ts ===
declare function AssertType(value:any, type:string):void;
function noArgs(): string {
AssertType(null as any, "any");
AssertType(null, "null");
    return null as any;
}

function oneArg(input: string): string {
AssertType(null as any, "any");
AssertType(null, "null");
    return null as any;
}

export class Wrapper {
    private proxy<T, U>(fn: (options: T) => U): (options: T) => U;
    private proxy<T, U>(fn: (options?: T) => U, noArgs: true): (options?: T) => U;

    private proxy<T, U>(fn: (options: T) => U) {
AssertType(null as any, "any");
AssertType(null, "null");
        return null as any;
    }

    public Proxies = {
        Failure: this.proxy(noArgs, true),
        Success: this.proxy(oneArg),
    };
}

