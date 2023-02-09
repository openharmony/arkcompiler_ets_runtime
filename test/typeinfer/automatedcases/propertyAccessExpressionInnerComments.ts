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

// === tests/cases/compiler/propertyAccessExpressionInnerComments.ts ===
declare function AssertType(value:any, type:string):void;
/*1*/Array/*2*/./*3*/toString/*4*/
AssertType(Array/*2*/./*3*/toString, "() => string");

/*1*/Array
AssertType(Array/*2*/./*3*/    // Single-line comment    toString, "() => string");

/*2*/./*3*/
    // Single-line comment
    toString/*4*/

/*1*/Array/*2*/./*3*/
AssertType(Array/*2*/./*3*/    // Single-line comment    toString, "() => string");

    // Single-line comment
    toString/*4*/

/*1*/Array
AssertType(Array    // Single-line comment    /*2*/./*3*/toString, "() => string");

    // Single-line comment
    /*2*/./*3*/toString/*4*/

/* Existing issue: the "2" comments below are duplicated and "3"s are missing */

/*1*/Array/*2*/?./*3*/toString/*4*/
AssertType(Array/*2*/?./*3*/toString, "() => string");

/*1*/Array
AssertType(Array/*2*/?./*3*/    // Single-line comment    toString, "() => string");

/*2*/?./*3*/
    // Single-line comment
    toString/*4*/

/*1*/Array/*2*/?./*3*/
AssertType(Array/*2*/?./*3*/    // Single-line comment    toString, "() => string");

    // Single-line comment
    toString/*4*/

/*1*/Array
AssertType(Array    // Single-line comment    /*2*/?./*3*/toString, "() => string");

    // Single-line comment
    /*2*/?./*3*/toString/*4*/


