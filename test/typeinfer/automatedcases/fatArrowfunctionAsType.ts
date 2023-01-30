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

// === tests/cases/compiler/fatArrowfunctionAsType.ts ===
declare function AssertType(value:any, type:string):void;
declare let b: <T>(x: T) => void ;
AssertType(b, "<T>(T) => void");
AssertType(x, "T");

let c: <T>(x: T) => void = function <T>(x: T) { 
AssertType(c, "<T>(T) => void");
return 42; 

AssertType(x, "T");

AssertType(function <T>(x: T) { return 42; }, "<T>(T) => number");

AssertType(x, "T");

AssertType(42, "int");
}

b = c;
AssertType(b = c, "<T>(T) => void");
AssertType(b, "<T>(T) => void");
AssertType(c, "<T>(T) => void");


