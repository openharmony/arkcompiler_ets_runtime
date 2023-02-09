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

// === tests/cases/compiler/commentsAfterSpread.ts ===
declare function AssertType(value:any, type:string):void;
const identity = (a) => a;
AssertType(identity, "(any) => any");
AssertType((a) => a, "(any) => any");
AssertType(a, "any");
AssertType(a, "any");

const a = {
AssertType(a, "any");
AssertType({  .../*#__PURE__*/identity({    b: 1  })}, "any");

  .../*#__PURE__*/identity({
AssertType(identity({    b: 1  }), "any");
AssertType(identity, "(any) => any");
AssertType({    b: 1  }, "{ b: number; }");

    b: 1
AssertType(b, "number");
AssertType(1, "int");

  })
};

const b = {
AssertType(b, "any");
AssertType({  ...  /*#__PURE__*/identity({    b: 1  })}, "any");

  ...
  /*#__PURE__*/identity({
AssertType(identity({    b: 1  }), "any");
AssertType(identity, "(any) => any");
AssertType({    b: 1  }, "{ b: number; }");

    b: 1
AssertType(b, "number");
AssertType(1, "int");

  })
};

const c = {
AssertType(c, "any");
AssertType({  ...  /*#__PURE__*/  identity({    b: 1  })}, "any");

  ...
  /*#__PURE__*/
  identity({
AssertType(identity({    b: 1  }), "any");
AssertType(identity, "(any) => any");
AssertType({    b: 1  }, "{ b: number; }");

    b: 1
AssertType(b, "number");
AssertType(1, "int");

  })
};

const d = {
AssertType(d, "any");
AssertType({  .../*#__PURE__*/  identity({    b: 1  })}, "any");

  .../*#__PURE__*/
  identity({
AssertType(identity({    b: 1  }), "any");
AssertType(identity, "(any) => any");
AssertType({    b: 1  }, "{ b: number; }");

    b: 1
AssertType(b, "number");
AssertType(1, "int");

  })
};

function e (.../* comment e */args) {
  const [ea, eb] = [.../* comment eab */args];
AssertType(ea, "any");
AssertType(eb, "any");
AssertType([.../* comment eab */args], "any[]");
AssertType(.../* comment eab */args, "any");
AssertType(args, "any[]");

AssertType(args.length, "number");
  return args.length;
}

function f (
  first,
  .../* comment f */rest
) {
AssertType(rest.length, "number");
  return rest.length;
}

function g (
  first,
  .../* comment g */
  rest
) {
  const [ga, gb] = [.../* comment gab */
AssertType(ga, "any");
AssertType(gb, "any");
AssertType([.../* comment gab */    rest  ], "any[]");
AssertType(.../* comment gab */    rest, "any");

    rest
AssertType(rest, "any[]");

  ]
AssertType(rest.length, "number");
  return rest.length;
}

const h = (.../* comment h */args) => args.length;
AssertType(h, "(...any[]) => number");
AssertType((.../* comment h */args) => args.length, "(...any[]) => number");
AssertType(args, "any[]");
AssertType(args.length, "number");

const i = (
AssertType(i, "(any, ...any[]) => number");
AssertType((  first, .../* comment i */rest) => rest.length, "(any, ...any[]) => number");

  first, .../* comment i */rest
AssertType(first, "any");
AssertType(rest, "any[]");

) => rest.length;
AssertType(rest.length, "number");

const j = (
AssertType(j, "(any, ...any[]) => number");
AssertType((  first,  .../* comment j */  rest) => rest.length, "(any, ...any[]) => number");

  first,
AssertType(first, "any");

  .../* comment j */
  rest
AssertType(rest, "any[]");

) => rest.length;
AssertType(rest.length, "number");

function k ({
  first,
  .../* comment k */rest
}) {
  const { ka, kb, .../* comment kr */remaining } = rest;
AssertType(ka, "any");
AssertType(kb, "any");
AssertType(remaining, "{ [string]: any; }");
AssertType(rest, "{ [string]: any; }");

  const {
    kc,
AssertType(kc, "any");

    kd,
AssertType(kd, "any");

    .../* comment kr2 */
    remaining2
AssertType(remaining2, "{ [string]: any; }");

  } = rest;
AssertType(rest, "{ [string]: any; }");

AssertType({ .../* comment return k */ remaining }, "{ [string]: any; }");
AssertType(remaining, "{ [string]: any; }");
  return { .../* comment return k */ remaining };
}

