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

// === tests/cases/compiler/contextSensitiveReturnTypeInference.ts ===
declare function AssertType(value:any, type:string):void;
// Repro from #34849

interface IData {
  bar: boolean
}

declare function test<TDependencies>(
  getter: (deps: TDependencies, data: IData) => any,
  deps: TDependencies,
): any 

const DEPS = {
AssertType(DEPS, "{ foo: number; }");
AssertType({  foo: 1}, "{ foo: number; }");

  foo: 1
AssertType(foo, "number");
AssertType(1, "int");
}

test(
AssertType(test, "<TDependencies>((TDependencies, IData) => any, TDependencies) => any");
AssertType(test(  (deps, data) => ({    fn1: function() { return deps.foo },    fn2: data.bar  }),  DEPS), "any");

  (deps, data) => ({
AssertType(deps, "{ foo: number; }");
AssertType(data, "IData");
AssertType(({    fn1: function() { return deps.foo },    fn2: data.bar  }), "{ fn1: () => number; fn2: boolean; }");
AssertType({    fn1: function() { return deps.foo },    fn2: data.bar  }, "{ fn1: () => number; fn2: boolean; }");
AssertType((deps, data) => ({    fn1: function() { return deps.foo },    fn2: data.bar  }), "({ foo: number; }, IData) => { fn1: () => number; fn2: boolean; }");

    fn1: function() { 
AssertType(fn1, "() => number");
AssertType(function() { return deps.foo }, "() => number");
AssertType(deps.foo, "number");
return deps.foo },

    fn2: data.bar
AssertType(fn2, "boolean");
AssertType(data.bar, "boolean");

  }),
  DEPS
AssertType(DEPS, "{ foo: number; }");

);

test(
AssertType(test, "<TDependencies>((TDependencies, IData) => any, TDependencies) => any");
AssertType(test(  (deps: typeof DEPS, data) => ({    fn1: function() { return deps.foo },    fn2: data.bar  }),  DEPS), "any");

  (deps: typeof DEPS, data) => ({
AssertType(deps, "{ foo: number; }");
AssertType(DEPS, "{ foo: number; }");
AssertType(data, "IData");
AssertType(({    fn1: function() { return deps.foo },    fn2: data.bar  }), "{ fn1: () => number; fn2: boolean; }");
AssertType({    fn1: function() { return deps.foo },    fn2: data.bar  }, "{ fn1: () => number; fn2: boolean; }");
AssertType((deps: typeof DEPS, data) => ({    fn1: function() { return deps.foo },    fn2: data.bar  }), "(typeof DEPS, IData) => { fn1: () => number; fn2: boolean; }");

    fn1: function() { 
AssertType(fn1, "() => number");
AssertType(function() { return deps.foo }, "() => number");
AssertType(deps.foo, "number");
return deps.foo },

    fn2: data.bar
AssertType(fn2, "boolean");
AssertType(data.bar, "boolean");

  }),
  DEPS
AssertType(DEPS, "{ foo: number; }");

);

test(
AssertType(test(  (deps, data) => ({    fn1: () => deps.foo,    fn2: data.bar  }),  DEPS), "any");
AssertType(test, "<TDependencies>((TDependencies, IData) => any, TDependencies) => any");

  (deps, data) => ({
AssertType((deps, data) => ({    fn1: () => deps.foo,    fn2: data.bar  }), "({ foo: number; }, IData) => { fn1: () => number; fn2: boolean; }");
AssertType(deps, "{ foo: number; }");
AssertType(data, "IData");
AssertType(({    fn1: () => deps.foo,    fn2: data.bar  }), "{ fn1: () => number; fn2: boolean; }");
AssertType({    fn1: () => deps.foo,    fn2: data.bar  }, "{ fn1: () => number; fn2: boolean; }");

    fn1: () => deps.foo,
AssertType(fn1, "() => number");
AssertType(() => deps.foo, "() => number");
AssertType(deps.foo, "number");

    fn2: data.bar
AssertType(fn2, "boolean");
AssertType(data.bar, "boolean");

  }),
  DEPS
AssertType(DEPS, "{ foo: number; }");

);

test(
AssertType(test, "<TDependencies>((TDependencies, IData) => any, TDependencies) => any");
AssertType(test(  (deps, data) => {    return {      fn1() { return deps.foo },      fn2: data.bar    }  },  DEPS), "any");

  (deps, data) => {
AssertType(deps, "{ foo: number; }");
AssertType(data, "IData");
AssertType((deps, data) => {    return {      fn1() { return deps.foo },      fn2: data.bar    }  }, "({ foo: number; }, IData) => { fn1(): number; fn2: boolean; }");

AssertType({      fn1() { return deps.foo },      fn2: data.bar    }, "{ fn1(): number; fn2: boolean; }");
    return {

      fn1() { 
AssertType(fn1, "() => number");
AssertType(deps.foo, "number");
return deps.foo },

      fn2: data.bar
AssertType(fn2, "boolean");
AssertType(data.bar, "boolean");
    }
  },
  DEPS
AssertType(DEPS, "{ foo: number; }");

);

test(
AssertType(test, "<TDependencies>((TDependencies, IData) => any, TDependencies) => any");
AssertType(test(  (deps) => ({    fn1() { return deps.foo },    fn2: 1  }),  DEPS), "any");

  (deps) => ({
AssertType(deps, "{ foo: number; }");
AssertType(({    fn1() { return deps.foo },    fn2: 1  }), "{ fn1(): number; fn2: number; }");
AssertType({    fn1() { return deps.foo },    fn2: 1  }, "{ fn1(): number; fn2: number; }");
AssertType((deps) => ({    fn1() { return deps.foo },    fn2: 1  }), "({ foo: number; }) => { fn1(): number; fn2: number; }");

    fn1() { 
AssertType(fn1, "() => number");
AssertType(deps.foo, "number");
return deps.foo },

    fn2: 1
AssertType(fn2, "number");
AssertType(1, "int");

  }),
  DEPS
AssertType(DEPS, "{ foo: number; }");

);


