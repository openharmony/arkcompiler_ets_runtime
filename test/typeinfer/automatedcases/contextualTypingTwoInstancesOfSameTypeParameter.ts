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

// === tests/cases/compiler/contextualTypingTwoInstancesOfSameTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;
function f6<T>(x: (a: T) => T) {
AssertType(null, "null");
    return null;
} 
f6(x => f6(y => x = y));
AssertType(f6(x => f6(y => x = y)), "any");
AssertType(f6, "<T>((T) => T) => any");
AssertType(x => f6(y => x = y), "(unknown) => any");
AssertType(x, "unknown");
AssertType(f6(y => x = y), "any");
AssertType(f6, "<T>((T) => T) => any");
AssertType(y => x = y, "(unknown) => unknown");
AssertType(y, "unknown");
AssertType(x = y, "unknown");
AssertType(x, "unknown");
AssertType(y, "unknown");


