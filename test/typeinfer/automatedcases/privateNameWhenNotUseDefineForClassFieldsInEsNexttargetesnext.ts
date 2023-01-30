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

// === tests/cases/conformance/classes/members/privateNames/privateNameWhenNotUseDefineForClassFieldsInEsNext.ts ===
declare function AssertType(value:any, type:string):void;
class TestWithStatics {
    #prop = 0
    static dd = new TestWithStatics().#prop; // OK
    static ["X_ z_ zz"] = class Inner {
        #foo  = 10
        m() {
            new TestWithStatics().#prop // OK
AssertType(new TestWithStatics().#prop, "number");
AssertType(new TestWithStatics(), "TestWithStatics");
AssertType(TestWithStatics, "typeof TestWithStatics");
        }
        static C = class InnerInner {
            m() {
                new TestWithStatics().#prop // OK
AssertType(new TestWithStatics().#prop, "number");
AssertType(new TestWithStatics(), "TestWithStatics");
AssertType(TestWithStatics, "typeof TestWithStatics");

                new Inner().#foo; // OK
AssertType(new Inner().#foo, "number");
AssertType(new Inner(), "Inner");
AssertType(Inner, "typeof Inner");
            }
        }

        static M(){
AssertType(class {                m() {                    new TestWithStatics().#prop // OK                    new Inner().#foo; // OK                }            }, "typeof (Anonymous class)");
            return class {

                m() {
AssertType(m, "() => void");

                    new TestWithStatics().#prop // OK
AssertType(new TestWithStatics().#prop, "number");
AssertType(new TestWithStatics(), "TestWithStatics");
AssertType(TestWithStatics, "typeof TestWithStatics");

                    new Inner().#foo; // OK
AssertType(new Inner().#foo, "number");
AssertType(new Inner(), "Inner");
AssertType(Inner, "typeof Inner");
                }
            }
        }
    }
}

class TestNonStatics {
    #prop = 0
    dd = new TestNonStatics().#prop; // OK
    ["X_ z_ zz"] = class Inner {
        #foo  = 10
        m() {
            new TestNonStatics().#prop // Ok
AssertType(new TestNonStatics().#prop, "number");
AssertType(new TestNonStatics(), "TestNonStatics");
AssertType(TestNonStatics, "typeof TestNonStatics");
        }
        C = class InnerInner {
            m() {
                new TestNonStatics().#prop // Ok
AssertType(new TestNonStatics().#prop, "number");
AssertType(new TestNonStatics(), "TestNonStatics");
AssertType(TestNonStatics, "typeof TestNonStatics");

                new Inner().#foo; // Ok
AssertType(new Inner().#foo, "number");
AssertType(new Inner(), "Inner");
AssertType(Inner, "typeof Inner");
            }
        }

        static M(){
AssertType(class {                m() {                    new TestNonStatics().#prop // OK                    new Inner().#foo; // OK                }            }, "typeof (Anonymous class)");
            return class {

                m() {
AssertType(m, "() => void");

                    new TestNonStatics().#prop // OK
AssertType(new TestNonStatics().#prop, "number");
AssertType(new TestNonStatics(), "TestNonStatics");
AssertType(TestNonStatics, "typeof TestNonStatics");

                    new Inner().#foo; // OK
AssertType(new Inner().#foo, "number");
AssertType(new Inner(), "Inner");
AssertType(Inner, "typeof Inner");
                }
            }
        }
    }
}

