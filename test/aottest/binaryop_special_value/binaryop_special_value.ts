/*
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
 */

declare function print(arg:any):string;

let undf: any = undefined;
let intNum: number = 5;
let doubleNum: number = 1.1;
let falseValue: boolean = false;
let trueValue: boolean = true;
let nullValue: any = null;

// int op undefined
print("=====int op undefined=====")
print(intNum + undf);
print(intNum - undf);
print(intNum * undf);
print(intNum / undf);
print(intNum % undf);
print(intNum < undf);
print(intNum <= undf);
print(intNum > undf);
print(intNum >= undf);
print(intNum == undf);
print(intNum === undf);
print(intNum != undf);
print(intNum !== undf);
print(intNum << undf);
print(intNum >> undf);
print(intNum >>> undf);
print(intNum | undf);
print(intNum ^ undf);

// double op undefined
print("=====double op undefined=====")
print(doubleNum + undf);
print(doubleNum - undf);
print(doubleNum * undf);
print(doubleNum / undf);
print(doubleNum % undf);
print(doubleNum < undf);
print(doubleNum <= undf);
print(doubleNum > undf);
print(doubleNum >= undf);
print(doubleNum == undf);
print(doubleNum === undf);
print(doubleNum != undf);
print(doubleNum !== undf);
print(doubleNum << undf);
print(doubleNum >> undf);
print(doubleNum >>> undf);
print(doubleNum | undf);
print(doubleNum ^ undf);

// false op undefined
print("=====false op undefined=====")
print(falseValue + undf);
print(falseValue - undf);
print(falseValue * undf);
print(falseValue / undf);
print(falseValue % undf);
print(falseValue < undf);
print(falseValue <= undf);
print(falseValue > undf);
print(falseValue >= undf);
print(falseValue == undf);
print(falseValue === undf);
print(falseValue != undf);
print(falseValue !== undf);
print(falseValue << undf);
print(falseValue >> undf);
print(falseValue >>> undf);
print(falseValue | undf);
print(falseValue ^ undf);

// true op undefined
print("=====true op undefined=====")
print(trueValue + undf);
print(trueValue - undf);
print(trueValue * undf);
print(trueValue / undf);
print(trueValue % undf);
print(trueValue < undf);
print(trueValue <= undf);
print(trueValue > undf);
print(trueValue >= undf);
print(trueValue == undf);
print(trueValue === undf);
print(trueValue != undf);
print(trueValue !== undf);
print(trueValue << undf);
print(trueValue >> undf);
print(trueValue >>> undf);
print(trueValue | undf);
print(trueValue ^ undf);

// null op undefined
print("=====null op undefined=====")
print(nullValue + undf);
print(nullValue - undf);
print(nullValue * undf);
print(nullValue / undf);
print(nullValue % undf);
print(nullValue < undf);
print(nullValue <= undf);
print(nullValue > undf);
print(nullValue >= undf);
print(nullValue == undf);
print(nullValue === undf);
print(nullValue != undf);
print(nullValue !== undf);
print(nullValue << undf);
print(nullValue >> undf);
print(nullValue >>> undf);
print(nullValue | undf);
print(nullValue ^ undf);

// int op null
print("=====int op null=====")
print(intNum + nullValue);
print(intNum - nullValue);
print(intNum * nullValue);
print(intNum / nullValue);
print(intNum % nullValue);
print(intNum < nullValue);
print(intNum <= nullValue);
print(intNum > nullValue);
print(intNum >= nullValue);
print(intNum == nullValue);
print(intNum === nullValue);
print(intNum != nullValue);
print(intNum !== nullValue);
print(intNum << nullValue);
print(intNum >> nullValue);
print(intNum >>> nullValue);
print(intNum | nullValue);
print(intNum ^ nullValue);

// double op null
print("=====double op null=====")
print(doubleNum + nullValue);
print(doubleNum - nullValue);
print(doubleNum * nullValue);
print(doubleNum / nullValue);
print(doubleNum % nullValue);
print(doubleNum < nullValue);
print(doubleNum <= nullValue);
print(doubleNum > nullValue);
print(doubleNum >= nullValue);
print(doubleNum == nullValue);
print(doubleNum === nullValue);
print(doubleNum != nullValue);
print(doubleNum !== nullValue);
print(doubleNum << nullValue);
print(doubleNum >> nullValue);
print(doubleNum >>> nullValue);
print(doubleNum | nullValue);
print(doubleNum ^ nullValue);

// false op null
print("=====false op null=====")
print(falseValue + nullValue);
print(falseValue - nullValue);
print(falseValue * nullValue);
print(falseValue / nullValue);
print(falseValue % nullValue);
print(falseValue < nullValue);
print(falseValue <= nullValue);
print(falseValue > nullValue);
print(falseValue >= nullValue);
print(falseValue == nullValue);
print(falseValue === nullValue);
print(falseValue != nullValue);
print(falseValue !== nullValue);
print(falseValue << nullValue);
print(falseValue >> nullValue);
print(falseValue >>> nullValue);
print(falseValue | nullValue);
print(falseValue ^ nullValue);

// true op null
print("=====true op null=====")
print(trueValue + nullValue);
print(trueValue - nullValue);
print(trueValue * nullValue);
print(trueValue / nullValue);
print(trueValue % nullValue);
print(trueValue < nullValue);
print(trueValue <= nullValue);
print(trueValue > nullValue);
print(trueValue >= nullValue);
print(trueValue == nullValue);
print(trueValue === nullValue);
print(trueValue != nullValue);
print(trueValue !== nullValue);
print(trueValue << nullValue);
print(trueValue >> nullValue);
print(trueValue >>> nullValue);
print(trueValue | nullValue);
print(trueValue ^ nullValue);

// int op false
print("=====int op false=====")
print(intNum + falseValue);
print(intNum - falseValue);
print(intNum * falseValue);
print(intNum / falseValue);
print(intNum % falseValue);
print(intNum < falseValue);
print(intNum <= falseValue);
print(intNum > falseValue);
print(intNum >= falseValue);
print(intNum == falseValue);
print(intNum === falseValue);
print(intNum != falseValue);
print(intNum !== falseValue);
print(intNum << falseValue);
print(intNum >> falseValue);
print(intNum >>> falseValue);
print(intNum | falseValue);
print(intNum ^ falseValue);

// double op false
print("=====double op false=====")
print(doubleNum + falseValue);
print(doubleNum - falseValue);
print(doubleNum * falseValue);
print(doubleNum / falseValue);
print(doubleNum % falseValue);
print(doubleNum < falseValue);
print(doubleNum <= falseValue);
print(doubleNum > falseValue);
print(doubleNum >= falseValue);
print(doubleNum == falseValue);
print(doubleNum === falseValue);
print(doubleNum != falseValue);
print(doubleNum !== falseValue);
print(doubleNum << falseValue);
print(doubleNum >> falseValue);
print(doubleNum >>> falseValue);
print(doubleNum | falseValue);
print(doubleNum ^ falseValue);

// true op false
print("=====true op false=====")
print(trueValue + falseValue);
print(trueValue - falseValue);
print(trueValue * falseValue);
print(trueValue / falseValue);
print(trueValue % falseValue);
print(trueValue < falseValue);
print(trueValue <= falseValue);
print(trueValue > falseValue);
print(trueValue >= falseValue);
print(trueValue == falseValue);
print(trueValue === falseValue);
print(trueValue != falseValue);
print(trueValue !== falseValue);
print(trueValue << falseValue);
print(trueValue >> falseValue);
print(trueValue >>> falseValue);
print(trueValue | falseValue);
print(trueValue ^ falseValue);

// int op true
print("=====int op true=====")
print(intNum + trueValue);
print(intNum - trueValue);
print(intNum * trueValue);
print(intNum / trueValue);
print(intNum % trueValue);
print(intNum < trueValue);
print(intNum <= trueValue);
print(intNum > trueValue);
print(intNum >= trueValue);
print(intNum == trueValue);
print(intNum === trueValue);
print(intNum != trueValue);
print(intNum !== trueValue);
print(intNum << trueValue);
print(intNum >> trueValue);
print(intNum >>> trueValue);
print(intNum | trueValue);
print(intNum ^ trueValue);

// double op true
print("=====double op true=====")
print(doubleNum + trueValue);
print(doubleNum - trueValue);
print(doubleNum * trueValue);
print(doubleNum / trueValue);
print(doubleNum % trueValue);
print(doubleNum < trueValue);
print(doubleNum <= trueValue);
print(doubleNum > trueValue);
print(doubleNum >= trueValue);
print(doubleNum == trueValue);
print(doubleNum === trueValue);
print(doubleNum != trueValue);
print(doubleNum !== trueValue);
print(doubleNum << trueValue);
print(doubleNum >> trueValue);
print(doubleNum >>> trueValue);
print(doubleNum | trueValue);
print(doubleNum ^ trueValue);

// true op false
print("=====true op false=====")
print(trueValue + falseValue);
print(trueValue - falseValue);
print(trueValue * falseValue);
print(trueValue / falseValue);
print(trueValue % falseValue);
print(trueValue < falseValue);
print(trueValue <= falseValue);
print(trueValue > falseValue);
print(trueValue >= falseValue);
print(trueValue == falseValue);
print(trueValue === falseValue);
print(trueValue != falseValue);
print(trueValue !== falseValue);
print(trueValue << falseValue);
print(trueValue >> falseValue);
print(trueValue >>> falseValue);
print(trueValue | falseValue);
print(trueValue ^ falseValue);

// undefined op undefined
print("=====undefined op undefined=====")
print(undf + undf);
print(undf - undf);
print(undf * undf);
print(undf / undf);
print(undf % undf);
print(undf < undf);
print(undf <= undf);
print(undf > undf);
print(undf >= undf);
print(undf == undf);
print(undf === undf);
print(undf != undf);
print(undf !== undf);
print(undf << undf);
print(undf >> undf);
print(undf >>> undf);
print(undf | undf);
print(undf ^ undf);

// null op null
print("=====null op null=====")
print(nullValue + nullValue);
print(nullValue - nullValue);
print(nullValue * nullValue);
print(nullValue / nullValue);
print(nullValue % nullValue);
print(nullValue < nullValue);
print(nullValue <= nullValue);
print(nullValue > nullValue);
print(nullValue >= nullValue);
print(nullValue == nullValue);
print(nullValue === nullValue);
print(nullValue != nullValue);
print(nullValue !== nullValue);
print(nullValue << nullValue);
print(nullValue >> nullValue);
print(nullValue >>> nullValue);
print(nullValue | nullValue);
print(nullValue ^ nullValue);

// true op true
print("=====true op true=====")
print(trueValue + trueValue);
print(trueValue - trueValue);
print(trueValue * trueValue);
print(trueValue / trueValue);
print(trueValue % trueValue);
print(trueValue < trueValue);
print(trueValue <= trueValue);
print(trueValue > trueValue);
print(trueValue >= trueValue);
print(trueValue == trueValue);
print(trueValue === trueValue);
print(trueValue != trueValue);
print(trueValue !== trueValue);
print(trueValue << trueValue);
print(trueValue >> trueValue);
print(trueValue >>> trueValue);
print(trueValue | trueValue);
print(trueValue ^ trueValue);

// false op false
print("=====false op false=====")
print(falseValue + falseValue);
print(falseValue - falseValue);
print(falseValue * falseValue);
print(falseValue / falseValue);
print(falseValue % falseValue);
print(falseValue < falseValue);
print(falseValue <= falseValue);
print(falseValue > falseValue);
print(falseValue >= falseValue);
print(falseValue == falseValue);
print(falseValue === falseValue);
print(falseValue != falseValue);
print(falseValue !== falseValue);
print(falseValue << falseValue);
print(falseValue >> falseValue);
print(falseValue >>> falseValue);
print(falseValue | falseValue);
print(falseValue ^ falseValue);