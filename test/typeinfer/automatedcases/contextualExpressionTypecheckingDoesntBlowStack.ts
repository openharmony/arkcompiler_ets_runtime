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

// === tests/cases/compiler/contextualExpressionTypecheckingDoesntBlowStack.ts ===
declare function AssertType(value:any, type:string):void;
// repro for: https://github.com/Microsoft/TypeScript/issues/23661
export interface IValidationError {
    message: string;
}

export default class Operation {
    validateParameters(parameterValues: any) : IValidationError[] | null {
        let result: IValidationError[] | null = null;
AssertType(result, "union");
AssertType(null, "null");
AssertType(null, "null");

        for(const parameterLocation of Object.keys(parameterValues)) {
AssertType(parameterLocation, "string");
AssertType(Object.keys(parameterValues), "string[]");
AssertType(Object.keys, "{ (object): string[]; ({}): string[]; }");
AssertType(parameterValues, "any");

            const parameter: any = (this as any).getParameter();;
AssertType(parameter, "any");
AssertType((this as any).getParameter(), "any");
AssertType((this as any).getParameter, "any");
AssertType((this as any), "any");
AssertType(this as any, "any");
AssertType(this, "this");

            const values = (this as any).getValues();
AssertType(values, "any");
AssertType((this as any).getValues(), "any");
AssertType((this as any).getValues, "any");
AssertType((this as any), "any");
AssertType(this as any, "any");
AssertType(this, "this");

            const innerResult = parameter.validate(values[parameter.oaParameter.name]);
AssertType(innerResult, "any");
AssertType(parameter.validate(values[parameter.oaParameter.name]), "any");
AssertType(parameter.validate, "any");
AssertType(values[parameter.oaParameter.name], "any");
AssertType(values, "any");
AssertType(parameter.oaParameter.name, "any");
AssertType(parameter.oaParameter, "any");

            if(innerResult && innerResult.length > 0) {
AssertType(innerResult && innerResult.length > 0, "any");
AssertType(innerResult, "any");
AssertType(innerResult.length > 0, "boolean");
AssertType(innerResult.length, "any");
AssertType(0, "int");

                // Commenting out this line will fix the problem.
                result = (result || []).concat(innerResult);
AssertType(result = (result || []).concat(innerResult), "IValidationError[]");
AssertType(result, "union");
AssertType((result || []).concat(innerResult), "IValidationError[]");
AssertType((result || []).concat, "{ (...ConcatArray<IValidationError>[]): IValidationError[]; (...(union)[]): IValidationError[]; }");
AssertType((result || []), "IValidationError[]");
AssertType(result || [], "IValidationError[]");
AssertType(result, "union");
AssertType([], "never[]");
AssertType(innerResult, "any");
            }
        }

AssertType(result, "union");
        return result;
    }
}

