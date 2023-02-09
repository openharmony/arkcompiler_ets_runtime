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

// === tests/cases/compiler/unusedParametersThis.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    public a: number;

    public method(this: this): number {
AssertType(this.a, "number");
AssertType(this, "this");
        return this.a;
    }

    public method2(this: A): number {
AssertType(this.a, "number");
AssertType(this, "A");
        return this.a;
    }

    public method3(this: this): number {
        let fn = () => this.a;
AssertType(fn, "() => number");
AssertType(() => this.a, "() => number");
AssertType(this.a, "number");
AssertType(this, "this");

AssertType(fn(), "number");
AssertType(fn, "() => number");
        return fn();
    }

    public method4(this: A): number {
        let fn = () => this.a;
AssertType(fn, "() => number");
AssertType(() => this.a, "() => number");
AssertType(this.a, "number");
AssertType(this, "A");

AssertType(fn(), "number");
AssertType(fn, "() => number");
        return fn();
    }

    static staticMethod(this: A): number {
AssertType(this.a, "number");
AssertType(this, "A");
        return this.a;
    }
}

function f(this: A): number {
AssertType(this.a, "number");
AssertType(this, "A");
    return this.a
}

let f2 = function f2(this: A): number {
AssertType(f2, "(A) => number");
AssertType(f2, "(A) => number");
AssertType(this, "A");
AssertType(function f2(this: A): number {    return this.a;}, "(A) => number");

AssertType(this.a, "number");
AssertType(this, "A");
    return this.a;

};

