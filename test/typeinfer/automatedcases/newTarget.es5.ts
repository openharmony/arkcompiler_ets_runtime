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

// === tests/cases/conformance/es6/newTarget/newTarget.es5.ts ===
declare function AssertType(value:any, type:string):void;
class A {
    constructor() {
        const a = new.target;
AssertType(a, "typeof A");
AssertType(new.target, "typeof A");
AssertType(target, "any");

        const b = () => new.target;
AssertType(b, "() => typeof A");
AssertType(() => new.target, "() => typeof A");
AssertType(new.target, "typeof A");
AssertType(target, "any");
    }
    static c = function () { 
AssertType(new.target, "() => any");
return new.target; 

AssertType(target, "any");
}

    d = function () { 
AssertType(new.target, "() => any");
return new.target; 

AssertType(target, "any");
}
}

class B extends A {
    constructor() {
        super();
AssertType(super(), "void");
AssertType(super, "typeof A");

        const e = new.target;
AssertType(e, "typeof B");
AssertType(new.target, "typeof B");
AssertType(target, "any");

        const f = () => new.target;
AssertType(f, "() => typeof B");
AssertType(() => new.target, "() => typeof B");
AssertType(new.target, "typeof B");
AssertType(target, "any");
    }
}

function f1() {
    const g = new.target;
AssertType(g, "() => void");
AssertType(new.target, "() => void");
AssertType(target, "any");

    const h = () => new.target;
AssertType(h, "() => () => void");
AssertType(() => new.target, "() => () => void");
AssertType(new.target, "() => void");
AssertType(target, "any");
}

const f2 = function () {
AssertType(f2, "() => void");
AssertType(function () {    const i = new.target;    const j = () => new.target;}, "() => void");

    const i = new.target;
AssertType(i, "() => void");
AssertType(new.target, "() => void");
AssertType(target, "any");

    const j = () => new.target;
AssertType(j, "() => () => void");
AssertType(() => new.target, "() => () => void");
AssertType(new.target, "() => void");
AssertType(target, "any");
}

const O = {
AssertType(O, "{ k: () => any; }");
AssertType({    k: function () { return new.target; }}, "{ k: () => any; }");

    k: function () { 
AssertType(k, "() => any");
return new.target; 

AssertType(function () { return new.target; }, "() => any");

AssertType(new.target, "() => any");

AssertType(target, "any");
}

};



