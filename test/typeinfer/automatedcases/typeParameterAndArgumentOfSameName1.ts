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

// === tests/cases/compiler/typeParameterAndArgumentOfSameName1.ts ===
declare function AssertType(value:any, type:string):void;
function f<A extends Number>(A: A): A {
    let r = A.toExponential(123);
AssertType(r, "string");
AssertType(A.toExponential(123), "string");
AssertType(A.toExponential, "(?number) => string");
AssertType(123, "int");

AssertType(null, "null");
    return null;
}

