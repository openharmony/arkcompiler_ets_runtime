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

// === tests/cases/conformance/types/typeRelationships/recursiveTypes/nominalSubtypeCheckOfTypeParameter.ts ===
declare function AssertType(value:any, type:string):void;

interface BinaryTuple<T, S> {
    first: T
    second: S
}

interface Sequence<T> {
    hasNext(): boolean
    pop(): T
    zip<S>(seq: Sequence<S>): Sequence<BinaryTuple<T, S>>
}

// error, despite the fact that the code explicitly says List<T> extends Sequence<T>, the current rules for infinitely expanding type references 
// perform nominal subtyping checks that allow variance for type arguments, but not nominal subtyping for the generic type itself
interface List<T> extends Sequence<T> {
    getLength(): number
    zip<S>(seq: Sequence<S>): List<BinaryTuple<T, S>>
}


