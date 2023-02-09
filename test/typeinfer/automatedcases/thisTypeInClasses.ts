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

// === tests/cases/conformance/types/thisType/thisTypeInClasses.ts ===
declare function AssertType(value:any, type:string):void;
class C1 {
    x: this;
    f(x: this): this { 
AssertType(undefined, "undefined");
return undefined; 
}
}

class C2 {
    [x: string]: this;
}

interface Foo<T> {
    x: T;
    y: this;
}

class C3 {
    a: this[];
    b: [this, this];
    c: this | Date;
    d: this & Date;
    e: (((this)));
    f: (x: this) => this;
    g: new (x: this) => this;
    h: Foo<this>;
    i: Foo<this | (() => this)>;
    j: (x: any) => x is this;
}

declare class C4 {
    x: this;
    f(x: this): this;
}

class C5 {
    foo() {
        let f1 = (x: this): this => this;
AssertType(f1, "(this) => this");
AssertType((x: this): this => this, "(this) => this");
AssertType(x, "this");
AssertType(this, "this");

        let f2 = (x: this) => this;
AssertType(f2, "(this) => this");
AssertType((x: this) => this, "(this) => this");
AssertType(x, "this");
AssertType(this, "this");

        let f3 = (x: this) => (y: this) => this;
AssertType(f3, "(this) => (this) => this");
AssertType((x: this) => (y: this) => this, "(this) => (this) => this");
AssertType(x, "this");
AssertType((y: this) => this, "(this) => this");
AssertType(y, "this");
AssertType(this, "this");

        let f4 = (x: this) => {
AssertType(f4, "(this) => () => this");
AssertType(x, "this");
AssertType((x: this) => {            let g = (y: this) => {                return () => this;            }            return g(this);        }, "(this) => () => this");

            let g = (y: this) => {
AssertType(g, "(this) => () => this");
AssertType(y, "this");
AssertType((y: this) => {                return () => this;            }, "(this) => () => this");

AssertType(() => this, "() => this");
AssertType(this, "this");
                return () => this;
            }
AssertType(g(this), "() => this");
AssertType(g, "(this) => () => this");
AssertType(this, "this");
            return g(this);
        }
    }
    bar() {
        let x1 = <this>undefined;
AssertType(x1, "this");
AssertType(<this>undefined, "this");
AssertType(undefined, "undefined");

        let x2 = undefined as this;
AssertType(x2, "this");
AssertType(undefined as this, "this");
AssertType(undefined, "undefined");
    }
}


