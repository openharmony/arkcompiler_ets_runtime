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

// === tests/cases/compiler/privacyFunctionReturnTypeDeclFile_consumer.ts ===
declare function AssertType(value:any, type:string):void;
import * as exporter from "./privacyFunctionReturnTypeDeclFile_exporter";
export class publicClassWithWithPrivateParmeterTypes {
    static myPublicStaticMethod() { // Error
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    private static myPrivateStaticMethod() {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();;
    }
    myPublicMethod() { // Error
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();;
    }
    private myPrivateMethod() {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();;
    }
    static myPublicStaticMethod1() { // Error
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
    private static myPrivateStaticMethod1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();;
    }
    myPublicMethod1() { // Error
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();;
    }
    private myPrivateMethod1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();;
    }
}

class privateClassWithWithPrivateParmeterTypes {
    static myPublicStaticMethod() { 
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    private static myPrivateStaticMethod() {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();;
    }
    myPublicMethod() { 
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();;
    }
    private myPrivateMethod() {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();;
    }
    static myPublicStaticMethod1() { 
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
    private static myPrivateStaticMethod1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();;
    }
    myPublicMethod1() { 
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();;
    }
    private myPrivateMethod1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();;
    }
}

export function publicFunctionWithPrivateParmeterTypes() { // Error
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
    return exporter.createExportedWidget1();
}
function privateFunctionWithPrivateParmeterTypes()   {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").Widget1");
    return exporter.createExportedWidget1();
}
export function publicFunctionWithPrivateParmeterTypes1() { // Error
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
    return exporter.createExportedWidget3();
}
function privateFunctionWithPrivateParmeterTypes1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
    return exporter.createExportedWidget3();
}

export class publicClassWithPrivateModuleReturnTypes {
    static myPublicStaticMethod() { // Error
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
        return exporter.createExportedWidget2();
    }
    myPublicMethod() { // Error
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
        return exporter.createExportedWidget2();
    }
    static myPublicStaticMethod1() { // Error
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
        return exporter.createExportedWidget4();
    }
    myPublicMethod1() { // Error
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
        return exporter.createExportedWidget4();
    }
}
export function publicFunctionWithPrivateModuleReturnTypes() { // Error
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
    return exporter.createExportedWidget2();
}
export function publicFunctionWithPrivateModuleReturnTypes1() { // Error
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
    return exporter.createExportedWidget4();
}

class privateClassWithPrivateModuleReturnTypes {
    static myPublicStaticMethod() { 
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
        return exporter.createExportedWidget2();
    }
    myPublicMethod() { 
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
        return exporter.createExportedWidget2();
    }
    static myPublicStaticMethod1() { // Error
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
        return exporter.createExportedWidget4();
    }
    myPublicMethod1() { // Error
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
        return exporter.createExportedWidget4();
    }
}
function privateFunctionWithPrivateModuleReturnTypes() { 
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyFunctionReturnTypeDeclFile_Widgets").SpecializedWidget.Widget2");
    return exporter.createExportedWidget2();
}
function privateFunctionWithPrivateModuleReturnTypes1() {
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
    return exporter.createExportedWidget4();
}


