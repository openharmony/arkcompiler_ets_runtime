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

// === tests/cases/compiler/privacyAccessorDeclFile_externalModule.ts ===
declare function AssertType(value:any, type:string):void;
class privateClass {
}

export class publicClass {
}

export class publicClassWithWithPrivateGetAccessorTypes {
    static get myPublicStaticMethod(): privateClass { // Error
AssertType(null, "null");
        return null;
    }
    private static get myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    get myPublicMethod(): privateClass { // Error
AssertType(null, "null");
        return null;
    }
    private get myPrivateMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    static get myPublicStaticMethod1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    private static get myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    get myPublicMethod1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    private get myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
}

export class publicClassWithWithPublicGetAccessorTypes {
    static get myPublicStaticMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    private static get myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    get myPublicMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    private get myPrivateMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    static get myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    private static get myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    get myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    private get myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
}

class privateClassWithWithPrivateGetAccessorTypes {
    static get myPublicStaticMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    private static get myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    get myPublicMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    private get myPrivateMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    static get myPublicStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    private static get myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    get myPublicMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    private get myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
}

class privateClassWithWithPublicGetAccessorTypes {
    static get myPublicStaticMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    private static get myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    get myPublicMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    private get myPrivateMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    static get myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    private static get myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    get myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    private get myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
}

export class publicClassWithWithPrivateSetAccessorTypes {
    static set myPublicStaticMethod(param: privateClass) { // Error
    }
    private static set myPrivateStaticMethod(param: privateClass) {
    }
    set myPublicMethod(param: privateClass) { // Error
    }
    private set myPrivateMethod(param: privateClass) {
    }
}

export class publicClassWithWithPublicSetAccessorTypes {
    static set myPublicStaticMethod(param: publicClass) {
    }
    private static set myPrivateStaticMethod(param: publicClass) {
    }
    set myPublicMethod(param: publicClass) {
    }
    private set myPrivateMethod(param: publicClass) {
    }
}

class privateClassWithWithPrivateSetAccessorTypes {
    static set myPublicStaticMethod(param: privateClass) {
    }
    private static set myPrivateStaticMethod(param: privateClass) {
    }
    set myPublicMethod(param: privateClass) { 
    }
    private set myPrivateMethod(param: privateClass) {
    }
}

class privateClassWithWithPublicSetAccessorTypes {
    static set myPublicStaticMethod(param: publicClass) {
    }
    private static set myPrivateStaticMethod(param: publicClass) {
    }
    set myPublicMethod(param: publicClass) {
    }
    private set myPrivateMethod(param: publicClass) {
    }
}

export class publicClassWithPrivateModuleGetAccessorTypes {
    static get myPublicStaticMethod(): privateModule.publicClass { // Error
AssertType(null, "null");
        return null;
    }
    get myPublicMethod(): privateModule.publicClass { // Error
AssertType(null, "null");
        return null;
    }
    static get myPublicStaticMethod1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
    get myPublicMethod1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
}

export class publicClassWithPrivateModuleSetAccessorTypes {
    static set myPublicStaticMethod(param: privateModule.publicClass) { // Error
    }
    set myPublicMethod(param: privateModule.publicClass) { // Error
    }
}

class privateClassWithPrivateModuleGetAccessorTypes {
    static get myPublicStaticMethod(): privateModule.publicClass {
AssertType(null, "null");
        return null;
    }
    get myPublicMethod(): privateModule.publicClass {
AssertType(null, "null");
        return null;
    }
    static get myPublicStaticMethod1() {
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
    get myPublicMethod1() {
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
}

class privateClassWithPrivateModuleSetAccessorTypes {
    static set myPublicStaticMethod(param: privateModule.publicClass) {
    }
    set myPublicMethod(param: privateModule.publicClass) {
    }
}

export module publicModule {
    class privateClass {
    }

    export class publicClass {
    }
    export class publicClassWithWithPrivateGetAccessorTypes {
        static get myPublicStaticMethod(): privateClass { // Error
AssertType(null, "null");
            return null;
        }
        private static get myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): privateClass { // Error
AssertType(null, "null");
            return null;
        }
        private get myPrivateMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private static get myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        get myPublicMethod1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private get myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
    }

    export class publicClassWithWithPublicGetAccessorTypes {
        static get myPublicStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private static get myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private get myPrivateMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private static get myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        get myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private get myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
    }

    class privateClassWithWithPrivateGetAccessorTypes {
        static get myPublicStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        private static get myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        private get myPrivateMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private static get myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        get myPublicMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private get myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
    }

    class privateClassWithWithPublicGetAccessorTypes {
        static get myPublicStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private static get myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private get myPrivateMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private static get myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        get myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private get myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
    }

    export class publicClassWithWithPrivateSetAccessorTypes {
        static set myPublicStaticMethod(param: privateClass) { // Error
        }
        private static set myPrivateStaticMethod(param: privateClass) {
        }
        set myPublicMethod(param: privateClass) { // Error
        }
        private set myPrivateMethod(param: privateClass) {
        }
    }

    export class publicClassWithWithPublicSetAccessorTypes {
        static set myPublicStaticMethod(param: publicClass) {
        }
        private static set myPrivateStaticMethod(param: publicClass) {
        }
        set myPublicMethod(param: publicClass) {
        }
        private set myPrivateMethod(param: publicClass) {
        }
    }

    class privateClassWithWithPrivateSetAccessorTypes {
        static set myPublicStaticMethod(param: privateClass) {
        }
        private static set myPrivateStaticMethod(param: privateClass) {
        }
        set myPublicMethod(param: privateClass) {
        }
        private set myPrivateMethod(param: privateClass) {
        }
    }

    class privateClassWithWithPublicSetAccessorTypes {
        static set myPublicStaticMethod(param: publicClass) {
        }
        private static set myPrivateStaticMethod(param: publicClass) {
        }
        set myPublicMethod(param: publicClass) {
        }
        private set myPrivateMethod(param: publicClass) {
        }
    }

    export class publicClassWithPrivateModuleGetAccessorTypes {
        static get myPublicStaticMethod(): privateModule.publicClass { // Error
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): privateModule.publicClass { // Error
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
            return new privateModule.publicClass();
        }
        get myPublicMethod1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
            return new privateModule.publicClass();
        }
    }

    export class publicClassWithPrivateModuleSetAccessorTypes {
        static set myPublicStaticMethod(param: privateModule.publicClass) { // Error
        }
        set myPublicMethod(param: privateModule.publicClass) { // Error
        }
    }

    class privateClassWithPrivateModuleGetAccessorTypes {
        static get myPublicStaticMethod(): privateModule.publicClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): privateModule.publicClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() {
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
            return new privateModule.publicClass();
        }
        get myPublicMethod1() {
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
            return new privateModule.publicClass();
        }
    }

    class privateClassWithPrivateModuleSetAccessorTypes {
        static set myPublicStaticMethod(param: privateModule.publicClass) {
        }
        set myPublicMethod(param: privateModule.publicClass) {
        }
    }
}

module privateModule {
    class privateClass {
    }

    export class publicClass {
    }
    export class publicClassWithWithPrivateGetAccessorTypes {
        static get myPublicStaticMethod(): privateClass { 
AssertType(null, "null");
            return null;
        }
        private static get myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): privateClass { 
AssertType(null, "null");
            return null;
        }
        private get myPrivateMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() { 
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private static get myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        get myPublicMethod1() { 
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private get myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
    }

    export class publicClassWithWithPublicGetAccessorTypes {
        static get myPublicStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private static get myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private get myPrivateMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private static get myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        get myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private get myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
    }

    class privateClassWithWithPrivateGetAccessorTypes {
        static get myPublicStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        private static get myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        private get myPrivateMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private static get myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        get myPublicMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private get myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
    }

    class privateClassWithWithPublicGetAccessorTypes {
        static get myPublicStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private static get myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private get myPrivateMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private static get myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        get myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private get myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
    }

    export class publicClassWithWithPrivateSetAccessorTypes {
        static set myPublicStaticMethod(param: privateClass) { 
        }
        private static set myPrivateStaticMethod(param: privateClass) {
        }
        set myPublicMethod(param: privateClass) { 
        }
        private set myPrivateMethod(param: privateClass) {
        }
    }

    export class publicClassWithWithPublicSetAccessorTypes {
        static set myPublicStaticMethod(param: publicClass) {
        }
        private static set myPrivateStaticMethod(param: publicClass) {
        }
        set myPublicMethod(param: publicClass) {
        }
        private set myPrivateMethod(param: publicClass) {
        }
    }

    class privateClassWithWithPrivateSetAccessorTypes {
        static set myPublicStaticMethod(param: privateClass) {
        }
        private static set myPrivateStaticMethod(param: privateClass) {
        }
        set myPublicMethod(param: privateClass) {
        }
        private set myPrivateMethod(param: privateClass) {
        }
    }

    class privateClassWithWithPublicSetAccessorTypes {
        static set myPublicStaticMethod(param: publicClass) {
        }
        private static set myPrivateStaticMethod(param: publicClass) {
        }
        set myPublicMethod(param: publicClass) {
        }
        private set myPrivateMethod(param: publicClass) {
        }
    }

    export class publicClassWithPrivateModuleGetAccessorTypes {
        static get myPublicStaticMethod(): privateModule.publicClass { 
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): privateModule.publicClass { 
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() { 
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
            return new privateModule.publicClass();
        }
        get myPublicMethod1() { 
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
            return new privateModule.publicClass();
        }
    }

    export class publicClassWithPrivateModuleSetAccessorTypes {
        static set myPublicStaticMethod(param: privateModule.publicClass) { 
        }
        set myPublicMethod(param: privateModule.publicClass) { 
        }
    }

    class privateClassWithPrivateModuleGetAccessorTypes {
        static get myPublicStaticMethod(): privateModule.publicClass {
AssertType(null, "null");
            return null;
        }
        get myPublicMethod(): privateModule.publicClass {
AssertType(null, "null");
            return null;
        }
        static get myPublicStaticMethod1() {
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
            return new privateModule.publicClass();
        }
        get myPublicMethod1() {
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
            return new privateModule.publicClass();
        }
    }

    class privateClassWithPrivateModuleSetAccessorTypes {
        static set myPublicStaticMethod(param: privateModule.publicClass) {
        }
        set myPublicMethod(param: privateModule.publicClass) {
        }
    }
}


