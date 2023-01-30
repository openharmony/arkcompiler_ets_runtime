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

// === tests/cases/conformance/classes/members/accessibility/privateClassPropertyAccessibleWithinNestedClass.ts ===
declare function AssertType(value:any, type:string):void;
// no errors

class C {
    private x: string;
    private get y() { 
AssertType(this.x, "string");
return this.x; 

AssertType(this, "this");
}

    private set y(x) { this.y = this.x; 
AssertType(this.y = this.x, "string");

AssertType(this.y, "string");

AssertType(this, "this");

AssertType(this.x, "string");

AssertType(this, "this");
}

    private foo() { 
AssertType(this.foo, "() => any");
return this.foo; 

AssertType(this, "this");
}

    private static x: string;
    private static get y() { 
AssertType(this.x, "string");
return this.x; 

AssertType(this, "typeof C");
}

    private static set y(x) { this.y = this.x; 
AssertType(this.y = this.x, "string");

AssertType(this.y, "string");

AssertType(this, "typeof C");

AssertType(this.x, "string");

AssertType(this, "typeof C");
}

    private static foo() { 
AssertType(this.foo, "() => typeof C.foo");
return this.foo; 

AssertType(this, "typeof C");
}

    private static bar() { this.foo(); 
AssertType(this.foo(), "() => typeof C.foo");

AssertType(this.foo, "() => typeof C.foo");

AssertType(this, "typeof C");
}

    private bar() {
        class C2 {
            private foo() {
                let x: C;
AssertType(x, "C");

                let x1 = x.foo;
AssertType(x1, "() => any");
AssertType(x.foo, "() => any");

                let x2 = x.bar;
AssertType(x2, "() => void");
AssertType(x.bar, "() => void");

                let x3 = x.x;
AssertType(x3, "string");
AssertType(x.x, "string");

                let x4 = x.y;
AssertType(x4, "string");
AssertType(x.y, "string");

                let sx1 = C.x;
AssertType(sx1, "string");
AssertType(C.x, "string");

                let sx2 = C.y;
AssertType(sx2, "string");
AssertType(C.y, "string");

                let sx3 = C.bar;
AssertType(sx3, "() => void");
AssertType(C.bar, "() => void");

                let sx4 = C.foo;
AssertType(sx4, "() => typeof C.foo");
AssertType(C.foo, "() => typeof C.foo");

                let y = new C();
AssertType(y, "C");
AssertType(new C(), "C");
AssertType(C, "typeof C");

                let y1 = y.foo;
AssertType(y1, "() => any");
AssertType(y.foo, "() => any");

                let y2 = y.bar;
AssertType(y2, "() => void");
AssertType(y.bar, "() => void");

                let y3 = y.x;
AssertType(y3, "string");
AssertType(y.x, "string");

                let y4 = y.y;
AssertType(y4, "string");
AssertType(y.y, "string");
            }
        }
    }
}

