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

// === tests/cases/compiler/privacyCannotNameVarTypeDeclFile_consumer.ts ===
declare function AssertType(value:any, type:string):void;
import * as exporter from "./privacyCannotNameVarTypeDeclFile_exporter";
export class publicClassWithWithPrivatePropertyTypes {
    static myPublicStaticProperty = exporter.createExportedWidget1(); // Error
    private static myPrivateStaticProperty = exporter.createExportedWidget1();
    myPublicProperty = exporter.createExportedWidget1(); // Error
    private myPrivateProperty = exporter.createExportedWidget1();

    static myPublicStaticProperty1 = exporter.createExportedWidget3(); // Error
    private static myPrivateStaticProperty1 = exporter.createExportedWidget3();
    myPublicProperty1 = exporter.createExportedWidget3(); // Error
    private myPrivateProperty1 = exporter.createExportedWidget3();
}

class privateClassWithWithPrivatePropertyTypes {
    static myPublicStaticProperty = exporter.createExportedWidget1(); 
    private static myPrivateStaticProperty = exporter.createExportedWidget1();
    myPublicProperty = exporter.createExportedWidget1(); 
    private myPrivateProperty = exporter.createExportedWidget1();

    static myPublicStaticProperty1 = exporter.createExportedWidget3(); 
    private static myPrivateStaticProperty1 = exporter.createExportedWidget3();
    myPublicProperty1 = exporter.createExportedWidget3(); 
    private myPrivateProperty1 = exporter.createExportedWidget3();
}

export let publicVarWithPrivatePropertyTypes= exporter.createExportedWidget1(); // Error
AssertType(publicVarWithPrivatePropertyTypes, "import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").Widget1");

let privateVarWithPrivatePropertyTypes= exporter.createExportedWidget1();
AssertType(privateVarWithPrivatePropertyTypes, "import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1(), "import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").Widget1");
AssertType(exporter.createExportedWidget1, "() => import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").Widget1");

export let publicVarWithPrivatePropertyTypes1 = exporter.createExportedWidget3(); // Error
AssertType(publicVarWithPrivatePropertyTypes1, "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");

let privateVarWithPrivatePropertyTypes1 = exporter.createExportedWidget3();
AssertType(privateVarWithPrivatePropertyTypes1, "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3(), "import("GlobalWidgets").Widget3");
AssertType(exporter.createExportedWidget3, "() => import("GlobalWidgets").Widget3");

export class publicClassWithPrivateModulePropertyTypes {
    static myPublicStaticProperty= exporter.createExportedWidget2(); // Error
    myPublicProperty = exporter.createExportedWidget2(); // Error
    static myPublicStaticProperty1 = exporter.createExportedWidget4(); // Error
    myPublicProperty1 = exporter.createExportedWidget4(); // Error
}
export let publicVarWithPrivateModulePropertyTypes= exporter.createExportedWidget2(); // Error
AssertType(publicVarWithPrivateModulePropertyTypes, "import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").SpecializedWidget.Widget2");

export let publicVarWithPrivateModulePropertyTypes1 = exporter.createExportedWidget4(); // Error
AssertType(publicVarWithPrivateModulePropertyTypes1, "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");

class privateClassWithPrivateModulePropertyTypes {
    static myPublicStaticProperty= exporter.createExportedWidget2();
    myPublicProperty= exporter.createExportedWidget2();
    static myPublicStaticProperty1 = exporter.createExportedWidget4();
    myPublicProperty1 = exporter.createExportedWidget4();
}
let privateVarWithPrivateModulePropertyTypes= exporter.createExportedWidget2();
AssertType(privateVarWithPrivateModulePropertyTypes, "import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2(), "import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").SpecializedWidget.Widget2");
AssertType(exporter.createExportedWidget2, "() => import("tests/cases/compiler/privacyCannotNameVarTypeDeclFile_Widgets").SpecializedWidget.Widget2");

let privateVarWithPrivateModulePropertyTypes1 = exporter.createExportedWidget4();
AssertType(privateVarWithPrivateModulePropertyTypes1, "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4(), "import("GlobalWidgets").SpecializedGlobalWidget.Widget4");
AssertType(exporter.createExportedWidget4, "() => import("GlobalWidgets").SpecializedGlobalWidget.Widget4");


