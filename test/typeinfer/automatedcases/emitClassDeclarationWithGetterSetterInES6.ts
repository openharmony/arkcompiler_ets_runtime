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

// === tests/cases/conformance/es6/classDeclaration/emitClassDeclarationWithGetterSetterInES6.ts ===
declare function AssertType(value:any, type:string):void;
class C {
    _name: string;
    get name(): string {
AssertType(this._name, "string");
AssertType(this, "this");
        return this._name;
    }
    static get name2(): string {
AssertType("BYE", "string");
        return "BYE";
    }
    static get ["computedname"]() {
AssertType("", "string");
        return "";
    }
    get ["computedname1"]() {
AssertType("", "string");
        return "";
    }
    get ["computedname2"]() {
AssertType("", "string");
        return "";
    }

    set ["computedname3"](x: any) {
    }
    set ["computedname4"](y: string) {
    }

    set foo(a: string) { }
    static set bar(b: number) { }
    static set ["computedname"](b: string) { }
}

