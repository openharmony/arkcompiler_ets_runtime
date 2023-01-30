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

// === tests/cases/conformance/classes/members/privateNames/privateNameClassExpressionLoop.ts ===
declare function AssertType(value:any, type:string):void;
const array = [];
AssertType(array, "any[]");
AssertType([], "undefined[]");

for (let i = 0; i < 10; ++i) {
    array.push(class C {
AssertType(array.push, "(...any[]) => number");
AssertType(class C {        #myField = "hello";        #method() {}        get #accessor() { return 42; }        set #accessor(val) { }    }, "typeof C");
AssertType(C, "typeof C");
AssertType(array.push(class C {        #myField = "hello";        #method() {}        get #accessor() { return 42; }        set #accessor(val) { }    }), "number");

        #myField = "hello";
AssertType(#myField, "string");
AssertType("hello", "string");

        #method() {
AssertType(#method, "() => void");
}

        get #accessor() { 
AssertType(#accessor, "number");
return 42; 

AssertType(42, "int");
}

        set #accessor(val) { 
AssertType(#accessor, "number");

AssertType(val, "number");
}

    });
}


