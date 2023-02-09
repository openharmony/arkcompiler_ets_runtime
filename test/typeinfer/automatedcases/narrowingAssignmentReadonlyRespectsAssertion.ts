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

// === tests/cases/compiler/narrowingAssignmentReadonlyRespectsAssertion.ts ===
declare function AssertType(value:any, type:string):void;
// https://github.com/microsoft/TypeScript/issues/41984

interface TestCase<T extends string | number> {
  readonly val1: T | ReadonlyArray<T>;
  readonly val2: ReadonlyArray<T>;
}

interface MultiCaseFixture<T> {
  cases: T[];
}

function subDataFunc(): TestCase<string | number>[] {
AssertType([      { val1: "a", val2: ["a", "b", "c"] },      { val1: 2, val2: [1, 2, 3] },      { val1: ["a", "z"], val2: ["x", "y", "z"] },      { val1: [5, 10], val2: [10, 100, 1000] },  ], "(union)[]");
  return [

      { val1: "a", val2: ["a", "b", "c"] },
AssertType({ val1: "a", val2: ["a", "b", "c"] }, "{ val1: string; val2: string[]; }");
AssertType(val1, "string");
AssertType("a", "string");
AssertType(val2, "string[]");
AssertType(["a", "b", "c"], "string[]");
AssertType("a", "string");
AssertType("b", "string");
AssertType("c", "string");

      { val1: 2, val2: [1, 2, 3] },
AssertType({ val1: 2, val2: [1, 2, 3] }, "{ val1: number; val2: number[]; }");
AssertType(val1, "number");
AssertType(2, "int");
AssertType(val2, "number[]");
AssertType([1, 2, 3], "number[]");
AssertType(1, "int");
AssertType(2, "int");
AssertType(3, "int");

      { val1: ["a", "z"], val2: ["x", "y", "z"] },
AssertType({ val1: ["a", "z"], val2: ["x", "y", "z"] }, "{ val1: string[]; val2: string[]; }");
AssertType(val1, "string[]");
AssertType(["a", "z"], "string[]");
AssertType("a", "string");
AssertType("z", "string");
AssertType(val2, "string[]");
AssertType(["x", "y", "z"], "string[]");
AssertType("x", "string");
AssertType("y", "string");
AssertType("z", "string");

      { val1: [5, 10], val2: [10, 100, 1000] },
AssertType({ val1: [5, 10], val2: [10, 100, 1000] }, "{ val1: number[]; val2: number[]; }");
AssertType(val1, "number[]");
AssertType([5, 10], "number[]");
AssertType(5, "int");
AssertType(10, "int");
AssertType(val2, "number[]");
AssertType([10, 100, 1000], "number[]");
AssertType(10, "int");
AssertType(100, "int");
AssertType(1000, "int");

  ];
}

function dataFunc<T>(subFunc: () => T[]): MultiCaseFixture<T> {
AssertType({ cases: subFunc() }, "{ cases: T[]; }");
AssertType(cases, "T[]");
AssertType(subFunc(), "T[]");
AssertType(subFunc, "() => T[]");
  return { cases: subFunc() };
}

function testFunc() {
  const fixture = dataFunc<TestCase<string | number>>(subDataFunc);
AssertType(fixture, "MultiCaseFixture<TestCase<union>>");
AssertType(dataFunc<TestCase<string | number>>(subDataFunc), "MultiCaseFixture<TestCase<union>>");
AssertType(dataFunc, "<T>(() => T[]) => MultiCaseFixture<T>");
AssertType(subDataFunc, "() => TestCase<union>[]");

  fixture.cases.forEach(({ val1, val2 }) => {
AssertType(fixture.cases.forEach(({ val1, val2 }) => {      if (Array.isArray(val1)) {          // This should retain val1 as being an array          const reversedVal1 = val1.slice().reverse();          console.log(reversedVal1);      } else {          console.log(val1);      }      console.log(val2);  }), "void");
AssertType(fixture.cases.forEach, "((TestCase<union>, number, TestCase<union>[]) => void, ?any) => void");
AssertType(fixture.cases, "TestCase<union>[]");
AssertType(({ val1, val2 }) => {      if (Array.isArray(val1)) {          // This should retain val1 as being an array          const reversedVal1 = val1.slice().reverse();          console.log(reversedVal1);      } else {          console.log(val1);      }      console.log(val2);  }, "(TestCase<union>) => void");
AssertType(val1, "union");
AssertType(val2, "readonly (union)[]");

      if (Array.isArray(val1)) {
AssertType(Array.isArray(val1), "boolean");
AssertType(Array.isArray, "(any) => arg is any[]");
AssertType(val1, "union");

          // This should retain val1 as being an array
          const reversedVal1 = val1.slice().reverse();
AssertType(reversedVal1, "any[]");
AssertType(val1.slice().reverse(), "any[]");
AssertType(val1.slice().reverse, "() => any[]");
AssertType(val1.slice(), "any[]");
AssertType(val1.slice, "(?number, ?number) => any[]");

          console.log(reversedVal1);
AssertType(console.log(reversedVal1), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(reversedVal1, "any[]");

      } else {
          console.log(val1);
AssertType(console.log(val1), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(val1, "union");
      }
      console.log(val2);
AssertType(console.log(val2), "void");
AssertType(console.log, "(...any[]) => void");
AssertType(val2, "readonly (union)[]");

  });
}

testFunc();
AssertType(testFunc(), "void");
AssertType(testFunc, "() => void");


