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

// === tests/cases/compiler/privacyInterfaceExtendsClauseDeclFile_externalModule.ts ===
declare function AssertType(value:any, type:string):void;

export module publicModule {
    export interface publicInterfaceInPublicModule {
    }

    interface privateInterfaceInPublicModule {
    }

    interface privateInterfaceImplementingPublicInterfaceInModule extends publicInterfaceInPublicModule {
    }
    interface privateInterfaceImplementingPrivateInterfaceInModule extends privateInterfaceInPublicModule {
    }
    export interface publicInterfaceImplementingPublicInterfaceInModule extends publicInterfaceInPublicModule {
    }
    export interface publicInterfaceImplementingPrivateInterfaceInModule extends privateInterfaceInPublicModule { // Should error
    }

    interface privateInterfaceImplementingFromPrivateModuleInterface extends privateModule.publicInterfaceInPrivateModule {
    }
    export interface publicInterfaceImplementingFromPrivateModuleInterface extends privateModule.publicInterfaceInPrivateModule { // Should error
    }

    export interface publicInterfaceImplementingPrivateAndPublicInterface extends privateInterfaceInPublicModule, publicInterfaceInPublicModule { // Should error
    }
}

module privateModule {
    export interface publicInterfaceInPrivateModule {

    }

    interface privateInterfaceInPrivateModule {
    }

    interface privateInterfaceImplementingPublicInterfaceInModule extends publicInterfaceInPrivateModule {
    }
    interface privateInterfaceImplementingPrivateInterfaceInModule extends privateInterfaceInPrivateModule {
    }
    export interface publicInterfaceImplementingPublicInterfaceInModule extends publicInterfaceInPrivateModule {
    }
    export interface publicInterfaceImplementingPrivateInterfaceInModule extends privateInterfaceInPrivateModule {
    }

    interface privateInterfaceImplementingFromPrivateModuleInterface extends privateModule.publicInterfaceInPrivateModule {
    }
    export interface publicInterfaceImplementingFromPrivateModuleInterface extends privateModule.publicInterfaceInPrivateModule {
    }
}

export interface publicInterface {

}

interface privateInterface {
}

interface privateInterfaceImplementingPublicInterface extends publicInterface {
}
interface privateInterfaceImplementingPrivateInterfaceInModule extends privateInterface {
}
export interface publicInterfaceImplementingPublicInterface extends publicInterface {
}
export interface publicInterfaceImplementingPrivateInterface extends privateInterface { // Should error
}

interface privateInterfaceImplementingFromPrivateModuleInterface extends privateModule.publicInterfaceInPrivateModule {
}
export interface publicInterfaceImplementingFromPrivateModuleInterface extends privateModule.publicInterfaceInPrivateModule { // Should error
}


