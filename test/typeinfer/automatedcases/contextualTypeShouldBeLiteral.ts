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

// === tests/cases/compiler/contextualTypeShouldBeLiteral.ts ===
declare function AssertType(value:any, type:string):void;
interface X {
    type: 'x';
    value: string;
    method(): void;
}

interface Y {
    type: 'y';
    value: 'none' | 'done';
    method(): void;
}

function foo(bar: X | Y) { }

foo({
AssertType(foo({    type: 'y',    value: 'done',    method() {        this;        this.type;        this.value;    }}), "void");
AssertType(foo, "(union) => void");
AssertType({    type: 'y',    value: 'done',    method() {        this;        this.type;        this.value;    }}, "{ type: "y"; value: "done"; method(): void; }");

    type: 'y',
AssertType(type, "string");
AssertType('y', "string");

    value: 'done',
AssertType(value, "string");
AssertType('done', "string");

    method() {
AssertType(method, "() => void");

        this;
AssertType(this, "Y");

        this.type;
AssertType(this.type, "string");
AssertType(this, "Y");

        this.value;
AssertType(this.value, "union");
AssertType(this, "Y");
    }
});

interface X2 {
    type1: 'x';
    value: string;
    method(): void;
}

interface Y2 {
    type2: 'y';
    value: 'none' | 'done';
    method(): void;
}

function foo2(bar: X2 | Y2) { }

foo2({
AssertType(foo2({    type2: 'y',    value: 'done',    method() {        this;        this.value;    }}), "void");
AssertType(foo2, "(union) => void");
AssertType({    type2: 'y',    value: 'done',    method() {        this;        this.value;    }}, "{ type2: "y"; value: "done"; method(): void; }");

    type2: 'y',
AssertType(type2, "string");
AssertType('y', "string");

    value: 'done',
AssertType(value, "string");
AssertType('done', "string");

    method() {
AssertType(method, "() => void");

        this;
AssertType(this, "union");

        this.value;
AssertType(this.value, "string");
AssertType(this, "union");
    }
});

interface X3 {
    type: 'x';
    value: 1 | 2 | 3;
    xtra: number;
}

interface Y3 {
    type: 'y';
    value: 11 | 12 | 13;
    ytra: number;
}

let xy: X3 | Y3 = {
AssertType(xy, "union");
AssertType({    type: 'y',    value: 11,    ytra: 12}, "{ type: "y"; value: 11; ytra: number; }");

    type: 'y',
AssertType(type, "string");
AssertType('y', "string");

    value: 11,
AssertType(value, "int");
AssertType(11, "int");

    ytra: 12
AssertType(ytra, "number");
AssertType(12, "int");

};

xy;
AssertType(xy, "Y3");


interface LikeA {
    x: 'x';
    y: 'y';
    value: string;
    method(): void;
}

interface LikeB {
    x: 'xx';
    y: 'yy';
    value: number;
    method(): void;
}

let xyz: LikeA | LikeB = {
AssertType(xyz, "union");
AssertType({    x: 'x',    y: 'y',    value: "foo",    method() {        this;        this.x;        this.y;        this.value;    }}, "{ x: "x"; y: "y"; value: string; method(): void; }");

    x: 'x',
AssertType(x, "string");
AssertType('x', "string");

    y: 'y',
AssertType(y, "string");
AssertType('y', "string");

    value: "foo",
AssertType(value, "string");
AssertType("foo", "string");

    method() {
AssertType(method, "() => void");

        this;
AssertType(this, "LikeA");

        this.x;
AssertType(this.x, "string");
AssertType(this, "LikeA");

        this.y;
AssertType(this.y, "string");
AssertType(this, "LikeA");

        this.value;
AssertType(this.value, "string");
AssertType(this, "LikeA");
    }
};

xyz;
AssertType(xyz, "LikeA");

// Repro from #29168

interface TestObject {
  type?: 'object';
  items: {
    [k: string]: TestGeneric;
  };
}

interface TestString {
  type: 'string';
}

type TestGeneric = (TestString | TestObject) & { [k: string]: any; };

const test: TestGeneric = {
AssertType(test, "TestGeneric");
AssertType({  items: {    hello: { type: 'string' },    world: {      items: {        nested: { type: 'string' }      }    }  }}, "{ items: { hello: { type: "string"; }; world: { items: { nested: { type: "string"; }; }; }; }; }");

  items: {
AssertType(items, "{ hello: { type: "string"; }; world: { items: { nested: { type: "string"; }; }; }; }");
AssertType({    hello: { type: 'string' },    world: {      items: {        nested: { type: 'string' }      }    }  }, "{ hello: { type: "string"; }; world: { items: { nested: { type: "string"; }; }; }; }");

    hello: { type: 'string' },
AssertType(hello, "{ type: "string"; }");
AssertType({ type: 'string' }, "{ type: "string"; }");
AssertType(type, "string");
AssertType('string', "string");

    world: {
AssertType(world, "{ items: { nested: { type: "string"; }; }; }");
AssertType({      items: {        nested: { type: 'string' }      }    }, "{ items: { nested: { type: "string"; }; }; }");

      items: {
AssertType(items, "{ nested: { type: "string"; }; }");
AssertType({        nested: { type: 'string' }      }, "{ nested: { type: "string"; }; }");

        nested: { type: 'string' 
AssertType(nested, "{ type: "string"; }");

AssertType({ type: 'string' }, "{ type: "string"; }");

AssertType(type, "string");

AssertType('string', "string");
}
      }
    }
  }
};


