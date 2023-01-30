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

// === tests/cases/compiler/privacyTypeParametersOfInterface.ts ===
declare function AssertType(value:any, type:string):void;

class privateClass {
}

export class publicClass {
}

class privateClassT<T> {
}

export class publicClassT<T> {
}

// TypeParameter_0_of_exported_interface_1_has_or_is_using_private_type_2
export interface publicInterfaceWithPrivateTypeParameters<T extends privateClass> {
    myMethod(val: T): T; // Error
    myMethod0(): publicClassT<T>; // error
    myMethod1(): privateClassT<privateClass>; // error
    myMethod2(): privateClassT<publicClass>; // error
    myMethod3(): publicClassT<privateClass>; //error
    myMethod4(): publicClassT<publicClass>; // no error
}

export interface publicInterfaceWithPublicTypeParameters<T extends publicClass> {
    myMethod(val: T): T; // No Error
    myMethod0(): publicClassT<T>; // No error
    myMethod1(): privateClassT<privateClass>; // error
    myMethod2(): privateClassT<publicClass>; // error
    myMethod3(): publicClassT<privateClass>; //error
    myMethod4(): publicClassT<publicClass>; // no error
}

interface privateInterfaceWithPrivateTypeParameters<T extends privateClass> {
    myMethod(val: T): T; // No Error
    myMethod0(): publicClassT<T>; // No error
    myMethod1(): privateClassT<privateClass>; // No error
    myMethod2(): privateClassT<publicClass>; // No error
    myMethod3(): publicClassT<privateClass>; //No error
    myMethod4(): publicClassT<publicClass>; // no error
}

interface privateInterfaceWithPublicTypeParameters<T extends publicClass> {
    myMethod(val: T): T; // No Error
    myMethod0(): publicClassT<T>; // No error
    myMethod1(): privateClassT<privateClass>; // No error
    myMethod2(): privateClassT<publicClass>; // No error
    myMethod3(): publicClassT<privateClass>; //No error
    myMethod4(): publicClassT<publicClass>; // no error
}

export interface publicInterfaceWithPublicTypeParametersWithoutExtends<T> {
    myMethod(val: T): T; // No Error
    myMethod0(): publicClassT<T>; // No error
}

interface privateInterfaceWithPublicTypeParametersWithoutExtends<T> {
    myMethod(val: T): T; // No Error
    myMethod0(): publicClassT<T>; // No error
}

