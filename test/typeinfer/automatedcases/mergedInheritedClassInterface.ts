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

// === tests/cases/conformance/classes/classDeclarations/mergedInheritedClassInterface.ts ===
declare function AssertType(value:any, type:string):void;
interface BaseInterface {
    required: number;
    optional?: number;
}

class BaseClass {
    baseMethod() { }
    baseNumber: number;
}

interface Child extends BaseInterface {
    additional: number;
}

class Child extends BaseClass {
    classNumber: number;
    method() { }
}

interface ChildNoBaseClass extends BaseInterface {
    additional2: string;
}
class ChildNoBaseClass {
    classString: string;
    method2() { }
}
class Grandchild extends ChildNoBaseClass {
}

// checks if properties actually were merged
let child : Child;
AssertType(child, "Child");

child.required;
AssertType(child.required, "number");

child.optional;
AssertType(child.optional, "number");

child.additional;
AssertType(child.additional, "number");

child.baseNumber;
AssertType(child.baseNumber, "number");

child.classNumber;
AssertType(child.classNumber, "number");

child.baseMethod();
AssertType(child.baseMethod(), "void");
AssertType(child.baseMethod, "() => void");

child.method();
AssertType(child.method(), "void");
AssertType(child.method, "() => void");

let grandchild: Grandchild;
AssertType(grandchild, "Grandchild");

grandchild.required;
AssertType(grandchild.required, "number");

grandchild.optional;
AssertType(grandchild.optional, "number");

grandchild.additional2;
AssertType(grandchild.additional2, "string");

grandchild.classString;
AssertType(grandchild.classString, "string");

grandchild.method2();
AssertType(grandchild.method2(), "void");
AssertType(grandchild.method2, "() => void");


