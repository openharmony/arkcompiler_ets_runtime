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

// === tests/cases/compiler/taggedPrimitiveNarrowing.ts ===
declare function AssertType(value:any, type:string):void;
type Hash = string & { __hash: true };

function getHashLength(hash: Hash): number {
    if (typeof hash !== "string") {
AssertType(typeof hash !== "string", "boolean");
AssertType(typeof hash, "union");
AssertType(hash, "Hash");
AssertType("string", "string");

        throw new Error("This doesn't look like a hash");
AssertType(new Error("This doesn't look like a hash"), "Error");
AssertType(Error, "ErrorConstructor");
AssertType("This doesn't look like a hash", "string");
    }
AssertType(hash.length, "number");
    return hash.length;
}

function getHashLength2<T extends { __tag__: unknown}>(hash: string & T): number {
    if (typeof hash !== "string") {
AssertType(typeof hash !== "string", "boolean");
AssertType(typeof hash, "union");
AssertType(hash, "string & T");
AssertType("string", "string");

        throw new Error("This doesn't look like a hash");
AssertType(new Error("This doesn't look like a hash"), "Error");
AssertType(Error, "ErrorConstructor");
AssertType("This doesn't look like a hash", "string");
    }
AssertType(hash.length, "number");
    return hash.length;
}


