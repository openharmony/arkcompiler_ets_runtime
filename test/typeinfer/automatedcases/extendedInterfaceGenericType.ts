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

// === tests/cases/compiler/extendedInterfaceGenericType.ts ===
declare function AssertType(value:any, type:string):void;
interface Alpha<T> {
    takesArgOfT(arg: T): Alpha<T>;
    makeBetaOfNumber(): Beta<number>;
}
interface Beta<T> extends Alpha<T> {
}

let alpha: Alpha<number>;
AssertType(alpha, "Alpha<number>");

let betaOfNumber = alpha.makeBetaOfNumber();
AssertType(betaOfNumber, "Beta<number>");
AssertType(alpha.makeBetaOfNumber(), "Beta<number>");
AssertType(alpha.makeBetaOfNumber, "() => Beta<number>");

betaOfNumber.takesArgOfT(5);
AssertType(betaOfNumber.takesArgOfT(5), "Alpha<number>");
AssertType(betaOfNumber.takesArgOfT, "(number) => Alpha<number>");
AssertType(5, "int");


