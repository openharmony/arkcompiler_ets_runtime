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

// === tests/cases/conformance/es6/computedProperties/computedPropertyNames52.js ===
declare function AssertType(value:any, type:string):void;
const array = [];
AssertType(array, "any[]");
AssertType([], "undefined[]");

for (let i = 0; i < 10; ++i) {
    array.push(class C {
AssertType(array.push(class C {        [i] = () => C;        static [i] = 100;    }), "number");
AssertType(array.push, "(...any[]) => number");
AssertType(class C {        [i] = () => C;        static [i] = 100;    }, "typeof C");
AssertType(C, "typeof C");

        [i] = () => C;
AssertType([i], "() => typeof C");
AssertType(i, "number");
AssertType(() => C, "() => typeof C");
AssertType(C, "typeof C");

        static [i] = 100;
AssertType([i], "number");
AssertType(i, "number");
AssertType(100, "int");

    })
}


