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

// === tests/cases/compiler/thisInPropertyBoundDeclarations.ts ===
declare function AssertType(value:any, type:string):void;
class Bug {
    private name: string;

    private static func: Function[] = [
     (that: Bug, name: string) => {
         that.foo(name);
AssertType(that.foo(name), "void");
AssertType(that.foo, "(string) => void");
AssertType(name, "string");
     }
    ];

    private foo(name: string) {
        this.name = name;
AssertType(this.name = name, "string");
AssertType(this.name, "string");
AssertType(this, "this");
AssertType(name, "string");
    }
}

// Valid use of this in a property bound decl
class A {
    prop1 = function() {
        this;
AssertType(this, "any");

    };

    prop2 = function() {
        function inner() {
AssertType(inner, "() => void");

            this;
AssertType(this, "any");
        }
        () => this;
AssertType(() => this, "() => any");
AssertType(this, "any");

    };

    prop3 = () => {
        function inner() {
AssertType(inner, "() => void");

            this;
AssertType(this, "any");
        }
    };

    prop4 = {
        a: function() { 
AssertType(this, "any");
return this; },

    };

    prop5 = () => {
AssertType({            a: function() { return this; },        }, "{ a: () => any; }");
        return {

            a: function() { 
AssertType(a, "() => any");
AssertType(function() { return this; }, "() => any");
AssertType(this, "any");
return this; },

        };
    };
}

class B {
    prop1 = this;

    prop2 = () => this;

    prop3 = () => () => () => () => this;

    prop4 = '  ' +
    function() {
    } +
    ' ' +
    (() => () => () => this);

    prop5 = {
        a: () => { 
AssertType(this, "this");
return this; 
}

    };

    prop6 = () => {
AssertType({            a: () => { return this; }        }, "{ a: () => this; }");
        return {

            a: () => { 
AssertType(a, "() => this");
return this; 

AssertType(() => { return this; }, "() => this");

AssertType(this, "this");
}

        };
    };
}

