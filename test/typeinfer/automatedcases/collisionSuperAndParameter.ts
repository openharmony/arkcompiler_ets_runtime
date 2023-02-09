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

// === tests/cases/compiler/collisionSuperAndParameter.ts ===
declare function AssertType(value:any, type:string):void;
class Foo {
    a() {
        let lamda = (_super: number) => { // No Error 
AssertType(lamda, "(number) => (any) => this");
AssertType(_super, "number");
AssertType((_super: number) => { // No Error             return x => this;   // New scope.  So should inject new _this capture        }, "(number) => (any) => this");

AssertType(x => this, "(any) => this");
AssertType(x, "any");
AssertType(this, "this");
            return x => this;   // New scope.  So should inject new _this capture
        }
    }
    b(_super: number) { // No Error 
        let lambda = () => {
AssertType(lambda, "() => (any) => this");
AssertType(() => {            return x => this;   // New scope.  So should inject new _this capture        }, "() => (any) => this");

AssertType(x => this, "(any) => this");
AssertType(x, "any");
AssertType(this, "this");
            return x => this;   // New scope.  So should inject new _this capture
        }
    }
    set c(_super: number) { // No error
    }
}
class Foo2 extends Foo {
    x() {
        let lamda = (_super: number) => { // Error 
AssertType(lamda, "(number) => (any) => this");
AssertType(_super, "number");
AssertType((_super: number) => { // Error             return x => this;   // New scope.  So should inject new _this capture        }, "(number) => (any) => this");

AssertType(x => this, "(any) => this");
AssertType(x, "any");
AssertType(this, "this");
            return x => this;   // New scope.  So should inject new _this capture
        }
    }
    y(_super: number) { // Error 
        let lambda = () => {
AssertType(lambda, "() => (any) => this");
AssertType(() => {            return x => this;   // New scope.  So should inject new _this capture        }, "() => (any) => this");

AssertType(x => this, "(any) => this");
AssertType(x, "any");
AssertType(this, "this");
            return x => this;   // New scope.  So should inject new _this capture
        }
    }
    set z(_super: number) { // Error
    }
    public prop3: {
        doStuff: (_super: number) => void; // no error - no code gen
    }
    public prop4 = {
        doStuff: (_super: number) => { // should be error
        }
    }
    constructor(_super: number) { // should be error
        super();
AssertType(super(), "void");
AssertType(super, "typeof Foo");
    }
}
declare class Foo3 extends Foo {
    x();
    y(_super: number); // No error - no code gen
    constructor(_super: number); // No error - no code gen
    public prop2: {
        doStuff: (_super: number) => void; // no error - no code gen
    };
    public _super: number; // No error
}

class Foo4 extends Foo {
    constructor(_super: number); // no code gen - no error
    constructor(_super: string);// no code gen - no error
    constructor(_super: any) { // should be error
        super();
AssertType(super(), "void");
AssertType(super, "typeof Foo");
    }
    y(_super: number); // no code gen - no error
    y(_super: string); // no code gen - no error
    y(_super: any) { // Error 
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

