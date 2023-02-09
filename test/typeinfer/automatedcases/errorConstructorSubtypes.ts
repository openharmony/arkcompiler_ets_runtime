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

// === tests/cases/compiler/errorConstructorSubtypes.ts ===
declare function AssertType(value:any, type:string):void;
// In Node, ErrorConstructor is augmented with extra properties. Excerpted below.
interface ErrorConstructor {
  captureStackTrace(targetObject: Object, constructorOpt?: Function): void;
}

declare let x: ErrorConstructor
AssertType(x, "ErrorConstructor");

x = Error; // OK
AssertType(x = Error, "ErrorConstructor");
AssertType(x, "ErrorConstructor");
AssertType(Error, "ErrorConstructor");

x = RangeError;
AssertType(x = RangeError, "RangeErrorConstructor");
AssertType(x, "ErrorConstructor");
AssertType(RangeError, "RangeErrorConstructor");

new x().message
AssertType(new x().message, "string");

x.captureStackTrace
AssertType(x.captureStackTrace, "(Object, ?Function) => void");


