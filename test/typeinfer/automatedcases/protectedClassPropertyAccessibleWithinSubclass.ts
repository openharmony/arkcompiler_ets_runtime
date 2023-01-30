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

// === tests/cases/conformance/classes/members/accessibility/protectedClassPropertyAccessibleWithinSubclass.ts ===
declare function AssertType(value:any, type:string):void;
// no errors

class B {
    protected x: string;
    protected static x: string;
}

class C extends B {
    protected get y() { 
AssertType(this.x, "string");
return this.x; 

AssertType(this, "this");
}

    protected set y(x) { this.y = this.x; 
AssertType(this.y = this.x, "string");

AssertType(this.y, "string");

AssertType(this, "this");

AssertType(this.x, "string");

AssertType(this, "this");
}

    protected foo() { 
AssertType(this.x, "string");
return this.x; 

AssertType(this, "this");
}

    protected bar() { 
AssertType(this.foo(), "string");
return this.foo(); 

AssertType(this.foo, "() => string");

AssertType(this, "this");
}

    protected static get y() { 
AssertType(this.x, "string");
return this.x; 

AssertType(this, "typeof C");
}

    protected static set y(x) { this.y = this.x; 
AssertType(this.y = this.x, "string");

AssertType(this.y, "string");

AssertType(this, "typeof C");

AssertType(this.x, "string");

AssertType(this, "typeof C");
}

    protected static foo() { 
AssertType(this.x, "string");
return this.x; 

AssertType(this, "typeof C");
}

    protected static bar() { this.foo(); 
AssertType(this.foo(), "string");

AssertType(this.foo, "() => string");

AssertType(this, "typeof C");
}
}


