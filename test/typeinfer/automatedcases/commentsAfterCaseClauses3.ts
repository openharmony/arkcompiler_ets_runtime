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

// === tests/cases/compiler/commentsAfterCaseClauses3.ts ===
declare function AssertType(value:any, type:string):void;
function getSecurity(level) {
    switch(level){
AssertType(level, "any");

        case 0: /*Zero*/
AssertType(0, "int");

        case 1: /*One*/ 
AssertType(1, "int");

        case 2: /*two*/
AssertType(2, "int");

            // Leading comments
AssertType("Hi", "string");
            return "Hi";

        case 3: /*three*/
AssertType(3, "int");

        case 4: /*four*/
AssertType(4, "int");

AssertType("hello", "string");
            return "hello";

        case 5: /*five*/
AssertType(5, "int");

        default:  /*six*/
AssertType("world", "string");
            return "world";
    }
    
}

