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

// === tests/cases/compiler/arrayConcat3.ts ===
declare function AssertType(value:any, type:string):void;
// TODO: remove lib hack when https://github.com/Microsoft/TypeScript/issues/20454 is fixed
type Fn<T extends object> = <U extends T>(subj: U) => U
function doStuff<T extends object, T1 extends T>(a: Array<Fn<T>>, b: Array<Fn<T1>>) {
    b.concat(a);
AssertType(b.concat(a), "Fn<T1>[]");
AssertType(b.concat, "{ (...ConcatArray<Fn<T1>>[]): Fn<T1>[]; (...(union)[]): Fn<T1>[]; }");
AssertType(a, "Fn<T>[]");
}


