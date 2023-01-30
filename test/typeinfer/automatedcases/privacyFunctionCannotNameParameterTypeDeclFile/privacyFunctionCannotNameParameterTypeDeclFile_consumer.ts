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

// === tests/cases/compiler/privacyFunctionCannotNameParameterTypeDeclFile_consumer.ts ===
declare function AssertType(value:any, type:string):void;

import * as exporter from "./privacyFunctionCannotNameParameterTypeDeclFile_exporter";
export class publicClassWithWithPrivateParmeterTypes {
    static myPublicStaticMethod(param = exporter.createExportedWidget1()) { // Error
    }
    private static myPrivateStaticMethod(param = exporter.createExportedWidget1()) {
    }
    myPublicMethod(param  = exporter.createExportedWidget1()) { // Error
    }
    private myPrivateMethod(param = exporter.createExportedWidget1()) {
    }
    constructor(param = exporter.createExportedWidget1(), private param1 = exporter.createExportedWidget1(), public param2 = exporter.createExportedWidget1()) { // Error
    }
}
export class publicClassWithWithPrivateParmeterTypes1 {
    static myPublicStaticMethod(param = exporter.createExportedWidget3()) { // Error
    }
    private static myPrivateStaticMethod(param = exporter.createExportedWidget3()) {
    }
    myPublicMethod(param  = exporter.createExportedWidget3()) { // Error
    }
    private myPrivateMethod(param = exporter.createExportedWidget3()) {
    }
    constructor(param = exporter.createExportedWidget3(), private param1 = exporter.createExportedWidget3(), public param2 = exporter.createExportedWidget3()) { // Error
    }
}

class privateClassWithWithPrivateParmeterTypes {
    static myPublicStaticMethod(param = exporter.createExportedWidget1()) {
    }
    private static myPrivateStaticMethod(param = exporter.createExportedWidget1()) {
    }
    myPublicMethod(param  = exporter.createExportedWidget1()) {
    }
    private myPrivateMethod(param = exporter.createExportedWidget1()) {
    }
    constructor(param = exporter.createExportedWidget1(), private param1 = exporter.createExportedWidget1(), public param2 = exporter.createExportedWidget1()) {
    }
}
class privateClassWithWithPrivateParmeterTypes2 {
    static myPublicStaticMethod(param = exporter.createExportedWidget3()) {
    }
    private static myPrivateStaticMethod(param = exporter.createExportedWidget3()) {
    }
    myPublicMethod(param  = exporter.createExportedWidget3()) {
    }
    private myPrivateMethod(param = exporter.createExportedWidget3()) {
    }
    constructor(param = exporter.createExportedWidget3(), private param1 = exporter.createExportedWidget3(), public param2 = exporter.createExportedWidget3()) {
    }
}

export function publicFunctionWithPrivateParmeterTypes(param = exporter.createExportedWidget1()) { // Error
}
function privateFunctionWithPrivateParmeterTypes(param = exporter.createExportedWidget1()) {
}
export function publicFunctionWithPrivateParmeterTypes1(param = exporter.createExportedWidget3()) { // Error
}
function privateFunctionWithPrivateParmeterTypes1(param = exporter.createExportedWidget3()) {
}


export class publicClassWithPrivateModuleParameterTypes {
    static myPublicStaticMethod(param= exporter.createExportedWidget2()) { // Error
    }
    myPublicMethod(param= exporter.createExportedWidget2()) { // Error
    }
    constructor(param= exporter.createExportedWidget2(), private param1= exporter.createExportedWidget2(), public param2= exporter.createExportedWidget2()) { // Error
    }
}
export class publicClassWithPrivateModuleParameterTypes2 {
    static myPublicStaticMethod(param= exporter.createExportedWidget4()) { // Error
    }
    myPublicMethod(param= exporter.createExportedWidget4()) { // Error
    }
    constructor(param= exporter.createExportedWidget4(), private param1= exporter.createExportedWidget4(), public param2= exporter.createExportedWidget4()) { // Error
    }
}
export function publicFunctionWithPrivateModuleParameterTypes(param= exporter.createExportedWidget2()) { // Error
}
export function publicFunctionWithPrivateModuleParameterTypes1(param= exporter.createExportedWidget4()) { // Error
}


class privateClassWithPrivateModuleParameterTypes {
    static myPublicStaticMethod(param= exporter.createExportedWidget2()) {
    }
    myPublicMethod(param= exporter.createExportedWidget2()) {
    }
    constructor(param= exporter.createExportedWidget2(), private param1= exporter.createExportedWidget2(), public param2= exporter.createExportedWidget2()) {
    }
}
class privateClassWithPrivateModuleParameterTypes1 {
    static myPublicStaticMethod(param= exporter.createExportedWidget4()) {
    }
    myPublicMethod(param= exporter.createExportedWidget4()) {
    }
    constructor(param= exporter.createExportedWidget4(), private param1= exporter.createExportedWidget4(), public param2= exporter.createExportedWidget4()) {
    }
}
function privateFunctionWithPrivateModuleParameterTypes(param= exporter.createExportedWidget2()) {
}
function privateFunctionWithPrivateModuleParameterTypes1(param= exporter.createExportedWidget4()) {
}

