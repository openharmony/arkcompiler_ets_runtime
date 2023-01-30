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

// === tests/cases/conformance/declarationEmit/lit.ts ===
declare function AssertType(value:any, type:string):void;
class ClassMapDirective {}

export type {ClassMapDirective};

export const directive =
AssertType(directive, "<C>(C) => () => { directive: C; }");

  <C>(class_: C) =>
AssertType(<C>(class_: C) =>  () => ({    directive: class_,  }), "<C>(C) => () => { directive: C; }");
AssertType(class_, "C");

  () => ({
AssertType(() => ({    directive: class_,  }), "() => { directive: C; }");
AssertType(({    directive: class_,  }), "{ directive: C; }");
AssertType({    directive: class_,  }, "{ directive: C; }");

    directive: class_,
AssertType(directive, "C");
AssertType(class_, "C");

  });

export const classMap = directive(ClassMapDirective);
AssertType(classMap, "() => { directive: typeof ClassMapDirective; }");
AssertType(directive(ClassMapDirective), "() => { directive: typeof ClassMapDirective; }");
AssertType(directive, "<C>(C) => () => { directive: C; }");
AssertType(ClassMapDirective, "typeof ClassMapDirective");


