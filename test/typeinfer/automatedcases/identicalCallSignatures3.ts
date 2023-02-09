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

// === tests/cases/conformance/types/objectTypeLiteral/callSignatures/identicalCallSignatures3.ts ===
declare function AssertType(value:any, type:string):void;

// Normally it is an error to have multiple overloads with identical signatures in a single type declaration.
// Here the multiple overloads come from multiple merged declarations, so we do not report errors.

interface I {
    (x: number): string;
}

interface I {
    (x: number): string;
}

interface I2<T> {
    (x: number): string;
}

interface I2<T> {
    (x: number): string;
}

