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

// === tests/cases/conformance/classes/members/accessibility/protectedClassPropertyAccessibleWithinClass.ts ===
declare function AssertType(value:any, type:string):void;
// no errors

class C {
    protected x: string;
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
AssertType(this.foo, "() => any");
return this.foo; 

AssertType(this, "this");
}

    protected static x: string;
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
AssertType(this.foo, "() => typeof C.foo");
return this.foo; 

AssertType(this, "typeof C");
}

    protected static bar() { this.foo(); 
AssertType(this.foo(), "() => typeof C.foo");

AssertType(this.foo, "() => typeof C.foo");

AssertType(this, "typeof C");
}
}

// added level of function nesting
class C2 {
    protected x: string;
    protected get y() { () => this.x; 
AssertType(() => this.x, "() => string");
return null; 

AssertType(this.x, "string");

AssertType(this, "this");

AssertType(null, "null");
}

    protected set y(x) { () => { this.y = this.x; } 
AssertType(() => { this.y = this.x; }, "() => void");

AssertType(this.y = this.x, "string");

AssertType(this.y, "any");

AssertType(this, "this");

AssertType(this.x, "string");

AssertType(this, "this");
}

    protected foo() { () => this.foo; 
AssertType(() => this.foo, "() => () => void");

AssertType(this.foo, "() => void");

AssertType(this, "this");
}

    protected static x: string;
    protected static get y() { () => this.x; 
AssertType(() => this.x, "() => string");
return null; 

AssertType(this.x, "string");

AssertType(this, "typeof C2");

AssertType(null, "null");
}

    protected static set y(x) {
        () => { this.y = this.x; 
AssertType(() => { this.y = this.x; }, "() => void");

AssertType(this.y = this.x, "string");

AssertType(this.y, "any");

AssertType(this, "typeof C2");

AssertType(this.x, "string");

AssertType(this, "typeof C2");
}
     }
    protected static foo() { () => this.foo; 
AssertType(() => this.foo, "() => () => void");

AssertType(this.foo, "() => void");

AssertType(this, "typeof C2");
}

    protected static bar() { () => this.foo(); 
AssertType(() => this.foo(), "() => void");

AssertType(this.foo(), "void");

AssertType(this.foo, "() => void");

AssertType(this, "typeof C2");
}
}


