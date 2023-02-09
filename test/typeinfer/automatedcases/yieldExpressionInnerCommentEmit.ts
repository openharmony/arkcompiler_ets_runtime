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

// === tests/cases/compiler/yieldExpressionInnerCommentEmit.ts ===
declare function AssertType(value:any, type:string):void;
function * foo2() {
    /*comment1*/ yield 1;
AssertType(yield 1, "any");
AssertType(1, "int");

    yield /*comment2*/ 2;
AssertType(yield /*comment2*/ 2, "any");
AssertType(2, "int");

    yield 3 /*comment3*/
AssertType(yield 3, "any");
AssertType(3, "int");

    yield */*comment4*/ [4];
AssertType(yield */*comment4*/ [4], "any");
AssertType([4], "number[]");
AssertType(4, "int");

    yield /*comment5*/* [5];
AssertType(yield /*comment5*/* [5], "any");
AssertType([5], "number[]");
AssertType(5, "int");
}


