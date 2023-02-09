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

// === tests/cases/compiler/varArgParamTypeCheck.ts ===
declare function AssertType(value:any, type:string):void;
function sequence(...sequences:{():void;}[]) {
}

function callback(clb:()=>void) {
}

sequence(
AssertType(sequence(    function bar() {    },    function foo() {        callback(()=>{            this();        });    },    function baz() {        callback(()=>{            this();        });    }), "void");
AssertType(sequence, "(...(() => void)[]) => void");

    function bar() {
AssertType(function bar() {    }, "() => void");
AssertType(bar, "() => void");

    },
    function foo() {
AssertType(function foo() {        callback(()=>{            this();        });    }, "() => void");
AssertType(foo, "() => void");

        callback(()=>{
AssertType(callback(()=>{            this();        }), "void");
AssertType(callback, "(() => void) => void");
AssertType(()=>{            this();        }, "() => void");

            this();
AssertType(this(), "any");
AssertType(this, "any");

        });
    },
    function baz() {
AssertType(function baz() {        callback(()=>{            this();        });    }, "() => void");
AssertType(baz, "() => void");

        callback(()=>{
AssertType(callback(()=>{            this();        }), "void");
AssertType(callback, "(() => void) => void");
AssertType(()=>{            this();        }, "() => void");

            this();
AssertType(this(), "any");
AssertType(this, "any");

        });
    }
);


