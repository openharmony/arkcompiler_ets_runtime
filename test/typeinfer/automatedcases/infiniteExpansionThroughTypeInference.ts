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

// === tests/cases/conformance/types/typeRelationships/recursiveTypes/infiniteExpansionThroughTypeInference.ts ===
declare function AssertType(value:any, type:string):void;
interface G<T> {
    x: G<G<T>> // infinitely expanding type reference
    y: T
}

function ff<T>(g: G<T>): void {
    ff(g) // when infering T here we need to make sure to not descend into the structure of G<T> infinitely
AssertType(ff(g), "void");
AssertType(ff, "<T>(G<T>) => void");
AssertType(g, "G<T>");
}



