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

// === tests/cases/compiler/missingSelf.ts ===
declare function AssertType(value:any, type:string):void;
class CalcButton
{
    public a() { this.onClick(); 
AssertType(this.onClick(), "void");

AssertType(this.onClick, "() => void");

AssertType(this, "this");
}

    public onClick() { }
}

class CalcButton2
{
    public b() { () => this.onClick(); 
AssertType(() => this.onClick(), "() => void");

AssertType(this.onClick(), "void");

AssertType(this.onClick, "() => void");

AssertType(this, "this");
}

    public onClick() { }
}

let c = new CalcButton();
AssertType(c, "CalcButton");
AssertType(new CalcButton(), "CalcButton");
AssertType(CalcButton, "typeof CalcButton");

c.a();
AssertType(c.a(), "void");
AssertType(c.a, "() => void");

let c2 = new CalcButton2();
AssertType(c2, "CalcButton2");
AssertType(new CalcButton2(), "CalcButton2");
AssertType(CalcButton2, "typeof CalcButton2");

c2.b();
AssertType(c2.b(), "void");
AssertType(c2.b, "() => void");



