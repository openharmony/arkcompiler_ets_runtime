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

// === tests/cases/conformance/expressions/contextualTyping/superCallParameterContextualTyping3.ts ===
declare function AssertType(value:any, type:string):void;
interface ContextualType<T> {
    method(parameter: T): void;
}

class CBase<T>  {
    constructor(param: ContextualType<T>) {
    }

    foo(param: ContextualType<T>) {
    }
}

class C extends CBase<string> {
    constructor() {
        // Should be okay.
        // 'p' should have type 'string'.
        super({
AssertType(super({            method(p) {                p.length;            }        }), "void");
AssertType(super, "typeof CBase");
AssertType({            method(p) {                p.length;            }        }, "{ method(string): void; }");

            method(p) {
AssertType(method, "(string) => void");
AssertType(p, "string");

                p.length;
AssertType(p.length, "number");
            }
        });

        // Should be okay.
        // 'p' should have type 'string'.
        super.foo({
AssertType(super.foo({            method(p) {                p.length;            }        }), "void");
AssertType(super.foo, "(ContextualType<string>) => void");
AssertType(super, "CBase<string>");
AssertType({            method(p) {                p.length;            }        }, "{ method(string): void; }");

            method(p) {
AssertType(method, "(string) => void");
AssertType(p, "string");

                p.length;
AssertType(p.length, "number");
            }
        });
    }
}

