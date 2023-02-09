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

// === tests/cases/compiler/privacyVarDeclFile_GlobalFile.ts ===
declare function AssertType(value:any, type:string):void;
class publicClassInGlobal {
}
interface publicInterfaceWithPublicPropertyTypesInGlobal {
    myProperty: publicClassInGlobal;
}
class publicClassWithWithPublicPropertyTypesInGlobal {
    static myPublicStaticProperty: publicClassInGlobal;
    private static myPrivateStaticProperty: publicClassInGlobal;
    myPublicProperty: publicClassInGlobal;
    private myPrivateProperty: publicClassInGlobal;
}
let publicVarWithPublicPropertyTypesInGlobal: publicClassInGlobal;
AssertType(publicVarWithPublicPropertyTypesInGlobal, "publicClassInGlobal");

declare let publicAmbientVarWithPublicPropertyTypesInGlobal: publicClassInGlobal;
AssertType(publicAmbientVarWithPublicPropertyTypesInGlobal, "publicClassInGlobal");

module publicModuleInGlobal {
    class privateClass {
    }

    export class publicClass {
    }

    module privateModule {
        class privateClass {
        }

        export class publicClass {
        }

        export interface publicInterfaceWithPrivatePropertyTypes {
            myProperty: privateClass;
        }

        export interface publicInterfaceWithPublicPropertyTypes {
            myProperty: publicClass;
        }

        interface privateInterfaceWithPrivatePropertyTypes {
            myProperty: privateClass;
        }

        interface privateInterfaceWithPublicPropertyTypes {
            myProperty: publicClass;
        }

        export class publicClassWithWithPrivatePropertyTypes {
            static myPublicStaticProperty: privateClass;
            private static myPrivateStaticProperty: privateClass;
            myPublicProperty: privateClass;
            private myPrivateProperty: privateClass;
        }

        export class publicClassWithWithPublicPropertyTypes {
            static myPublicStaticProperty: publicClass;
            private static myPrivateStaticProperty: publicClass;
            myPublicProperty: publicClass;
            private myPrivateProperty: publicClass;
        }

        class privateClassWithWithPrivatePropertyTypes {
            static myPublicStaticProperty: privateClass;
            private static myPrivateStaticProperty: privateClass;
            myPublicProperty: privateClass;
            private myPrivateProperty: privateClass;
        }

        class privateClassWithWithPublicPropertyTypes {
            static myPublicStaticProperty: publicClass;
            private static myPrivateStaticProperty: publicClass;
            myPublicProperty: publicClass;
            private myPrivateProperty: publicClass;
        }

        export let publicVarWithPrivatePropertyTypes: privateClass;
        export let publicVarWithPublicPropertyTypes: publicClass;
        let privateVarWithPrivatePropertyTypes: privateClass;
        let privateVarWithPublicPropertyTypes: publicClass;

        export declare let publicAmbientVarWithPrivatePropertyTypes: privateClass;
        export declare let publicAmbientVarWithPublicPropertyTypes: publicClass;
        declare let privateAmbientVarWithPrivatePropertyTypes: privateClass;
        declare let privateAmbientVarWithPublicPropertyTypes: publicClass;

        export interface publicInterfaceWithPrivateModulePropertyTypes {
            myProperty: privateModule.publicClass;
        }
        export class publicClassWithPrivateModulePropertyTypes {
            static myPublicStaticProperty: privateModule.publicClass;
            myPublicProperty: privateModule.publicClass;
        }
        export let publicVarWithPrivateModulePropertyTypes: privateModule.publicClass;
        export declare let publicAmbientVarWithPrivateModulePropertyTypes: privateModule.publicClass;

        interface privateInterfaceWithPrivateModulePropertyTypes {
            myProperty: privateModule.publicClass;
        }
        class privateClassWithPrivateModulePropertyTypes {
            static myPublicStaticProperty: privateModule.publicClass;
            myPublicProperty: privateModule.publicClass;
        }
        let privateVarWithPrivateModulePropertyTypes: privateModule.publicClass;
        declare let privateAmbientVarWithPrivateModulePropertyTypes: privateModule.publicClass;
    }

    export interface publicInterfaceWithPrivatePropertyTypes {
        myProperty: privateClass;  // Error
    }

    export interface publicInterfaceWithPublicPropertyTypes {
        myProperty: publicClass;
    }

    interface privateInterfaceWithPrivatePropertyTypes {
        myProperty: privateClass;
    }

    interface privateInterfaceWithPublicPropertyTypes {
        myProperty: publicClass;
    }

    export class publicClassWithWithPrivatePropertyTypes {
        static myPublicStaticProperty: privateClass; // Error
        private static myPrivateStaticProperty: privateClass;
        myPublicProperty: privateClass; // Error
        private myPrivateProperty: privateClass;
    }

    export class publicClassWithWithPublicPropertyTypes {
        static myPublicStaticProperty: publicClass;
        private static myPrivateStaticProperty: publicClass;
        myPublicProperty: publicClass;
        private myPrivateProperty: publicClass;
    }

    class privateClassWithWithPrivatePropertyTypes {
        static myPublicStaticProperty: privateClass;
        private static myPrivateStaticProperty: privateClass;
        myPublicProperty: privateClass;
        private myPrivateProperty: privateClass;
    }

    class privateClassWithWithPublicPropertyTypes {
        static myPublicStaticProperty: publicClass;
        private static myPrivateStaticProperty: publicClass;
        myPublicProperty: publicClass;
        private myPrivateProperty: publicClass;
    }

    export let publicVarWithPrivatePropertyTypes: privateClass; // Error
    export let publicVarWithPublicPropertyTypes: publicClass;
    let privateVarWithPrivatePropertyTypes: privateClass;
    let privateVarWithPublicPropertyTypes: publicClass;

    export declare let publicAmbientVarWithPrivatePropertyTypes: privateClass; // Error
    export declare let publicAmbientVarWithPublicPropertyTypes: publicClass;
    declare let privateAmbientVarWithPrivatePropertyTypes: privateClass;
    declare let privateAmbientVarWithPublicPropertyTypes: publicClass;

    export interface publicInterfaceWithPrivateModulePropertyTypes {
        myProperty: privateModule.publicClass; // Error
    }
    export class publicClassWithPrivateModulePropertyTypes {
        static myPublicStaticProperty: privateModule.publicClass; // Error
        myPublicProperty: privateModule.publicClass; // Error
    }
    export let publicVarWithPrivateModulePropertyTypes: privateModule.publicClass; // Error
    export declare let publicAmbientVarWithPrivateModulePropertyTypes: privateModule.publicClass; // Error

    interface privateInterfaceWithPrivateModulePropertyTypes {
        myProperty: privateModule.publicClass;
    }
    class privateClassWithPrivateModulePropertyTypes {
        static myPublicStaticProperty: privateModule.publicClass;
        myPublicProperty: privateModule.publicClass;
    }
    let privateVarWithPrivateModulePropertyTypes: privateModule.publicClass;
    declare let privateAmbientVarWithPrivateModulePropertyTypes: privateModule.publicClass;
}

