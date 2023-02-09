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

// === tests/cases/conformance/types/objectTypeLiteral/methodSignatures/functionLiterals.ts ===
declare function AssertType(value:any, type:string):void;
// PropName<TypeParamList>(ParamList):ReturnType is equivalent to PropName: { <TypeParamList>(ParamList): ReturnType }

let b: {
AssertType(b, "{ func1(number): number; func2: (number) => number; func3: (number) => number; }");

    func1(x: number): number;         // Method signature
AssertType(func1, "(number) => number");
AssertType(x, "number");

    func2: (x: number) => number;     // Function type literal
AssertType(func2, "(number) => number");
AssertType(x, "number");

    func3: { (x: number): number };   // Object type literal
AssertType(func3, "(number) => number");
AssertType(x, "number");
}

// no errors
b.func1 = b.func2;
AssertType(b.func1 = b.func2, "(number) => number");
AssertType(b.func1, "(number) => number");
AssertType(b.func2, "(number) => number");

b.func1 = b.func3;
AssertType(b.func1 = b.func3, "(number) => number");
AssertType(b.func1, "(number) => number");
AssertType(b.func3, "(number) => number");

b.func2 = b.func1;
AssertType(b.func2 = b.func1, "(number) => number");
AssertType(b.func2, "(number) => number");
AssertType(b.func1, "(number) => number");

b.func2 = b.func3;
AssertType(b.func2 = b.func3, "(number) => number");
AssertType(b.func2, "(number) => number");
AssertType(b.func3, "(number) => number");

b.func3 = b.func1;
AssertType(b.func3 = b.func1, "(number) => number");
AssertType(b.func3, "(number) => number");
AssertType(b.func1, "(number) => number");

b.func3 = b.func2;
AssertType(b.func3 = b.func2, "(number) => number");
AssertType(b.func3, "(number) => number");
AssertType(b.func2, "(number) => number");

let c: {
AssertType(c, "{ func4(number): number; func4(string): string; func5: { (number): number; (string): string; }; }");

    func4(x: number): number;
AssertType(func4, "{ (number): number; (string): string; }");
AssertType(x, "number");

    func4(s: string): string;
AssertType(func4, "{ (number): number; (string): string; }");
AssertType(s, "string");

    func5: {
AssertType(func5, "{ (number): number; (string): string; }");

        (x: number): number;
AssertType(x, "number");

        (s: string): string;
AssertType(s, "string");

    };
};

// no errors
c.func4 = c.func5;
AssertType(c.func4 = c.func5, "{ (number): number; (string): string; }");
AssertType(c.func4, "{ (number): number; (string): string; }");
AssertType(c.func5, "{ (number): number; (string): string; }");

c.func5 = c.func4;
AssertType(c.func5 = c.func4, "{ (number): number; (string): string; }");
AssertType(c.func5, "{ (number): number; (string): string; }");
AssertType(c.func4, "{ (number): number; (string): string; }");

// generic versions
let b2: {
AssertType(b2, "{ func1<T>(T): number; func2: <T>(T) => number; func3: <T>(T) => number; }");

    func1<T>(x: T): number;         // Method signature
AssertType(func1, "<T>(T) => number");
AssertType(x, "T");

    func2: <T>(x: T) => number;     // Function type literal
AssertType(func2, "<T>(T) => number");
AssertType(x, "T");

    func3: { <T>(x: T): number };   // Object type literal
AssertType(func3, "<T>(T) => number");
AssertType(x, "T");
}

// no errors
b2.func1 = b2.func2;
AssertType(b2.func1 = b2.func2, "<T>(T) => number");
AssertType(b2.func1, "<T>(T) => number");
AssertType(b2.func2, "<T>(T) => number");

b2.func1 = b2.func3;
AssertType(b2.func1 = b2.func3, "<T>(T) => number");
AssertType(b2.func1, "<T>(T) => number");
AssertType(b2.func3, "<T>(T) => number");

b2.func2 = b2.func1;
AssertType(b2.func2 = b2.func1, "<T>(T) => number");
AssertType(b2.func2, "<T>(T) => number");
AssertType(b2.func1, "<T>(T) => number");

b2.func2 = b2.func3;
AssertType(b2.func2 = b2.func3, "<T>(T) => number");
AssertType(b2.func2, "<T>(T) => number");
AssertType(b2.func3, "<T>(T) => number");

b2.func3 = b2.func1;
AssertType(b2.func3 = b2.func1, "<T>(T) => number");
AssertType(b2.func3, "<T>(T) => number");
AssertType(b2.func1, "<T>(T) => number");

b2.func3 = b2.func2;
AssertType(b2.func3 = b2.func2, "<T>(T) => number");
AssertType(b2.func3, "<T>(T) => number");
AssertType(b2.func2, "<T>(T) => number");

let c2: {
AssertType(c2, "{ func4<T>(T): number; func4<T>(T): string; func5: { <T>(T): number; <T>(T): string; }; }");

    func4<T>(x: T): number;
AssertType(func4, "{ <T>(T): number; <T>(T): string; }");
AssertType(x, "T");

    func4<T>(s: T): string;
AssertType(func4, "{ <T>(T): number; <T>(T): string; }");
AssertType(s, "T");

    func5: {
AssertType(func5, "{ <T>(T): number; <T>(T): string; }");

        <T>(x: T): number;
AssertType(x, "T");

        <T>(s: T): string;
AssertType(s, "T");

    };
};

// no errors
c2.func4 = c2.func5;
AssertType(c2.func4 = c2.func5, "{ <T>(T): number; <T>(T): string; }");
AssertType(c2.func4, "{ <T>(T): number; <T>(T): string; }");
AssertType(c2.func5, "{ <T>(T): number; <T>(T): string; }");

c2.func5 = c2.func4;
AssertType(c2.func5 = c2.func4, "{ <T>(T): number; <T>(T): string; }");
AssertType(c2.func5, "{ <T>(T): number; <T>(T): string; }");
AssertType(c2.func4, "{ <T>(T): number; <T>(T): string; }");


