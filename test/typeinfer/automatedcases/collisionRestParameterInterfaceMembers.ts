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

// === tests/cases/compiler/collisionRestParameterInterfaceMembers.ts ===
declare function AssertType(value:any, type:string):void;

// call
interface i1 {
    (_i: number, ...restParameters); // no error - no code gen
}
interface i1NoError {
    (_i: number);  // no error
}

// new
interface i2 {
    new (_i: number, ...restParameters); // no error - no code gen
}
interface i2NoError {
    new (_i: number);  // no error
}

// method
interface i3 {
    foo (_i: number, ...restParameters); // no error - no code gen
    fooNoError (_i: number);  // no error
}

