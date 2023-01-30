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

// === tests/cases/conformance/classes/classExpressions/modifierOnClassExpressionMemberInFunction.ts ===
declare function AssertType(value:any, type:string):void;
function g() {
    let x = class C {
AssertType(x, "typeof C");
AssertType(class C {        public prop1 = 1;        private foo() { }        static prop2 = 43;    }, "typeof C");
AssertType(C, "typeof C");

        public prop1 = 1;
AssertType(prop1, "number");
AssertType(1, "int");

        private foo() { 
AssertType(foo, "() => void");
}

        static prop2 = 43;
AssertType(prop2, "number");
AssertType(43, "int");
    }
}

