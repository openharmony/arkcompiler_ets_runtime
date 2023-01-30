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

// === tests/cases/compiler/reverseMappedTypeDeepDeclarationEmit.ts ===
declare function AssertType(value:any, type:string):void;
export type Validator<T> = NativeTypeValidator<T> | ObjectValidator<T>

export type NativeTypeValidator<T> = (n: any) => T | undefined
export type ObjectValidator<O> = {
  [K in keyof O]: Validator<O[K]> 
}

//native validators
export declare const SimpleStringValidator: NativeTypeValidator<string>;
AssertType(SimpleStringValidator, "NativeTypeValidator<string>");

///object validator function
export declare const ObjValidator: <V>(validatorObj: ObjectValidator<V>) => (o: any) => V;
AssertType(ObjValidator, "<V>(ObjectValidator<V>) => (any) => V");
AssertType(validatorObj, "ObjectValidator<V>");
AssertType(o, "any");

export const test  = {
AssertType(test, "{ Test: { Test1: { Test2: NativeTypeValidator<string>; }; }; }");
AssertType({  Test: {    Test1: {      Test2: SimpleStringValidator    },  }}, "{ Test: { Test1: { Test2: NativeTypeValidator<string>; }; }; }");

  Test: {
AssertType(Test, "{ Test1: { Test2: NativeTypeValidator<string>; }; }");
AssertType({    Test1: {      Test2: SimpleStringValidator    },  }, "{ Test1: { Test2: NativeTypeValidator<string>; }; }");

    Test1: {
AssertType(Test1, "{ Test2: NativeTypeValidator<string>; }");
AssertType({      Test2: SimpleStringValidator    }, "{ Test2: NativeTypeValidator<string>; }");

      Test2: SimpleStringValidator
AssertType(Test2, "NativeTypeValidator<string>");
AssertType(SimpleStringValidator, "NativeTypeValidator<string>");

    },
  }
}

export const validatorFunc = ObjValidator(test);
AssertType(validatorFunc, "(any) => { Test: { Test1: { Test2: string; }; }; }");
AssertType(ObjValidator(test), "(any) => { Test: { Test1: { Test2: string; }; }; }");
AssertType(ObjValidator, "<V>(ObjectValidator<V>) => (any) => V");
AssertType(test, "{ Test: { Test1: { Test2: NativeTypeValidator<string>; }; }; }");

export const outputExample = validatorFunc({
AssertType(outputExample, "{ Test: { Test1: { Test2: string; }; }; }");
AssertType(validatorFunc({  Test: {    Test1: {      Test2: "hi"    },  }}), "{ Test: { Test1: { Test2: string; }; }; }");
AssertType(validatorFunc, "(any) => { Test: { Test1: { Test2: string; }; }; }");
AssertType({  Test: {    Test1: {      Test2: "hi"    },  }}, "{ Test: { Test1: { Test2: string; }; }; }");

  Test: {
AssertType(Test, "{ Test1: { Test2: string; }; }");
AssertType({    Test1: {      Test2: "hi"    },  }, "{ Test1: { Test2: string; }; }");

    Test1: {
AssertType(Test1, "{ Test2: string; }");
AssertType({      Test2: "hi"    }, "{ Test2: string; }");

      Test2: "hi"
AssertType(Test2, "string");
AssertType("hi", "string");

    },
  }
});


