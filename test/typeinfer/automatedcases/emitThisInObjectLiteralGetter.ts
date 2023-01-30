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

// === tests/cases/compiler/emitThisInObjectLiteralGetter.ts ===
declare function AssertType(value:any, type:string):void;
const example = {
AssertType(example, "{ readonly foo: (any) => any; }");
AssertType({    get foo() {        return item => this.bar(item);    }}, "{ readonly foo: (any) => any; }");

    get foo() {
AssertType(foo, "(any) => any");

AssertType(item => this.bar(item), "(any) => any");
AssertType(item, "any");
AssertType(this.bar(item), "any");
AssertType(this.bar, "any");
AssertType(this, "any");
AssertType(item, "any");
        return item => this.bar(item);
    }
};


