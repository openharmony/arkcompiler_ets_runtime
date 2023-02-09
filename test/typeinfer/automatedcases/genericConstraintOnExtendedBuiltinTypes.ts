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

// === tests/cases/compiler/genericConstraintOnExtendedBuiltinTypes.ts ===
declare function AssertType(value:any, type:string):void;
declare module EndGate {
    export interface ICloneable {
        Clone(): any;
    }
}

interface Number extends EndGate.ICloneable { }

module EndGate.Tweening {
    export class Tween<T extends ICloneable>{
        private _from: T;


        constructor(from: T) {
            this._from = from.Clone();
AssertType(this._from = from.Clone(), "any");
AssertType(this._from, "T");
AssertType(this, "this");
AssertType(from.Clone(), "any");
AssertType(from.Clone, "() => any");
        }
    }
}

module EndGate.Tweening {
    export class NumberTween extends Tween<number>{
        constructor(from: number) {
            super(from);
AssertType(super(from), "void");
AssertType(super, "typeof Tween");
AssertType(from, "number");
        }
    }
}

