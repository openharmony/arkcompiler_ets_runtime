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

// === tests/cases/conformance/interfaces/declarationMerging/mergedInterfacesWithMultipleBases3.ts ===
declare function AssertType(value:any, type:string):void;

// merged interfaces behave as if all extends clauses from each declaration are merged together
// no errors expected

class C<T> {
    a: T;
}

class C2<T> {
    b: T;
}

class C3<T> {
    c: T;
}

class C4<T> {
    d: T;
}

interface A<T> extends C<string>, C3<string> {
    y: T;
}

interface A<T> extends C<string>, C4<string> {
    z: T;
}

class D implements A<boolean> {
    a: string;
    b: Date;
    c: string;
    d: string;
    y: boolean;
    z: boolean;
}

