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

// === tests/cases/conformance/types/typeParameters/typeParameterLists/typeParametersAvailableInNestedScope3.ts ===
declare function AssertType(value:any, type:string):void;
function foo<T>(v: T) {
    function a<T>(a: T) { 
AssertType(a, "<T>(T) => T");
return a; 

AssertType(a, "T");

AssertType(a, "T");
}

    function b(): T { 
AssertType(b, "() => T");
return v; 

AssertType(v, "T");
}

    function c<T>(v: T) {
AssertType(c, "<T>(T) => { a: <T>(T) => T; b: () => T; }");
AssertType(v, "T");

        function a<T>(a: T) { 
AssertType(a, "<T>(T) => T");
return a; 

AssertType(a, "T");

AssertType(a, "T");
}

        function b(): T { 
AssertType(b, "() => T");
return v; 

AssertType(v, "T");
}

AssertType({ a, b }, "{ a: <T>(T) => T; b: () => T; }");
AssertType(a, "<T>(T) => T");
AssertType(b, "() => T");
        return { a, b };
    }

AssertType({ a, b, c }, "{ a: <T>(T) => T; b: () => T; c: <T>(T) => { a: <T>(T) => T; b: () => T; }; }");
AssertType(a, "<T>(T) => T");
AssertType(b, "() => T");
AssertType(c, "<T>(T) => { a: <T>(T) => T; b: () => T; }");
    return { a, b, c };
}


