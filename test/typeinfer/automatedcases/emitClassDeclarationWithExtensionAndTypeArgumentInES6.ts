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

// === tests/cases/conformance/es6/classDeclaration/emitClassDeclarationWithExtensionAndTypeArgumentInES6.ts ===
declare function AssertType(value:any, type:string):void;
class B<T> {
    constructor(a: T) { }
}
class C extends B<string> { }
class D extends B<number> {
    constructor(a: any)
    constructor(b: number) {
        super(b);
AssertType(super(b), "void");
AssertType(super, "typeof B");
AssertType(b, "number");
    }
}

