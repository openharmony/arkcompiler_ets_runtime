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

// === tests/cases/compiler/arrowFunctionParsingDoesNotConfuseParenthesizedObjectForArrowHead.ts ===
declare function AssertType(value:any, type:string):void;
// regression test for https://github.com/microsoft/TypeScript/issues/32914
declare let value: boolean;
AssertType(value, "boolean");

declare let a: any;
AssertType(a, "any");

const test = () => ({
AssertType(test, "() => { prop: boolean; run: () => union; }");
AssertType(({    // "Identifier expected." error on "!" and two "Duplicate identifier '(Missing)'." errors on space.    prop: !value, // remove ! to see that errors will be gone    run: () => { //replace arrow function with regular function to see that errors will be gone        // comment next line or remove "()" to see that errors will be gone        if(!a.b()) { return 'special'; }        return 'default';    }}), "{ prop: boolean; run: () => union; }");
AssertType({    // "Identifier expected." error on "!" and two "Duplicate identifier '(Missing)'." errors on space.    prop: !value, // remove ! to see that errors will be gone    run: () => { //replace arrow function with regular function to see that errors will be gone        // comment next line or remove "()" to see that errors will be gone        if(!a.b()) { return 'special'; }        return 'default';    }}, "{ prop: boolean; run: () => union; }");
AssertType(() => ({    // "Identifier expected." error on "!" and two "Duplicate identifier '(Missing)'." errors on space.    prop: !value, // remove ! to see that errors will be gone    run: () => { //replace arrow function with regular function to see that errors will be gone        // comment next line or remove "()" to see that errors will be gone        if(!a.b()) { return 'special'; }        return 'default';    }}), "() => { prop: boolean; run: () => union; }");

    // "Identifier expected." error on "!" and two "Duplicate identifier '(Missing)'." errors on space.
    prop: !value, // remove ! to see that errors will be gone
AssertType(prop, "boolean");
AssertType(!value, "boolean");
AssertType(value, "boolean");

    run: () => { //replace arrow function with regular function to see that errors will be gone
AssertType(run, "() => union");
AssertType(() => { //replace arrow function with regular function to see that errors will be gone        // comment next line or remove "()" to see that errors will be gone        if(!a.b()) { return 'special'; }        return 'default';    }, "() => union");

        // comment next line or remove "()" to see that errors will be gone
        if(!a.b()) { 
AssertType(!a.b(), "boolean");
return 'special'; 

AssertType(a.b(), "any");

AssertType(a.b, "any");

AssertType('special', "string");
}

AssertType('default', "string");
        return 'default';
    }
}); 


