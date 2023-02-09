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

// === tests/cases/compiler/privacyTypeParametersOfInterfaceDeclFile.ts ===
declare function AssertType(value:any, type:string):void;

class privateClass {
}

export class publicClass {
}

class privateClassT<T> {
}

export class publicClassT<T> {
}

export interface publicInterfaceWithPrivateTypeParameters<T extends privateClass> { // Error
    myMethod(val: T): T;
    myMethod0(): publicClassT<T>;
    myMethod1(): privateClassT<privateClass>; 
    myMethod2(): privateClassT<publicClass>;
    myMethod3(): publicClassT<privateClass>;
    myMethod4(): publicClassT<publicClass>; 
}

export interface publicInterfaceWithPublicTypeParameters<T extends publicClass> {
    myMethod(val: T): T;
    myMethod0(): publicClassT<T>
    myMethod1(): privateClassT<privateClass>;
    myMethod2(): privateClassT<publicClass>;
    myMethod3(): publicClassT<privateClass>;
    myMethod4(): publicClassT<publicClass>;
}

interface privateInterfaceWithPrivateTypeParameters<T extends privateClass> {
    myMethod(val: T): T;
    myMethod0(): publicClassT<T>;
    myMethod1(): privateClassT<privateClass>;
    myMethod2(): privateClassT<publicClass>;
    myMethod3(): publicClassT<privateClass>;
    myMethod4(): publicClassT<publicClass>;
}

interface privateInterfaceWithPublicTypeParameters<T extends publicClass> {
    myMethod(val: T): T;
    myMethod0(): publicClassT<T>;
    myMethod1(): privateClassT<privateClass>;
    myMethod2(): privateClassT<publicClass>;
    myMethod3(): publicClassT<privateClass>;
    myMethod4(): publicClassT<publicClass>;
}

export interface publicInterfaceWithPublicTypeParametersWithoutExtends<T> {
    myMethod(val: T): T;
    myMethod0(): publicClassT<T>;
}

interface privateInterfaceWithPublicTypeParametersWithoutExtends<T> {
    myMethod(val: T): T;
    myMethod0(): publicClassT<T>;
}


export interface publicInterfaceWithPrivateModuleTypeParameterConstraints<T extends privateModule.publicClassInPrivateModule> { // Error
}

interface privateInterfaceWithPrivateModuleTypeParameterConstraints<T extends privateModule.publicClassInPrivateModule> { // Error
}

export module publicModule {
    class privateClassInPublicModule {
    }

    export class publicClassInPublicModule {
    }

    class privateClassInPublicModuleT<T> {
    }

    export class publicClassInPublicModuleT<T> {
    }

    export interface publicInterfaceWithPrivateTypeParameters<T extends privateClassInPublicModule> { // Error
        myMethod(val: T): T;
        myMethod0(): publicClassInPublicModuleT<T>;
        myMethod1(): privateClassInPublicModuleT<privateClassInPublicModule>;
        myMethod2(): privateClassInPublicModuleT<publicClassInPublicModule>;
        myMethod3(): publicClassInPublicModuleT<privateClassInPublicModule>;
        myMethod4(): publicClassInPublicModuleT<publicClassInPublicModule>;
    }

    export interface publicInterfaceWithPublicTypeParameters<T extends publicClassInPublicModule> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPublicModuleT<T>
        myMethod1(): privateClassInPublicModuleT<privateClassInPublicModule>;
        myMethod2(): privateClassInPublicModuleT<publicClassInPublicModule>;
        myMethod3(): publicClassInPublicModuleT<privateClassInPublicModule>;
        myMethod4(): publicClassInPublicModuleT<publicClassInPublicModule>;
    }

    interface privateInterfaceWithPrivateTypeParameters<T extends privateClassInPublicModule> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPublicModuleT<T>;
        myMethod1(): privateClassInPublicModuleT<privateClassInPublicModule>;
        myMethod2(): privateClassInPublicModuleT<publicClassInPublicModule>;
        myMethod3(): publicClassInPublicModuleT<privateClassInPublicModule>;
        myMethod4(): publicClassInPublicModuleT<publicClassInPublicModule>;
    }

    interface privateInterfaceWithPublicTypeParameters<T extends publicClassInPublicModule> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPublicModuleT<T>;
        myMethod1(): privateClassInPublicModuleT<privateClassInPublicModule>;
        myMethod2(): privateClassInPublicModuleT<publicClassInPublicModule>;
        myMethod3(): publicClassInPublicModuleT<privateClassInPublicModule>;
        myMethod4(): publicClassInPublicModuleT<publicClassInPublicModule>;
    }

    export interface publicInterfaceWithPublicTypeParametersWithoutExtends<T> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPublicModuleT<T>;
    }

    interface privateInterfaceWithPublicTypeParametersWithoutExtends<T> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPublicModuleT<T>;
    }

    export interface publicInterfaceWithPrivateModuleTypeParameterConstraints<T extends privateModule.publicClassInPrivateModule> { // Error
    }

    interface privateInterfaceWithPrivateModuleTypeParameterConstraints<T extends privateModule.publicClassInPrivateModule> { // Error
    }
}

module privateModule {
    class privateClassInPrivateModule {
    }

    export class publicClassInPrivateModule {
    }

    class privateClassInPrivateModuleT<T> {
    }

    export class publicClassInPrivateModuleT<T> {
    }

    export interface publicInterfaceWithPrivateTypeParameters<T extends privateClassInPrivateModule> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPrivateModuleT<T>;
        myMethod1(): privateClassInPrivateModuleT<privateClassInPrivateModule>;
        myMethod2(): privateClassInPrivateModuleT<publicClassInPrivateModule>;
        myMethod3(): publicClassInPrivateModuleT<privateClassInPrivateModule>;
        myMethod4(): publicClassInPrivateModuleT<publicClassInPrivateModule>;
    }

    export interface publicInterfaceWithPublicTypeParameters<T extends publicClassInPrivateModule> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPrivateModuleT<T>
        myMethod1(): privateClassInPrivateModuleT<privateClassInPrivateModule>;
        myMethod2(): privateClassInPrivateModuleT<publicClassInPrivateModule>;
        myMethod3(): publicClassInPrivateModuleT<privateClassInPrivateModule>;
        myMethod4(): publicClassInPrivateModuleT<publicClassInPrivateModule>;
    }

    interface privateInterfaceWithPrivateTypeParameters<T extends privateClassInPrivateModule> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPrivateModuleT<T>;
        myMethod1(): privateClassInPrivateModuleT<privateClassInPrivateModule>;
        myMethod2(): privateClassInPrivateModuleT<publicClassInPrivateModule>;
        myMethod3(): publicClassInPrivateModuleT<privateClassInPrivateModule>;
        myMethod4(): publicClassInPrivateModuleT<publicClassInPrivateModule>;
    }

    interface privateInterfaceWithPublicTypeParameters<T extends publicClassInPrivateModule> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPrivateModuleT<T>;
        myMethod1(): privateClassInPrivateModuleT<privateClassInPrivateModule>;
        myMethod2(): privateClassInPrivateModuleT<publicClassInPrivateModule>;
        myMethod3(): publicClassInPrivateModuleT<privateClassInPrivateModule>;
        myMethod4(): publicClassInPrivateModuleT<publicClassInPrivateModule>;
    }

    export interface publicInterfaceWithPublicTypeParametersWithoutExtends<T> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPrivateModuleT<T>;
    }

    interface privateInterfaceWithPublicTypeParametersWithoutExtends<T> {
        myMethod(val: T): T;
        myMethod0(): publicClassInPrivateModuleT<T>;
    }
}

