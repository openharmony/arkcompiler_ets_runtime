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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithConstructSignatures.ts ===
declare function AssertType(value:any, type:string):void;

module ConstructSignature {
    declare function foo1(cb: new (x: number) => void): typeof cb;
    declare function foo1(cb: any): any;
    let rarg1: new (x: number) => number;
    let r = foo1(rarg1); // ok because base returns void
    let rarg2: new <T>(x: T) => string;
    let r2 = foo1(rarg2); // ok because base returns void

    declare function foo2(cb: new (x: number, y: number) => void): typeof cb;
    declare function foo2(cb: any): any;
    let r3arg1: new (x: number, y: number) => number;
    let r3 = foo2(r3arg1); // ok because base returns void
    let r4arg1: new <T>(x: T) => string;
    let r4 = foo2(r4arg1); // ok because base returns void
}

