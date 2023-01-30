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

// === tests/cases/conformance/es6/classDeclaration/superCallBeforeThisAccessing8.ts ===
declare function AssertType(value:any, type:string):void;
class Base {
    constructor(c) { }
}
class D extends Base {
    private _t;
    constructor() {
        let x = {
AssertType(x, "{ k: void; j: any; }");
AssertType({            k: super(undefined),             j: this._t,  // no error        }, "{ k: void; j: any; }");

            k: super(undefined), 
AssertType(k, "void");
AssertType(super(undefined), "void");
AssertType(super, "typeof Base");
AssertType(undefined, "undefined");

            j: this._t,  // no error
AssertType(j, "any");
AssertType(this._t, "any");
AssertType(this, "this");
        }
    }
}


