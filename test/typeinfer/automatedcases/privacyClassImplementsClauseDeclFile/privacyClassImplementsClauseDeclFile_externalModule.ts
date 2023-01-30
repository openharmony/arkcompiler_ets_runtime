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

// === tests/cases/compiler/privacyClassImplementsClauseDeclFile_externalModule.ts ===
declare function AssertType(value:any, type:string):void;

export module publicModule {
    export interface publicInterfaceInPublicModule {
    }

    interface privateInterfaceInPublicModule {
    }

    class privateClassImplementingPublicInterfaceInModule implements publicInterfaceInPublicModule {
    }
    class privateClassImplementingPrivateInterfaceInModule implements privateInterfaceInPublicModule {
    }
    export class publicClassImplementingPublicInterfaceInModule implements publicInterfaceInPublicModule {
    }
    export class publicClassImplementingPrivateInterfaceInModule implements privateInterfaceInPublicModule { // Should error
    }

    class privateClassImplementingFromPrivateModuleInterface implements privateModule.publicInterfaceInPrivateModule {
    }
    export class publicClassImplementingFromPrivateModuleInterface implements privateModule.publicInterfaceInPrivateModule { // Should error
    }

    export class publicClassImplementingPrivateAndPublicInterface implements privateInterfaceInPublicModule, publicInterfaceInPublicModule { // Should error
    }
}

module privateModule {
    export interface publicInterfaceInPrivateModule {

    }

    interface privateInterfaceInPrivateModule {
    }

    class privateClassImplementingPublicInterfaceInModule implements publicInterfaceInPrivateModule {
    }
    class privateClassImplementingPrivateInterfaceInModule implements privateInterfaceInPrivateModule {
    }
    export class publicClassImplementingPublicInterfaceInModule implements publicInterfaceInPrivateModule {
    }
    export class publicClassImplementingPrivateInterfaceInModule implements privateInterfaceInPrivateModule { 
    }

    class privateClassImplementingFromPrivateModuleInterface implements privateModule.publicInterfaceInPrivateModule {
    }
    export class publicClassImplementingFromPrivateModuleInterface implements privateModule.publicInterfaceInPrivateModule {
    }
}

export interface publicInterface {

}

interface privateInterface {
}

class privateClassImplementingPublicInterface implements publicInterface {
}
class privateClassImplementingPrivateInterfaceInModule implements privateInterface {
}
export class publicClassImplementingPublicInterface implements publicInterface {
}
export class publicClassImplementingPrivateInterface implements privateInterface { // Should error
}

class privateClassImplementingFromPrivateModuleInterface implements privateModule.publicInterfaceInPrivateModule {
}
export class publicClassImplementingFromPrivateModuleInterface implements privateModule.publicInterfaceInPrivateModule { // Should error
}


