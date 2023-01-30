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

// === tests/cases/conformance/types/typeRelationships/subtypesAndSuperTypes/subtypingWithCallSignatures.ts ===
declare function AssertType(value:any, type:string):void;

module CallSignature {
    declare function foo1(cb: (x: number) => void): typeof cb;
    declare function foo1(cb: any): any;
    let r = foo1((x: number) => 1); // ok because base returns void
    let r2 = foo1(<T>(x: T) => ''); // ok because base returns void

    declare function foo2(cb: (x: number, y: number) => void): typeof cb;
    declare function foo2(cb: any): any;
    let r3 = foo2((x: number, y: number) => 1); // ok because base returns void
    let r4 = foo2(<T>(x: T) => ''); // ok because base returns void
}

