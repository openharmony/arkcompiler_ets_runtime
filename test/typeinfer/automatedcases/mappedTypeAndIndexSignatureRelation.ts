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

// === tests/cases/compiler/mappedTypeAndIndexSignatureRelation.ts ===
declare function AssertType(value:any, type:string):void;

type Same<T> = { [P in keyof T]: T[P] };

type T1<T extends Record<PropertyKey, number>> = T;
type T2<U extends Record<PropertyKey, number>> = T1<Same<U>>;

// Repro from #38235

type Foo<IdentifierT extends Record<PropertyKey, PropertyKey>> =
    IdentifierT
;

type Bar<IdentifierT extends Record<PropertyKey, PropertyKey>, T> =
    {
        [k in keyof T] : Foo<IdentifierT & { k : k }>
    }
;

type Merge2<T> = { [k in keyof T] : T[k] }
type Bar2<IdentifierT extends Record<PropertyKey, PropertyKey>, T> =
    {
        [k in keyof T]: Foo<Merge2<IdentifierT & { k: k }>>
    }
;

type Identity<T> = T;
type Merge3<T> = Identity<{ [k in keyof T] : T[k] }>
type Bar3<IdentifierT extends Record<PropertyKey, PropertyKey>, T> =
    {
        [k in keyof T]: Foo<Merge3<IdentifierT & { k: k }>>
    }
;


