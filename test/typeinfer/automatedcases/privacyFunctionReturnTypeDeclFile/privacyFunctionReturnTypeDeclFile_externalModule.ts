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

// === tests/cases/compiler/privacyFunctionReturnTypeDeclFile_externalModule.ts ===
declare function AssertType(value:any, type:string):void;
class privateClass {
}

export class publicClass {
}

export interface publicInterfaceWithPrivateParmeterTypes {
    new (): privateClass; // Error
    (): privateClass; // Error
    [x: number]: privateClass; // Error
    myMethod(): privateClass; // Error
}

export interface publicInterfaceWithPublicParmeterTypes {
    new (): publicClass;
    (): publicClass;
    [x: number]: publicClass;
    myMethod(): publicClass;
}

interface privateInterfaceWithPrivateParmeterTypes {
    new (): privateClass;
    (): privateClass;
    [x: number]: privateClass;
    myMethod(): privateClass;
}

interface privateInterfaceWithPublicParmeterTypes {
    new (): publicClass;
    (): publicClass;
    [x: number]: publicClass;
    myMethod(): publicClass;
}

export class publicClassWithWithPrivateParmeterTypes {
    static myPublicStaticMethod(): privateClass { // Error
AssertType(null, "null");
        return null;
    }
    private static myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    myPublicMethod(): privateClass { // Error
AssertType(null, "null");
        return null;
    }
    private myPrivateMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    static myPublicStaticMethod1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    private static myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    myPublicMethod1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    private myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
}

export class publicClassWithWithPublicParmeterTypes {
    static myPublicStaticMethod(): publicClass { 
AssertType(null, "null");
        return null;
    }
    private static myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    myPublicMethod(): publicClass { 
AssertType(null, "null");
        return null;
    }
    private myPrivateMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    static myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    private static myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    myPublicMethod1() { 
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    private myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
}

class privateClassWithWithPrivateParmeterTypes {
    static myPublicStaticMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    private static myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    myPublicMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    private myPrivateMethod(): privateClass {
AssertType(null, "null");
        return null;
    }
    static myPublicStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    private static myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    myPublicMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    private myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
}

class privateClassWithWithPublicParmeterTypes {
    static myPublicStaticMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    private static myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    myPublicMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    private myPrivateMethod(): publicClass {
AssertType(null, "null");
        return null;
    }
    static myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    private static myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    private myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
}

export function publicFunctionWithPrivateParmeterTypes(): privateClass { // Error
AssertType(null, "null");
    return null;
}
export function publicFunctionWithPublicParmeterTypes(): publicClass {
AssertType(null, "null");
    return null;
}
function privateFunctionWithPrivateParmeterTypes(): privateClass {
AssertType(null, "null");
    return null;
}
function privateFunctionWithPublicParmeterTypes(): publicClass {
AssertType(null, "null");
    return null;
}
export function publicFunctionWithPrivateParmeterTypes1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
    return new privateClass();
}
export function publicFunctionWithPublicParmeterTypes1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
    return new publicClass();
}
function privateFunctionWithPrivateParmeterTypes1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
    return new privateClass();
}
function privateFunctionWithPublicParmeterTypes1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
    return new publicClass();
}

export declare function publicAmbientFunctionWithPrivateParmeterTypes(): privateClass; // Error
export declare function publicAmbientFunctionWithPublicParmeterTypes(): publicClass;
declare function privateAmbientFunctionWithPrivateParmeterTypes(): privateClass;
declare function privateAmbientFunctionWithPublicParmeterTypes(): publicClass;

export interface publicInterfaceWithPrivateModuleParameterTypes {
    new (): privateModule.publicClass; // Error
    (): privateModule.publicClass; // Error
    [x: number]: privateModule.publicClass // Error
    myMethod(): privateModule.publicClass; // Error
}
export class publicClassWithPrivateModuleParameterTypes {
    static myPublicStaticMethod(): privateModule.publicClass { // Error
AssertType(null, "null");
        return null;
    }
    myPublicMethod(): privateModule.publicClass { // Error
AssertType(null, "null");
        return null;
    }
    static myPublicStaticMethod1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
    myPublicMethod1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
}
export function publicFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass { // Error
AssertType(null, "null");
    return null;
}
export function publicFunctionWithPrivateModuleParameterTypes1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
    return new privateModule.publicClass();
}
export declare function publicAmbientFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass; // Error

interface privateInterfaceWithPrivateModuleParameterTypes {
    new (): privateModule.publicClass; 
    (): privateModule.publicClass;
    [x: number]: privateModule.publicClass
    myMethod(): privateModule.publicClass;
}
class privateClassWithPrivateModuleParameterTypes {
    static myPublicStaticMethod(): privateModule.publicClass { 
AssertType(null, "null");
        return null;
    }
    myPublicMethod(): privateModule.publicClass { 
AssertType(null, "null");
        return null;
    }
    static myPublicStaticMethod1() { 
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
    myPublicMethod1() { 
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
}
function privateFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass { 
AssertType(null, "null");
    return null;
}
function privateFunctionWithPrivateModuleParameterTypes1() { 
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
    return new privateModule.publicClass();
}
declare function privateAmbientFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass; 

export module publicModule {
    class privateClass {
    }

    export class publicClass {
    }

    export interface publicInterfaceWithPrivateParmeterTypes {
        new (): privateClass; // Error
        (): privateClass; // Error
        [x: number]: privateClass; // Error
        myMethod(): privateClass; // Error
    }

    export interface publicInterfaceWithPublicParmeterTypes {
        new (): publicClass;
        (): publicClass;
        [x: number]: publicClass;
        myMethod(): publicClass;
    }

    interface privateInterfaceWithPrivateParmeterTypes {
        new (): privateClass;
        (): privateClass;
        [x: number]: privateClass;
        myMethod(): privateClass;
    }

    interface privateInterfaceWithPublicParmeterTypes {
        new (): publicClass;
        (): publicClass;
        [x: number]: publicClass;
        myMethod(): publicClass;
    }

    export class publicClassWithWithPrivateParmeterTypes {
        static myPublicStaticMethod(): privateClass { // Error
AssertType(null, "null");
            return null;
        }
        private static myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): privateClass { // Error
AssertType(null, "null");
            return null;
        }
        private myPrivateMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private static myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        myPublicMethod1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
    }

    export class publicClassWithWithPublicParmeterTypes {
        static myPublicStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private static myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private myPrivateMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private static myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
    }

    class privateClassWithWithPrivateParmeterTypes {
        static myPublicStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        private static myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        private myPrivateMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private static myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        myPublicMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
    }

    class privateClassWithWithPublicParmeterTypes {
        static myPublicStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private static myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private myPrivateMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private static myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
    }

    export function publicFunctionWithPrivateParmeterTypes(): privateClass { // Error
AssertType(null, "null");
        return null;
    }
    export function publicFunctionWithPublicParmeterTypes(): publicClass {
AssertType(null, "null");
        return null;
    }
    function privateFunctionWithPrivateParmeterTypes(): privateClass {
AssertType(null, "null");
        return null;
    }
    function privateFunctionWithPublicParmeterTypes(): publicClass {
AssertType(null, "null");
        return null;
    }
    export function publicFunctionWithPrivateParmeterTypes1() { // Error
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    export function publicFunctionWithPublicParmeterTypes1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    function privateFunctionWithPrivateParmeterTypes1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    function privateFunctionWithPublicParmeterTypes1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }

    export declare function publicAmbientFunctionWithPrivateParmeterTypes(): privateClass; // Error
    export declare function publicAmbientFunctionWithPublicParmeterTypes(): publicClass;
    declare function privateAmbientFunctionWithPrivateParmeterTypes(): privateClass;
    declare function privateAmbientFunctionWithPublicParmeterTypes(): publicClass;

    export interface publicInterfaceWithPrivateModuleParameterTypes {
        new (): privateModule.publicClass; // Error
        (): privateModule.publicClass; // Error
        [x: number]: privateModule.publicClass; // Error
        myMethod(): privateModule.publicClass; // Error
    }
    export class publicClassWithPrivateModuleParameterTypes {
        static myPublicStaticMethod(): privateModule.publicClass { // Error
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): privateModule.publicClass { // Error
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
            return new privateModule.publicClass();
        }
        myPublicMethod1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
            return new privateModule.publicClass();
        }
    }
    export function publicFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass { // Error
AssertType(null, "null");
        return null;
    }
    export function publicFunctionWithPrivateModuleParameterTypes1() { // Error
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
    export declare function publicAmbientFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass; // Error

    interface privateInterfaceWithPrivateModuleParameterTypes {
        new (): privateModule.publicClass;
        (): privateModule.publicClass;
        [x: number]: privateModule.publicClass;
        myMethod(): privateModule.publicClass;
    }
    class privateClassWithPrivateModuleParameterTypes {
        static myPublicStaticMethod(): privateModule.publicClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): privateModule.publicClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() {
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
            return new privateModule.publicClass();
        }
        myPublicMethod1() {
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
            return new privateModule.publicClass();
        }
    }
    function privateFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass {
AssertType(null, "null");
        return null;
    }
    function privateFunctionWithPrivateModuleParameterTypes1() {
AssertType(new privateModule.publicClass(), "privateModule.publicClass");
AssertType(privateModule.publicClass, "typeof privateModule.publicClass");
        return new privateModule.publicClass();
    }
    declare function privateAmbientFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass; 
}

module privateModule {
    class privateClass {
    }

    export class publicClass {
    }

    export interface publicInterfaceWithPrivateParmeterTypes {
        new (): privateClass; 
        (): privateClass; 
        [x: number]: privateClass; 
        myMethod(): privateClass; 
    }

    export interface publicInterfaceWithPublicParmeterTypes {
        new (): publicClass;
        (): publicClass;
        [x: number]: publicClass;
        myMethod(): publicClass;
    }

    interface privateInterfaceWithPrivateParmeterTypes {
        new (): privateClass;
        (): privateClass;
        [x: number]: privateClass;
        myMethod(): privateClass;
    }

    interface privateInterfaceWithPublicParmeterTypes {
        new (): publicClass;
        (): publicClass;
        [x: number]: publicClass;
        myMethod(): publicClass;
    }

    export class publicClassWithWithPrivateParmeterTypes {
        static myPublicStaticMethod(): privateClass { 
AssertType(null, "null");
            return null;
        }
        private static myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): privateClass { 
AssertType(null, "null");
            return null;
        }
        private myPrivateMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() { 
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private static myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        myPublicMethod1() { 
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
    }

    export class publicClassWithWithPublicParmeterTypes {
        static myPublicStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private static myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private myPrivateMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private static myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
    }

    class privateClassWithWithPrivateParmeterTypes {
        static myPublicStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        private static myPrivateStaticMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        private myPrivateMethod(): privateClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private static myPrivateStaticMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        myPublicMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
        private myPrivateMethod1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
            return new privateClass();
        }
    }

    class privateClassWithWithPublicParmeterTypes {
        static myPublicStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private static myPrivateStaticMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        private myPrivateMethod(): publicClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private static myPrivateStaticMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        myPublicMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
        private myPrivateMethod1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
            return new publicClass();
        }
    }

    export function publicFunctionWithPrivateParmeterTypes(): privateClass { 
AssertType(null, "null");
        return null;
    }
    export function publicFunctionWithPublicParmeterTypes(): publicClass {
AssertType(null, "null");
        return null;
    }
    function privateFunctionWithPrivateParmeterTypes(): privateClass {
AssertType(null, "null");
        return null;
    }
    function privateFunctionWithPublicParmeterTypes(): publicClass {
AssertType(null, "null");
        return null;
    }
    export function publicFunctionWithPrivateParmeterTypes1() { 
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    export function publicFunctionWithPublicParmeterTypes1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }
    function privateFunctionWithPrivateParmeterTypes1() {
AssertType(new privateClass(), "privateClass");
AssertType(privateClass, "typeof privateClass");
        return new privateClass();
    }
    function privateFunctionWithPublicParmeterTypes1() {
AssertType(new publicClass(), "publicClass");
AssertType(publicClass, "typeof publicClass");
        return new publicClass();
    }

    export declare function publicAmbientFunctionWithPrivateParmeterTypes(): privateClass; 
    export declare function publicAmbientFunctionWithPublicParmeterTypes(): publicClass;
    declare function privateAmbientFunctionWithPrivateParmeterTypes(): privateClass;
    declare function privateAmbientFunctionWithPublicParmeterTypes(): publicClass;

    export interface publicInterfaceWithPrivateModuleParameterTypes {
        new (): privateModule.publicClass;
        (): privateModule.publicClass;
        [x: number]: privateModule.publicClass;
        myMethod(): privateModule.publicClass;
    }
    export class publicClassWithPrivateModuleParameterTypes {
        static myPublicStaticMethod(): privateModule.publicClass { 
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): privateModule.publicClass { 
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() { 
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
            return new privateModule.publicClass();
        }
        myPublicMethod1() { 
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
            return new privateModule.publicClass();
        }
    }
    export function publicFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass { 
AssertType(null, "null");
        return null;
    }
    export function publicFunctionWithPrivateModuleParameterTypes1() { 
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
        return new privateModule.publicClass();
    }
    export declare function publicAmbientFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass; 

    interface privateInterfaceWithPrivateModuleParameterTypes {
        new (): privateModule.publicClass;
        (): privateModule.publicClass;
        [x: number]: privateModule.publicClass;
        myMethod(): privateModule.publicClass;
    }
    class privateClassWithPrivateModuleParameterTypes {
        static myPublicStaticMethod(): privateModule.publicClass {
AssertType(null, "null");
            return null;
        }
        myPublicMethod(): privateModule.publicClass {
AssertType(null, "null");
            return null;
        }
        static myPublicStaticMethod1() {
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
            return new privateModule.publicClass();
        }
        myPublicMethod1() {
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
            return new privateModule.publicClass();
        }
    }
    function privateFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass {
AssertType(null, "null");
        return null;
    }
    function privateFunctionWithPrivateModuleParameterTypes1() {
AssertType(new privateModule.publicClass(), "publicClass");
AssertType(privateModule.publicClass, "typeof publicClass");
        return new privateModule.publicClass();
    }
    declare function privateAmbientFunctionWithPrivateModuleParameterTypes(): privateModule.publicClass; 
}


