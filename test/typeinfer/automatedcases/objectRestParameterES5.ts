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

// === tests/cases/conformance/types/rest/objectRestParameterES5.ts ===
declare function AssertType(value:any, type:string):void;
function cloneAgain({ a, ...clone }: { a: number, b: string }): void {
}

declare function suddenly(f: (a: { x: { z, ka }, y: string }) => void);
suddenly(({ x: a, ...rest }) => rest.y);
AssertType(suddenly(({ x: a, ...rest }) => rest.y), "any");
AssertType(suddenly, "(({ x: { z: any; ka: any; }; y: string; }) => void) => any");
AssertType(({ x: a, ...rest }) => rest.y, "({ x: { z: any; ka: any; }; y: string; }) => string");
AssertType(x, "any");
AssertType(a, "{ z: any; ka: any; }");
AssertType(rest, "{ y: string; }");
AssertType(rest.y, "string");

suddenly(({ x: { z = 12, ...nested }, ...rest } = { x: { z: 1, ka: 1 }, y: 'noo' }) => rest.y + nested.ka);
AssertType(suddenly(({ x: { z = 12, ...nested }, ...rest } = { x: { z: 1, ka: 1 }, y: 'noo' }) => rest.y + nested.ka), "any");
AssertType(suddenly, "(({ x: { z: any; ka: any; }; y: string; }) => void) => any");
AssertType(({ x: { z = 12, ...nested }, ...rest } = { x: { z: 1, ka: 1 }, y: 'noo' }) => rest.y + nested.ka, "(?{ x: { z: any; ka: any; }; y: string; }) => string");
AssertType(x, "any");
AssertType(z, "any");
AssertType(12, "int");
AssertType(nested, "{ ka: any; }");
AssertType(rest, "{ y: string; }");
AssertType({ x: { z: 1, ka: 1 }, y: 'noo' }, "{ x: { z: number; ka: number; }; y: string; }");
AssertType(x, "{ z: number; ka: number; }");
AssertType({ z: 1, ka: 1 }, "{ z: number; ka: number; }");
AssertType(z, "number");
AssertType(1, "int");
AssertType(ka, "number");
AssertType(1, "int");
AssertType(y, "string");
AssertType('noo', "string");
AssertType(rest.y + nested.ka, "string");
AssertType(rest.y, "string");
AssertType(nested.ka, "any");

class C {
    m({ a, ...clone }: { a: number, b: string}): void {
        // actually, never mind, don't clone
    }
    set p({ a, ...clone }: { a: number, b: string}) {
        // actually, never mind, don't clone
    }
}
function foobar({ bar={}, ...opts }: any = {}) {
}
foobar();
AssertType(foobar(), "void");
AssertType(foobar, "(?any) => void");

foobar({ baz: 'hello' });
AssertType(foobar({ baz: 'hello' }), "void");
AssertType(foobar, "(?any) => void");
AssertType({ baz: 'hello' }, "{ baz: string; }");
AssertType(baz, "string");
AssertType('hello', "string");

foobar({ bar: { greeting: 'hello' } });
AssertType(foobar({ bar: { greeting: 'hello' } }), "void");
AssertType(foobar, "(?any) => void");
AssertType({ bar: { greeting: 'hello' } }, "{ bar: { greeting: string; }; }");
AssertType(bar, "{ greeting: string; }");
AssertType({ greeting: 'hello' }, "{ greeting: string; }");
AssertType(greeting, "string");
AssertType('hello', "string");


