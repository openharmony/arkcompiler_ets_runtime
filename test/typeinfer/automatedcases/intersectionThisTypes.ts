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

// === tests/cases/conformance/types/intersection/intersectionThisTypes.ts ===
declare function AssertType(value:any, type:string):void;
interface Thing1 {
    a: number;
    self(): this;
}

interface Thing2 {
    b: number;
    me(): this;
}

type Thing3 = Thing1 & Thing2;
type Thing4 = Thing3 & string[];

function f1(t: Thing3) {
    t = t.self();
AssertType(t = t.self(), "Thing3");
AssertType(t, "Thing3");
AssertType(t.self(), "Thing3");
AssertType(t.self, "() => Thing3");

    t = t.me().self().me();
AssertType(t = t.me().self().me(), "Thing3");
AssertType(t, "Thing3");
AssertType(t.me().self().me(), "Thing3");
AssertType(t.me().self().me, "() => Thing3");
AssertType(t.me().self(), "Thing3");
AssertType(t.me().self, "() => Thing3");
AssertType(t.me(), "Thing3");
AssertType(t.me, "() => Thing3");
}

interface Thing5 extends Thing4 {
    c: string;
}

function f2(t: Thing5) {
    t = t.self();
AssertType(t = t.self(), "Thing5");
AssertType(t, "Thing5");
AssertType(t.self(), "Thing5");
AssertType(t.self, "() => Thing5");

    t = t.me().self().me();
AssertType(t = t.me().self().me(), "Thing5");
AssertType(t, "Thing5");
AssertType(t.me().self().me(), "Thing5");
AssertType(t.me().self().me, "() => Thing5");
AssertType(t.me().self(), "Thing5");
AssertType(t.me().self, "() => Thing5");
AssertType(t.me(), "Thing5");
AssertType(t.me, "() => Thing5");
}

interface Component {
    extend<T>(props: T): this & T;
}

interface Label extends Component {
    title: string;
}

function test(label: Label) {
    const extended = label.extend({ id: 67 }).extend({ tag: "hello" });
AssertType(extended, "Label & { id: number; } & { tag: string; }");
AssertType(label.extend({ id: 67 }).extend({ tag: "hello" }), "Label & { id: number; } & { tag: string; }");
AssertType(label.extend({ id: 67 }).extend, "<T>(T) => Label & { id: number; } & T");
AssertType(label.extend({ id: 67 }), "Label & { id: number; }");
AssertType(label.extend, "<T>(T) => Label & T");
AssertType({ id: 67 }, "{ id: number; }");
AssertType(id, "number");
AssertType(67, "int");
AssertType({ tag: "hello" }, "{ tag: string; }");
AssertType(tag, "string");
AssertType("hello", "string");

    extended.id;  // Ok
AssertType(extended.id, "number");

    extended.tag;  // Ok
AssertType(extended.tag, "string");
}


