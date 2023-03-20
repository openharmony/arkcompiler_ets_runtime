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

// === tests/cases/compiler/partialTypeNarrowedToByTypeGuard.ts ===
declare function AssertType(value:any, type:string):void;
type Obj = {} | undefined;

type User = {
    email: string;
    name: string;
};

type PartialUser = Partial<User>;

function isUser(obj: Obj): obj is PartialUser {
AssertType(true, "boolean");
    return true;
}

function getUserName(obj: Obj) {
    if (isUser(obj)) {
AssertType(isUser(obj), "boolean");
AssertType(isUser, "(Obj) => obj is Partial<User>");
AssertType(obj, "Obj");

AssertType(obj.name, "union");
        return obj.name;
    }

AssertType('', "string");
    return '';
}

