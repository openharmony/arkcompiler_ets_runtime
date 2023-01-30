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

// === tests/cases/compiler/constraintReferencingTypeParameterFromSameTypeParameterList.ts ===
declare function AssertType(value:any, type:string):void;

// used to be valid, now an error to do this

interface IComparable<T> {
}
function f<T, I extends IComparable<T>>() {
}

interface I1<T, U extends I1<T, any>> { // Error, any does not satisfy the constraint I1<T, any>
}
interface I2<T, U extends T> {
}

interface I4<T, U extends () => T> {
}

// No error
interface I3<T, U extends string> {
    method1<X, Y extends T>();
}

function foo<T, U extends <V extends T>(v: V) => void>() {
}



