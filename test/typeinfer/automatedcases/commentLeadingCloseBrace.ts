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

// === tests/cases/compiler/commentLeadingCloseBrace.ts ===
declare function AssertType(value:any, type:string):void;
declare function commentedParameters(...args): any;

function ifelse() {
    if (commentedParameters(1, 2)) {
AssertType(commentedParameters(1, 2), "any");
AssertType(commentedParameters, "(...any[]) => any");
AssertType(1, "int");
AssertType(2, "int");

        /*comment1*/
        commentedParameters(3, 4);
AssertType(commentedParameters(3, 4), "any");
AssertType(commentedParameters, "(...any[]) => any");
AssertType(3, "int");
AssertType(4, "int");

        /*comment2*/
    } else {
        commentedParameters(5, 6);
AssertType(commentedParameters(5, 6), "any");
AssertType(commentedParameters, "(...any[]) => any");
AssertType(5, "int");
AssertType(6, "int");
    }
}

