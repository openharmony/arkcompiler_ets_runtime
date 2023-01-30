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

// === tests/cases/compiler/constAndNS.ts ===
declare function AssertType(value:any, type:string):void;
type A = number;
declare const Q: number;
AssertType(Q, "number");

declare namespace Q {
    export { A };
}
declare const try1: Q.A;
AssertType(try1, "number");
AssertType(Q, "any");

declare namespace Q2 {
    export { Q }
}
declare const try2: Q2.Q.A;
AssertType(try2, "number");
AssertType(Q2, "any");
AssertType(Q, "any");

declare namespace Q3 {
    export {A as B};
}
declare const try3: Q3.B;
AssertType(try3, "number");
AssertType(Q3, "any");

declare namespace Q4 {
    export { Q as default };
}
declare const try4: Q4.default.A;
AssertType(try4, "number");
AssertType(Q4, "any");
AssertType(default, "any");

export {};

