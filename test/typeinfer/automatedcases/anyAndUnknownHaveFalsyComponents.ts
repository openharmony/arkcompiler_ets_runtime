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

// === tests/cases/compiler/anyAndUnknownHaveFalsyComponents.ts ===
declare function AssertType(value:any, type:string):void;
declare let x1: any;
AssertType(x1, "any");

const y1 = x1 && 3;
AssertType(y1, "any");
AssertType(x1 && 3, "any");
AssertType(x1, "any");
AssertType(3, "int");

// #39113
declare let isTreeHeader1: any;
AssertType(isTreeHeader1, "any");

function foo1() {
AssertType({    display: "block",    ...(isTreeHeader1 && {      display: "flex",    })  }, "any");
  return {

    display: "block",
AssertType(display, "string");
AssertType("block", "string");

    ...(isTreeHeader1 && {
AssertType((isTreeHeader1 && {      display: "flex",    }), "any");
AssertType(isTreeHeader1 && {      display: "flex",    }, "any");
AssertType(isTreeHeader1, "any");
AssertType({      display: "flex",    }, "{ display: string; }");

      display: "flex",
AssertType(display, "string");
AssertType("flex", "string");

    })
  };
}

declare let x2: unknown;
AssertType(x2, "unknown");

const y2 = x2 && 3;
AssertType(y2, "unknown");
AssertType(x2 && 3, "unknown");
AssertType(x2, "unknown");
AssertType(3, "int");

// #39113
declare let isTreeHeader2: unknown;
AssertType(isTreeHeader2, "unknown");

function foo2() {
AssertType({    display: "block",    ...(isTreeHeader1 && {      display: "flex",    })  }, "any");
  return {

    display: "block",
AssertType(display, "string");
AssertType("block", "string");

    ...(isTreeHeader1 && {
AssertType((isTreeHeader1 && {      display: "flex",    }), "any");
AssertType(isTreeHeader1 && {      display: "flex",    }, "any");
AssertType(isTreeHeader1, "any");
AssertType({      display: "flex",    }, "{ display: string; }");

      display: "flex",
AssertType(display, "string");
AssertType("flex", "string");

    })
  };
}


