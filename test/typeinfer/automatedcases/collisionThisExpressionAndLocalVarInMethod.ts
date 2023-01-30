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

// === tests/cases/compiler/collisionThisExpressionAndLocalVarInMethod.ts ===
declare function AssertType(value:any, type:string):void;
class a {
    method1() {
AssertType({            doStuff: (callback) => () => {                let _this = 2;                return callback(this);            }        }, "{ doStuff: (any) => () => any; }");
        return {

            doStuff: (callback) => () => {
AssertType(doStuff, "(any) => () => any");
AssertType(callback, "any");
AssertType(() => {                let _this = 2;                return callback(this);            }, "() => any");
AssertType((callback) => () => {                let _this = 2;                return callback(this);            }, "(any) => () => any");

                let _this = 2;
AssertType(_this, "number");
AssertType(2, "int");

AssertType(callback(this), "any");
AssertType(callback, "any");
AssertType(this, "this");
                return callback(this);
            }
        }
    }
    method2() {
        let _this = 2;
AssertType(_this, "number");
AssertType(2, "int");

AssertType({            doStuff: (callback) => () => {                return callback(this);            }        }, "{ doStuff: (any) => () => any; }");
        return {

            doStuff: (callback) => () => {
AssertType(doStuff, "(any) => () => any");
AssertType(callback, "any");
AssertType(() => {                return callback(this);            }, "() => any");
AssertType((callback) => () => {                return callback(this);            }, "(any) => () => any");

AssertType(callback(this), "any");
AssertType(callback, "any");
AssertType(this, "this");
                return callback(this);
            }
        }
    }
}

