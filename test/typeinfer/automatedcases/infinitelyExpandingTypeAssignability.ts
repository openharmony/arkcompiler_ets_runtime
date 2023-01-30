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

// === tests/cases/compiler/infinitelyExpandingTypeAssignability.ts ===
declare function AssertType(value:any, type:string):void;
interface A<T> {
   x : T
}
 
interface B<T> extends A<B<B<B<T>>>> { }

interface C<T> extends A<C<C<C<T>>>> { }
 
let x : B<string>
AssertType(x, "B<string>");

let y : C<string> = x
AssertType(y, "C<string>");
AssertType(x, "B<string>");


