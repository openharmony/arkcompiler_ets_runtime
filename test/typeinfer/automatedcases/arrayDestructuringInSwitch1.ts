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

// === tests/cases/compiler/arrayDestructuringInSwitch1.ts ===
declare function AssertType(value:any, type:string):void;
export type Expression = BooleanLogicExpression | 'true' | 'false';
export type BooleanLogicExpression = ['and', ...Expression[]] | ['not', Expression];

export function evaluate(expression: Expression): boolean {
  if (Array.isArray(expression)) {
AssertType(Array.isArray(expression), "boolean");
AssertType(Array.isArray, "(any) => arg is any[]");
AssertType(expression, "Expression");

    const [operator, ...operands] = expression;
AssertType(operator, "union");
AssertType(operands, "union");
AssertType(expression, "BooleanLogicExpression");

    switch (operator) {
AssertType(operator, "union");

      case 'and': {
AssertType('and', "string");

AssertType(operands.every((child) => evaluate(child)), "boolean");
AssertType(operands.every, "union");
AssertType((child) => evaluate(child), "(Expression) => boolean");
AssertType(child, "Expression");
AssertType(evaluate(child), "boolean");
AssertType(evaluate, "(Expression) => boolean");
AssertType(child, "Expression");
        return operands.every((child) => evaluate(child));
      }
      case 'not': {
AssertType('not', "string");

AssertType(!evaluate(operands[0]), "boolean");
AssertType(evaluate(operands[0]), "boolean");
AssertType(evaluate, "(Expression) => boolean");
AssertType(operands[0], "Expression");
AssertType(operands, "union");
AssertType(0, "int");
        return !evaluate(operands[0]);
      }
      default: {
        throw new Error(`${operator} is not a supported operator`);
AssertType(new Error(`${operator} is not a supported operator`), "Error");
AssertType(Error, "ErrorConstructor");
AssertType(`${operator} is not a supported operator`, "string");
AssertType(operator, "never");
      }
    }
  } else {
AssertType(expression === 'true', "boolean");
AssertType(expression, "union");
AssertType('true', "string");
    return expression === 'true';
  }
}

