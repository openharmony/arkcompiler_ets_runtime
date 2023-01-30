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

// === tests/cases/compiler/contextualTypeAppliedToVarArgs.ts ===
declare function AssertType(value:any, type:string):void;
function delegate(instance: any, method: (...args: any[]) => any, data?: any): (...args: any[]) => any {
AssertType(function () { }, "() => void");
    return function () { };
}

class Foo{


    Bar() {
        delegate(this, function (source, args2)
AssertType(delegate(this, function (source, args2)        {            let a = source.node;            let b = args2.node;        } ), "(...any[]) => any");
AssertType(delegate, "(any, (...any[]) => any, ?any) => (...any[]) => any");
AssertType(this, "this");
AssertType(function (source, args2)        {            let a = source.node;            let b = args2.node;        }, "(any, any) => void");
AssertType(source, "any");
AssertType(args2, "any");
        {
            let a = source.node;
AssertType(a, "any");
AssertType(source.node, "any");

            let b = args2.node;
AssertType(b, "any");
AssertType(args2.node, "any");

        } );
    }
}


