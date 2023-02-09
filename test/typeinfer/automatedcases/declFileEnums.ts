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

// === tests/cases/compiler/declFileEnums.ts ===
declare function AssertType(value:any, type:string):void;

enum e1 {
    a,
    b,
    c
}

enum e2 {
    a = 10,
    b = a + 2,
    c = 10,
}

enum e3 {
    a = 10,
    b = Math.PI,
    c = a + 3
}

enum e4 {
    a,
    b,
    c,
    d = 10,
    e
}

enum e5 {
    "Friday",
    "Saturday",
    "Sunday",
    "Weekend days"
}




