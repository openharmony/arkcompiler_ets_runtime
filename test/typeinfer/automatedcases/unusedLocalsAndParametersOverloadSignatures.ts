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

// === tests/cases/compiler/unusedLocalsAndParametersOverloadSignatures.ts ===
declare function AssertType(value:any, type:string):void;
export function func(details: number, message: string, ...args: any[]): void;
export function func(details: number, message: string): any {
AssertType(details + message, "string");
AssertType(details, "number");
AssertType(message, "string");
    return details + message;
}

export class C {
    constructor(details: number, message: string, ...args: any[]);
    constructor(details: number, message: string) {
        details + message;
AssertType(details + message, "string");
AssertType(details, "number");
AssertType(message, "string");
    }

    method(details: number, message: string, ...args: any[]): void;
    method(details: number, message: string): any {
AssertType(details + message, "string");
AssertType(details, "number");
AssertType(message, "string");
        return details + message;
    }
}


export function genericFunc<T>(details: number, message: T, ...args: any[]): void;
export function genericFunc(details: number, message: any): any {
AssertType(details + message, "any");
AssertType(details, "number");
AssertType(message, "any");
    return details + message;
}

