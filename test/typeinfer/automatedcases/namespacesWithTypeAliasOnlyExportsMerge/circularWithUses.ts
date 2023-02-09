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

// === tests/cases/compiler/circularWithUses.ts ===
declare function AssertType(value:any, type:string):void;
type A = string;
type B = number;
declare namespace NS1 {
    export { NS2, A };
}
declare namespace NS2 {
    export { NS1, B };
}
export {};
declare const try1: NS1.A;
AssertType(try1, "string");
AssertType(NS1, "any");

declare const try2: NS2.B;
AssertType(try2, "number");
AssertType(NS2, "any");

declare const try3: NS1.NS2.B;
AssertType(try3, "number");
AssertType(NS1, "any");
AssertType(NS2, "any");

declare const try4: NS2.NS1.A;
AssertType(try4, "string");
AssertType(NS2, "any");
AssertType(NS1, "any");

declare const try5: NS1.NS2.NS1.A;
AssertType(try5, "string");
AssertType(NS1, "any");
AssertType(NS2, "any");
AssertType(NS1, "any");

declare const try6: NS2.NS1.NS2.B;
AssertType(try6, "number");
AssertType(NS2, "any");
AssertType(NS1, "any");
AssertType(NS2, "any");


