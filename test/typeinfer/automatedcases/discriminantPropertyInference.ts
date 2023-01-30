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

// === tests/cases/compiler/discriminantPropertyInference.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #41759

type DiscriminatorTrue = {
    disc: true;
    cb: (x: string) => void;
}

type DiscriminatorFalse = {
    disc?: false;
    cb: (x: number) => void;
}

type Props = DiscriminatorTrue | DiscriminatorFalse;

declare function f(options: DiscriminatorTrue | DiscriminatorFalse): any;

// simple inference
f({
AssertType(f({    disc: true,    cb: s => parseInt(s)}), "any");
AssertType(f, "(union) => any");
AssertType({    disc: true,    cb: s => parseInt(s)}, "{ disc: true; cb: (string) => number; }");

    disc: true,
AssertType(disc, "boolean");
AssertType(true, "boolean");

    cb: s => parseInt(s)
AssertType(cb, "(string) => number");
AssertType(s => parseInt(s), "(string) => number");
AssertType(s, "string");
AssertType(parseInt(s), "number");
AssertType(parseInt, "(string, ?union) => number");
AssertType(s, "string");

});

// simple inference
f({
AssertType(f({    disc: false,    cb: n => n.toFixed()}), "any");
AssertType(f, "(union) => any");
AssertType({    disc: false,    cb: n => n.toFixed()}, "{ disc: false; cb: (number) => string; }");

    disc: false,
AssertType(disc, "boolean");
AssertType(false, "boolean");

    cb: n => n.toFixed()
AssertType(cb, "(number) => string");
AssertType(n => n.toFixed(), "(number) => string");
AssertType(n, "number");
AssertType(n.toFixed(), "string");
AssertType(n.toFixed, "(?union) => string");

});

// simple inference when strict-null-checks are enabled
f({
AssertType(f({    disc: undefined,    cb: n => n.toFixed()}), "any");
AssertType(f, "(union) => any");
AssertType({    disc: undefined,    cb: n => n.toFixed()}, "{ disc: undefined; cb: (number) => string; }");

    disc: undefined,
AssertType(disc, "undefined");
AssertType(undefined, "undefined");

    cb: n => n.toFixed()
AssertType(cb, "(number) => string");
AssertType(n => n.toFixed(), "(number) => string");
AssertType(n, "number");
AssertType(n.toFixed(), "string");
AssertType(n.toFixed, "(?union) => string");

});

// requires checking type information since discriminator is missing from object
f({
AssertType(f({    cb: n => n.toFixed()}), "any");
AssertType(f, "(union) => any");
AssertType({    cb: n => n.toFixed()}, "{ cb: (number) => string; }");

    cb: n => n.toFixed()
AssertType(cb, "(number) => string");
AssertType(n => n.toFixed(), "(number) => string");
AssertType(n, "number");
AssertType(n.toFixed(), "string");
AssertType(n.toFixed, "(?union) => string");

});


