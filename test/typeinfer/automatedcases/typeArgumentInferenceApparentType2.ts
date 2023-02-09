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

// === tests/cases/compiler/typeArgumentInferenceApparentType2.ts ===
declare function AssertType(value:any, type:string):void;
function method<T>(iterable: Iterable<T>): T {
    function inner<U extends Iterable<T>>() {
AssertType(inner, "<U extends Iterable<T>>() => void");

        let u: U;
AssertType(u, "U");

        let res: T = method(u);
AssertType(res, "T");
AssertType(method(u), "T");
AssertType(method, "<T>(Iterable<T>) => T");
AssertType(u, "U");
    }
    return;
}

