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

// === tests/cases/conformance/jsdoc/typeParameterExtendsUnionConstraintDistributed.ts ===
declare function AssertType(value:any, type:string):void;
type A = 1 | 2;
function f<T extends A>(a: T): A & T { 
AssertType(a, "T");
return a; } // Shouldn't error

type B = 2 | 3;
function f2<T extends A, U extends B>(ab: T & U): (A | B) & T & U { 
AssertType(ab, "T & U");
return ab; } // Also shouldn't error


