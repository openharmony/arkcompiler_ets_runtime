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

// === tests/cases/compiler/letArgsOnConstructorTypes.ts ===
declare function AssertType(value:any, type:string):void;
export class A {
    constructor(ctor) { }
}

export class B extends A {
    private p1: number;
    private p2: string;

    constructor(element: any, url: string) {
       super(element);
AssertType(super(element), "void");
AssertType(super, "typeof A");
AssertType(element, "any");

        this.p1 = element;
AssertType(this.p1 = element, "any");
AssertType(this.p1, "number");
AssertType(this, "this");
AssertType(element, "any");

        this.p2 = url;
AssertType(this.p2 = url, "string");
AssertType(this.p2, "string");
AssertType(this, "this");
AssertType(url, "string");
    }
}

export interface I1 {
    register(inputClass: new(...params: any[]) => A);
    register(inputClass: { new (...params: any[]): A; }[]);
}


let reg: I1;
AssertType(reg, "I1");

reg.register(B);
AssertType(reg.register(B), "any");
AssertType(reg.register, "{ (new (...any[]) => A): any; ((new (...any[]) => A)[]): any; }");
AssertType(B, "typeof B");


