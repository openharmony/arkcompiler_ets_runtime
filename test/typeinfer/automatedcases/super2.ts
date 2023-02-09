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

// === tests/cases/compiler/super2.ts ===
declare function AssertType(value:any, type:string):void;
// Case 5
class Base5 {
    public x() {
AssertType("BaseX", "string");
        return "BaseX";
    }
    
    public y() {
AssertType("BaseY", "string");
        return "BaseY";
    }
}

class Sub5 extends Base5 {
    public x() {
AssertType("SubX", "string");
        return "SubX";
    }
}

class SubSub5 extends Sub5 {
    public x() {
AssertType(super.x(), "string");
AssertType(super.x, "() => string");
AssertType(super, "Sub5");
        return super.x();
    }
    public y() {
AssertType(super.y(), "string");
AssertType(super.y, "() => string");
AssertType(super, "Sub5");
        return super.y();
    }
}

// Case 6
class Base6 {
    public x() {
AssertType("BaseX", "string");
        return "BaseX";
    }
}

class Sub6 extends Base6 {
    public y() {
AssertType("SubY", "string");
        return "SubY";
    }
}

class SubSub6 extends Sub6 {
    public y() {
AssertType(super.y(), "string");
AssertType(super.y, "() => string");
AssertType(super, "Sub6");
        return super.y();
    }
}


let results1 = new SubSub5();
AssertType(results1, "SubSub5");
AssertType(new SubSub5(), "SubSub5");
AssertType(SubSub5, "typeof SubSub5");

let results2 = new SubSub6();
AssertType(results2, "SubSub6");
AssertType(new SubSub6(), "SubSub6");
AssertType(SubSub6, "typeof SubSub6");

results1.x() + results1.y() + results2.y();
AssertType(results1.x() + results1.y() + results2.y(), "string");
AssertType(results1.x() + results1.y(), "string");
AssertType(results1.x(), "string");
AssertType(results1.x, "() => string");
AssertType(results1.y(), "string");
AssertType(results1.y, "() => string");
AssertType(results2.y(), "string");
AssertType(results2.y, "() => string");


