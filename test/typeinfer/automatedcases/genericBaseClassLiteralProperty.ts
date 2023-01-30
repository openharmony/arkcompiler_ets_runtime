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

// === tests/cases/compiler/genericBaseClassLiteralProperty.ts ===
declare function AssertType(value:any, type:string):void;
class BaseClass<T> {
    public _getValue1: { (): T; };
    public _getValue2: () => T;
}

class SubClass extends BaseClass<number> {
    public Error(): void {

        let x : number = this._getValue1();
AssertType(x, "number");
AssertType(this._getValue1(), "number");
AssertType(this._getValue1, "() => number");
AssertType(this, "this");

        let y : number = this._getValue2();
AssertType(y, "number");
AssertType(this._getValue2(), "number");
AssertType(this._getValue2, "() => number");
AssertType(this, "this");
    }
}

