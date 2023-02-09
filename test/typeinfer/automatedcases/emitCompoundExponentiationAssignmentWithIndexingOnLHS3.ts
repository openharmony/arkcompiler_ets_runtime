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

// === tests/cases/conformance/es7/exponentiationOperator/emitCompoundExponentiationAssignmentWithIndexingOnLHS3.ts ===
declare function AssertType(value:any, type:string):void;
let object = {
AssertType(object, "{ _0: number; 0: number; }");
AssertType({    _0: 2,    get 0() {        return this._0;    },    set 0(x: number) {        this._0 = x;    },}, "{ _0: number; 0: number; }");

    _0: 2,
AssertType(_0, "number");
AssertType(2, "int");

    get 0() {
AssertType(0, "number");

AssertType(this._0, "any");
AssertType(this, "any");
        return this._0;

    },
    set 0(x: number) {
AssertType(0, "number");
AssertType(x, "number");

        this._0 = x;
AssertType(this._0 = x, "number");
AssertType(this._0, "any");
AssertType(this, "any");
AssertType(x, "number");

    },
}
object[0] **= object[0];
AssertType(object[0] **= object[0], "number");
AssertType(object[0], "number");
AssertType(object, "{ _0: number; 0: number; }");
AssertType(0, "int");
AssertType(object[0], "number");
AssertType(object, "{ _0: number; 0: number; }");
AssertType(0, "int");

object[0] **= object[0] **= 2;
AssertType(object[0] **= object[0] **= 2, "number");
AssertType(object[0], "number");
AssertType(object, "{ _0: number; 0: number; }");
AssertType(0, "int");
AssertType(object[0] **= 2, "number");
AssertType(object[0], "number");
AssertType(object, "{ _0: number; 0: number; }");
AssertType(0, "int");
AssertType(2, "int");

object[0] **= object[0] ** 2;
AssertType(object[0] **= object[0] ** 2, "number");
AssertType(object[0], "number");
AssertType(object, "{ _0: number; 0: number; }");
AssertType(0, "int");
AssertType(object[0] ** 2, "number");
AssertType(object[0], "number");
AssertType(object, "{ _0: number; 0: number; }");
AssertType(0, "int");
AssertType(2, "int");


