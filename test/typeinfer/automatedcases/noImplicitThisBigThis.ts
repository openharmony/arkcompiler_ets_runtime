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

// === tests/cases/compiler/noImplicitThisBigThis.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/29902

function createObj() {
AssertType({        func1() {            return this;        },        func2() {            return this;        },        func3() {            return this;        }    }, "{ func1(): { func1(): any; func2(): any; func3(): any; }; func2(): { func1(): any; func2(): any; func3(): any; }; func3(): { func1(): any; func2(): any; func3(): any; }; }");
    return {

        func1() {
AssertType(func1, "() => { func1(): any; func2(): any; func3(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; }");
            return this;

        },
        func2() {
AssertType(func2, "() => { func1(): any; func2(): any; func3(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; }");
            return this;

        },
        func3() {
AssertType(func3, "() => { func1(): any; func2(): any; func3(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; }");
            return this;
        }
    };
}

function createObjNoCrash() {
AssertType({        func1() {            return this;        },        func2() {            return this;        },        func3() {            return this;        },        func4() {            return this;        },        func5() {            return this;        },        func6() {            return this;        },        func7() {            return this;        },        func8() {            return this;        },        func9() {            return this;        }    }, "{ func1(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; func2(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; func3(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; func4(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; func5(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; func6(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; func7(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; func8(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; func9(): { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }; }");
    return {

        func1() {
AssertType(func1, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;

        },
        func2() {
AssertType(func2, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;

        },
        func3() {
AssertType(func3, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;

        },
        func4() {
AssertType(func4, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;

        },
        func5() {
AssertType(func5, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;

        },
        func6() {
AssertType(func6, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;

        },
        func7() {
AssertType(func7, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;

        },
        func8() {
AssertType(func8, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;

        },
        func9() {
AssertType(func9, "() => { func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");

AssertType(this, "{ func1(): any; func2(): any; func3(): any; func4(): any; func5(): any; func6(): any; func7(): any; func8(): any; func9(): any; }");
            return this;
        }
    };
}


