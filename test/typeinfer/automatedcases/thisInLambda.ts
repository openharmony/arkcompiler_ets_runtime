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

// === tests/cases/compiler/thisInLambda.ts ===
declare function AssertType(value:any, type:string):void;
class Foo {
    x = "hello";
    bar() {
        this.x; // 'this' is type 'Foo'
AssertType(this.x, "string");
AssertType(this, "this");

        let f = () => this.x; // 'this' should be type 'Foo' as well
AssertType(f, "() => string");
AssertType(() => this.x, "() => string");
AssertType(this.x, "string");
AssertType(this, "this");
    }
}

function myFn(a:any) { }
class myCls {
    constructor () {
        myFn(() => {
AssertType(myFn(() => {            myFn(() => {                let x = this;            });        }), "void");
AssertType(myFn, "(any) => void");
AssertType(() => {            myFn(() => {                let x = this;            });        }, "() => void");

            myFn(() => {
AssertType(myFn(() => {                let x = this;            }), "void");
AssertType(myFn, "(any) => void");
AssertType(() => {                let x = this;            }, "() => void");

                let x = this;
AssertType(x, "this");
AssertType(this, "this");

            });
        });
    }
}

