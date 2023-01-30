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

// === tests/cases/compiler/privacyCannotNameAccessorDeclFile_consumer.ts ===
declare function AssertType(value:any, type:string):void;
import * as exporter from "./privacyCannotNameAccessorDeclFile_exporter";
export class publicClassWithWithPrivateGetAccessorTypes {
    static get myPublicStaticMethod() { // Error
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    private static get myPrivateStaticMethod() {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    get myPublicMethod() { // Error
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    private get myPrivateMethod() {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    static get myPublicStaticMethod1() { // Error
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
    private static get myPrivateStaticMethod1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
    get myPublicMethod1() { // Error
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
    private get myPrivateMethod1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
}

class privateClassWithWithPrivateGetAccessorTypes {
    static get myPublicStaticMethod() { 
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    private static get myPrivateStaticMethod() {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    get myPublicMethod() { 
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    private get myPrivateMethod() {
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").Widget1");
        return exporter.createExportedWidget1();
    }
    static get myPublicStaticMethod1() { 
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
    private static get myPrivateStaticMethod1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
    get myPublicMethod1() { 
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
    private get myPrivateMethod1() {
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");
        return exporter.createExportedWidget3();
    }
}

export class publicClassWithPrivateModuleGetAccessorTypes {
    static get myPublicStaticMethod() { // Error
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").SpecializedWidget.Widget2");
        return exporter.createExportedWidget2();
    }
    get myPublicMethod() { // Error
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").SpecializedWidget.Widget2");
        return exporter.createExportedWidget2();
    }
    static get myPublicStaticMethod1() { // Error
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
        return exporter.createExportedWidget4();
    }
    get myPublicMethod1() { // Error
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
        return exporter.createExportedWidget4();
    }
}

class privateClassWithPrivateModuleGetAccessorTypes {
    static get myPublicStaticMethod() { 
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").SpecializedWidget.Widget2");
        return exporter.createExportedWidget2();
    }
    get myPublicMethod() { 
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyCannotNameAccessorDeclFile_Widgets").SpecializedWidget.Widget2");
        return exporter.createExportedWidget2();
    }
    static get myPublicStaticMethod1() { 
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
        return exporter.createExportedWidget4();
    }
    get myPublicMethod1() { 
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
        return exporter.createExportedWidget4();
    }
}

