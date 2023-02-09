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

// === tests/cases/compiler/typeParameterOrderReversal.ts ===
declare function AssertType(value:any, type:string):void;
interface X<T> {
    n: T;
}

// Only difference here is order of type parameters
function uFirst<U extends X<T>, T>(x: U) { }
function tFirst<T, U extends X<T>>(x: U) { }
let z: X<number> = null;
AssertType(z, "X<number>");
AssertType(null, "null");

// Both of these should be allowed
uFirst(z);
AssertType(uFirst(z), "void");
AssertType(uFirst, "<U extends X<T>, T>(U) => void");
AssertType(z, "X<number>");

tFirst(z);
AssertType(tFirst(z), "void");
AssertType(tFirst, "<T, U extends X<T>>(U) => void");
AssertType(z, "X<number>");


