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

// === tests/cases/compiler/collisionThisExpressionAndPropertyNameAsConstuctorParameter.ts ===
declare function AssertType(value:any, type:string):void;
class Foo2 {
    constructor(_this: number) { //Error
        let lambda = () => {
AssertType(lambda, "() => (any) => this");
AssertType(() => {            return x => this;   // New scope.  So should inject new _this capture        }, "() => (any) => this");

AssertType(x => this, "(any) => this");
AssertType(x, "any");
AssertType(this, "this");
            return x => this;   // New scope.  So should inject new _this capture
        }
    }
}

class Foo3 {
    constructor(private _this: number) { // Error
        let lambda = () => {
AssertType(lambda, "() => (any) => this");
AssertType(() => {            return x => this;   // New scope.  So should inject new _this capture        }, "() => (any) => this");

AssertType(x => this, "(any) => this");
AssertType(x, "any");
AssertType(this, "this");
            return x => this;   // New scope.  So should inject new _this capture
        }
    }
}   

class Foo4 {
    constructor(_this: number); // No code gen - no error
    constructor(_this: string); // No code gen - no error
    constructor(_this: any) { // Error
        let lambda = () => {
AssertType(lambda, "() => (any) => this");
AssertType(() => {            return x => this;   // New scope.  So should inject new _this capture        }, "() => (any) => this");

AssertType(x => this, "(any) => this");
AssertType(x, "any");
AssertType(this, "this");
            return x => this;   // New scope.  So should inject new _this capture
        }
    }
}  

class Foo5 {
    constructor(_this: number); // No code gen - no error
    constructor(_this: string); // No code gen - no error
    constructor(private _this: any) { // Error
        let lambda = () => {
AssertType(lambda, "() => (any) => this");
AssertType(() => {            return x => this;   // New scope.  So should inject new _this capture        }, "() => (any) => this");

AssertType(x => this, "(any) => this");
AssertType(x, "any");
AssertType(this, "this");
            return x => this;   // New scope.  So should inject new _this capture
        }
    }
}  

