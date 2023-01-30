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

// === tests/cases/compiler/implicitAnyAnyReturningFunction.ts ===
declare function AssertType(value:any, type:string):void;
function A() {
AssertType(<any>"", "any");
AssertType("", "string");
    return <any>"";
}

function B() {
    let someLocal: any = {};
AssertType(someLocal, "any");
AssertType({}, "{}");

AssertType(someLocal, "any");
    return someLocal;
}

class C {
    public A() {
AssertType(<any>"", "any");
AssertType("", "string");
        return <any>"";
    }

    public B() {
        let someLocal: any = {};
AssertType(someLocal, "any");
AssertType({}, "{}");

AssertType(someLocal, "any");
        return someLocal;
    }
}


