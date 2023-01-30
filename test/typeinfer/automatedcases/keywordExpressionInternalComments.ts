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

// === tests/cases/compiler/keywordExpressionInternalComments.ts ===
declare function AssertType(value:any, type:string):void;
/*1*/ new /*2*/ Array /*3*/;
AssertType(new /*2*/ Array, "any[]");
AssertType(Array, "ArrayConstructor");

/*1*/ typeof /*2*/ Array /*3*/;
AssertType(typeof /*2*/ Array, "union");
AssertType(Array, "ArrayConstructor");

/*1*/ void /*2*/ Array /*3*/;
AssertType(void /*2*/ Array, "undefined");
AssertType(Array, "ArrayConstructor");

/*1*/ delete /*2*/ Array.toString /*3*/;
AssertType(delete /*2*/ Array.toString, "boolean");
AssertType(Array.toString, "() => string");


