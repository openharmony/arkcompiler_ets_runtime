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
declare function assert_true(condition: boolean):void;
declare function assert_equal(a: Object, b: Object):void;

let undf: any = undefined;
let intNum: number = 5;
let doubleNum: number = 1.1;
let falseValue: boolean = false;
let trueValue: boolean = true;
let nullValue: any = null;

// int op undefined
print("=====int op undefined=====")
assert_true(isNaN(intNum + undf));
assert_true(isNaN(intNum - undf));
assert_true(isNaN(intNum * undf));
assert_true(isNaN(intNum / undf));
assert_true(isNaN(intNum % undf));
assert_equal(intNum < undf, falseValue);
assert_equal(intNum <= undf, falseValue);
assert_equal(intNum > undf, falseValue);
assert_equal(intNum >= undf, falseValue);
assert_equal(intNum == undf, falseValue);
assert_equal(intNum === undf, falseValue);
assert_equal(intNum != undf, trueValue);
assert_equal(intNum !== undf, trueValue);
assert_equal(intNum << undf, 5);
assert_equal(intNum >> undf, 5);
assert_equal(intNum >>> undf, 5);
assert_equal(intNum | undf, 5);
assert_equal(intNum ^ undf, 5);

// double op undefined
print("=====double op undefined=====")
assert_true(isNaN(doubleNum + undf));
assert_true(isNaN(doubleNum - undf));
assert_true(isNaN(doubleNum * undf));
assert_true(isNaN(doubleNum / undf));
assert_true(isNaN(doubleNum % undf));
assert_equal(doubleNum < undf, falseValue);
assert_equal(doubleNum <= undf, falseValue);
assert_equal(doubleNum > undf, falseValue);
assert_equal(doubleNum >= undf, falseValue);
assert_equal(doubleNum == undf, falseValue);
assert_equal(doubleNum === undf, falseValue);
assert_equal(doubleNum != undf, trueValue);
assert_equal(doubleNum !== undf, trueValue);
assert_equal(doubleNum << undf, 1);
assert_equal(doubleNum >> undf, 1);
assert_equal(doubleNum >>> undf, 1);
assert_equal(doubleNum | undf, 1);
assert_equal(doubleNum ^ undf, 1);

// false op undefined
print("=====false op undefined=====")
assert_true(isNaN(falseValue + undf));
assert_true(isNaN(falseValue - undf));
assert_true(isNaN(falseValue * undf));
assert_true(isNaN(falseValue / undf));
assert_true(isNaN(falseValue % undf));
assert_equal(falseValue < undf, falseValue);
assert_equal(falseValue <= undf, falseValue);
assert_equal(falseValue > undf, falseValue);
assert_equal(falseValue >= undf, falseValue);
assert_equal(falseValue == undf, falseValue);
assert_equal(falseValue === undf, falseValue);
assert_equal(falseValue != undf, trueValue);
assert_equal(falseValue !== undf, trueValue);
assert_equal(falseValue << undf, 0);
assert_equal(falseValue >> undf, 0);
assert_equal(falseValue >>> undf, 0);
assert_equal(falseValue | undf, 0);
assert_equal(falseValue ^ undf, 0);

// true op undefined
print("=====true op undefined=====")
assert_true(isNaN(trueValue + undf));
assert_true(isNaN(trueValue - undf));
assert_true(isNaN(trueValue * undf));
assert_true(isNaN(trueValue / undf));
assert_true(isNaN(trueValue % undf));
assert_equal(trueValue < undf, falseValue);
assert_equal(trueValue <= undf, falseValue);
assert_equal(trueValue > undf, falseValue);
assert_equal(trueValue >= undf, falseValue);
assert_equal(trueValue == undf, falseValue);
assert_equal(trueValue === undf, falseValue);
assert_equal(trueValue != undf, trueValue);
assert_equal(trueValue !== undf, trueValue);
assert_equal(trueValue << undf, 1);
assert_equal(trueValue >> undf, 1);
assert_equal(trueValue >>> undf, 1);
assert_equal(trueValue | undf, 1);
assert_equal(trueValue ^ undf, 1);

// null op undefined
print("=====null op undefined=====")
assert_true(isNaN(nullValue + undf));
assert_true(isNaN(nullValue - undf));
assert_true(isNaN(nullValue * undf));
assert_true(isNaN(nullValue / undf));
assert_true(isNaN(nullValue % undf));
assert_equal(nullValue < undf, falseValue);
assert_equal(nullValue <= undf, falseValue);
assert_equal(nullValue > undf, falseValue);
assert_equal(nullValue >= undf, falseValue);
assert_equal(nullValue == undf, trueValue);
assert_equal(nullValue === undf, falseValue);
assert_equal(nullValue != undf, falseValue);
assert_equal(nullValue !== undf, trueValue);
assert_equal(nullValue << undf, 0);
assert_equal(nullValue >> undf, 0);
assert_equal(nullValue >>> undf, 0);
assert_equal(nullValue | undf, 0);
assert_equal(nullValue ^ undf, 0);

// int op null
print("=====int op null=====")
assert_equal(intNum + nullValue, 5);
assert_equal(intNum - nullValue, 5);
assert_equal(intNum * nullValue, 0);
assert_equal(intNum / nullValue, Infinity);
assert_true(isNaN(intNum % nullValue));
assert_equal(intNum < nullValue, falseValue);
assert_equal(intNum <= nullValue, falseValue);
assert_equal(intNum > nullValue, trueValue);
assert_equal(intNum >= nullValue, trueValue);
assert_equal(intNum == nullValue, falseValue);
assert_equal(intNum === nullValue, falseValue);
assert_equal(intNum != nullValue, trueValue);
assert_equal(intNum !== nullValue, trueValue);
assert_equal(intNum << nullValue, 5);
assert_equal(intNum >> nullValue, 5);
assert_equal(intNum >>> nullValue, 5);
assert_equal(intNum | nullValue, 5);
assert_equal(intNum ^ nullValue, 5);

// double op null
print("=====double op null=====")
assert_equal(doubleNum + nullValue, 1.1);
assert_equal(doubleNum - nullValue, 1.1);
assert_equal(doubleNum * nullValue, 0);
assert_equal(doubleNum / nullValue, Infinity);
assert_true(isNaN(doubleNum % nullValue));
assert_equal(doubleNum < nullValue, falseValue);
assert_equal(doubleNum <= nullValue, falseValue);
assert_equal(doubleNum > nullValue, trueValue);
assert_equal(doubleNum >= nullValue, trueValue);
assert_equal(doubleNum == nullValue, falseValue);
assert_equal(doubleNum === nullValue, falseValue);
assert_equal(doubleNum != nullValue, trueValue);
assert_equal(doubleNum !== nullValue, trueValue);
assert_equal(doubleNum << nullValue, 1);
assert_equal(doubleNum >> nullValue, 1);
assert_equal(doubleNum >>> nullValue, 1);
assert_equal(doubleNum | nullValue, 1);
assert_equal(doubleNum ^ nullValue, 1);

// false op null
print("=====false op null=====")
assert_equal(falseValue + nullValue, 0);
assert_equal(falseValue - nullValue, 0);
assert_equal(falseValue * nullValue, 0);
assert_true(isNaN(falseValue / nullValue));
assert_true(isNaN(falseValue % nullValue));
assert_equal(falseValue < nullValue, falseValue);
assert_equal(falseValue <= nullValue, trueValue);
assert_equal(falseValue > nullValue, falseValue);
assert_equal(falseValue >= nullValue, trueValue);
assert_equal(falseValue == nullValue, falseValue);
assert_equal(falseValue === nullValue, falseValue);
assert_equal(falseValue != nullValue, trueValue);
assert_equal(falseValue !== nullValue, trueValue);
assert_equal(falseValue << nullValue, 0);
assert_equal(falseValue >> nullValue, 0);
assert_equal(falseValue >>> nullValue, 0);
assert_equal(falseValue | nullValue, 0);
assert_equal(falseValue ^ nullValue, 0);

// true op null
print("=====true op null=====")
assert_equal(trueValue + nullValue, 1);
assert_equal(trueValue - nullValue, 1);
assert_equal(trueValue * nullValue, 0);
assert_equal(trueValue / nullValue, Infinity);
assert_true(isNaN(trueValue % nullValue));
assert_equal(trueValue < nullValue, falseValue);
assert_equal(trueValue <= nullValue, falseValue);
assert_equal(trueValue > nullValue, trueValue);
assert_equal(trueValue >= nullValue, trueValue);
assert_equal(trueValue == nullValue, falseValue);
assert_equal(trueValue === nullValue, falseValue);
assert_equal(trueValue != nullValue, trueValue);
assert_equal(trueValue !== nullValue, trueValue);
assert_equal(trueValue << nullValue, 1);
assert_equal(trueValue >> nullValue, 1);
assert_equal(trueValue >>> nullValue, 1);
assert_equal(trueValue | nullValue, 1);
assert_equal(trueValue ^ nullValue, 1);

// int op false
print("=====int op false=====")
assert_equal(intNum + falseValue, 5);
assert_equal(intNum - falseValue, 5);
assert_equal(intNum * falseValue, 0);
assert_equal(intNum / falseValue, Infinity);
assert_true(isNaN(intNum % falseValue));
assert_equal(intNum < falseValue, falseValue);
assert_equal(intNum <= falseValue, falseValue);
assert_equal(intNum > falseValue, trueValue);
assert_equal(intNum >= falseValue, trueValue);
assert_equal(intNum == falseValue, falseValue);
assert_equal(intNum === falseValue, falseValue);
assert_equal(intNum != falseValue, trueValue);
assert_equal(intNum !== falseValue, trueValue);
assert_equal(intNum << falseValue, 5);
assert_equal(intNum >> falseValue, 5);
assert_equal(intNum >>> falseValue, 5);
assert_equal(intNum | falseValue, 5);
assert_equal(intNum ^ falseValue, 5);

// double op false
print("=====double op false=====")
assert_equal(doubleNum + falseValue, 1.1);
assert_equal(doubleNum - falseValue, 1.1);
assert_equal(doubleNum * falseValue, 0);
assert_equal(doubleNum / falseValue, Infinity);
assert_true(isNaN(doubleNum % falseValue));
assert_equal(doubleNum < falseValue, falseValue);
assert_equal(doubleNum <= falseValue, falseValue);
assert_equal(doubleNum > falseValue, trueValue);
assert_equal(doubleNum >= falseValue, trueValue);
assert_equal(doubleNum == falseValue, falseValue);
assert_equal(doubleNum === falseValue, falseValue);
assert_equal(doubleNum != falseValue, trueValue);
assert_equal(doubleNum !== falseValue, trueValue);
assert_equal(doubleNum << falseValue, 1);
assert_equal(doubleNum >> falseValue, 1);
assert_equal(doubleNum >>> falseValue, 1);
assert_equal(doubleNum | falseValue, 1);
assert_equal(doubleNum ^ falseValue, 1);

// true op false
print("=====true op false=====")
assert_equal(trueValue + falseValue, 1);
assert_equal(trueValue - falseValue, 1);
assert_equal(trueValue * falseValue, 0);
assert_equal(trueValue / falseValue, Infinity);
assert_true(isNaN(trueValue % falseValue));
assert_equal(trueValue < falseValue, falseValue);
assert_equal(trueValue <= falseValue, falseValue);
assert_equal(trueValue > falseValue, trueValue);
assert_equal(trueValue >= falseValue, trueValue);
assert_equal(trueValue == falseValue, falseValue);
assert_equal(trueValue === falseValue, falseValue);
assert_equal(trueValue != falseValue, trueValue);
assert_equal(trueValue !== falseValue, trueValue);
assert_equal(trueValue << falseValue, 1);
assert_equal(trueValue >> falseValue, 1);
assert_equal(trueValue >>> falseValue, 1);
assert_equal(trueValue | falseValue, 1);
assert_equal(trueValue ^ falseValue, 1);

// int op true
print("=====int op true=====")
assert_equal(intNum + trueValue, 6);
assert_equal(intNum - trueValue, 4);
assert_equal(intNum * trueValue, 5);
assert_equal(intNum / trueValue, 5);
assert_equal(intNum % trueValue, 0);
assert_equal(intNum < trueValue, falseValue);
assert_equal(intNum <= trueValue, falseValue);
assert_equal(intNum > trueValue, trueValue);
assert_equal(intNum >= trueValue, trueValue);
assert_equal(intNum == trueValue, falseValue);
assert_equal(intNum === trueValue, falseValue);
assert_equal(intNum != trueValue, trueValue);
assert_equal(intNum !== trueValue, trueValue);
assert_equal(intNum << trueValue, 10);
assert_equal(intNum >> trueValue, 2);
assert_equal(intNum >>> trueValue, 2);
assert_equal(intNum | trueValue, 5);
assert_equal(intNum ^ trueValue, 4);

// double op true
print("=====double op true=====")
assert_equal(doubleNum + trueValue, 2.1);
assert_equal(doubleNum - trueValue, 0.10000000000000009);
assert_equal(doubleNum * trueValue, 1.1);
assert_equal(doubleNum / trueValue, 1.1);
assert_equal(doubleNum % trueValue, 0.10000000000000009);
assert_equal(doubleNum < trueValue, falseValue);
assert_equal(doubleNum <= trueValue, falseValue);
assert_equal(doubleNum > trueValue, trueValue);
assert_equal(doubleNum >= trueValue, trueValue);
assert_equal(doubleNum == trueValue, falseValue);
assert_equal(doubleNum === trueValue, falseValue);
assert_equal(doubleNum != trueValue, trueValue);
assert_equal(doubleNum !== trueValue, trueValue);
assert_equal(doubleNum << trueValue, 2);
assert_equal(doubleNum >> trueValue, 0);
assert_equal(doubleNum >>> trueValue, 0);
assert_equal(doubleNum | trueValue, 1);
assert_equal(doubleNum ^ trueValue, 0);

// true op false
print("=====true op false=====")
assert_equal(trueValue + falseValue, 1);
assert_equal(trueValue - falseValue, 1);
assert_equal(trueValue * falseValue, 0);
assert_equal(trueValue / falseValue, Infinity);
assert_true(isNaN(trueValue % falseValue));
assert_equal(trueValue < falseValue, falseValue);
assert_equal(trueValue <= falseValue, falseValue);
assert_equal(trueValue > falseValue, trueValue);
assert_equal(trueValue >= falseValue, trueValue);
assert_equal(trueValue == falseValue, falseValue);
assert_equal(trueValue === falseValue, falseValue);
assert_equal(trueValue != falseValue, trueValue);
assert_equal(trueValue !== falseValue, trueValue);
assert_equal(trueValue << falseValue, 1);
assert_equal(trueValue >> falseValue, 1);
assert_equal(trueValue >>> falseValue, 1);
assert_equal(trueValue | falseValue, 1);
assert_equal(trueValue ^ falseValue, 1);

// undefined op undefined
print("=====undefined op undefined=====")
assert_true(isNaN(undf + undf));
assert_true(isNaN(undf - undf));
assert_true(isNaN(undf * undf));
assert_true(isNaN(undf / undf));
assert_true(isNaN(undf % undf));
assert_equal(undf < undf, falseValue);
assert_equal(undf <= undf, falseValue);
assert_equal(undf > undf, falseValue);
assert_equal(undf >= undf, falseValue);
assert_equal(undf == undf, trueValue);
assert_equal(undf === undf, trueValue);
assert_equal(undf != undf, falseValue);
assert_equal(undf !== undf, falseValue);
assert_equal(undf << undf, 0);
assert_equal(undf >> undf, 0);
assert_equal(undf >>> undf, 0);
assert_equal(undf | undf, 0);
assert_equal(undf ^ undf, 0);

// null op null
print("=====null op null=====")
assert_equal(nullValue + nullValue, 0);
assert_equal(nullValue - nullValue, 0);
assert_equal(nullValue * nullValue, 0);
assert_true(isNaN(nullValue / nullValue));
assert_true(isNaN(nullValue % nullValue));
assert_equal(nullValue < nullValue, falseValue);
assert_equal(nullValue <= nullValue, trueValue);
assert_equal(nullValue > nullValue, falseValue);
assert_equal(nullValue >= nullValue, trueValue);
assert_equal(nullValue == nullValue, trueValue);
assert_equal(nullValue === nullValue, trueValue);
assert_equal(nullValue != nullValue, falseValue);
assert_equal(nullValue !== nullValue, falseValue);
assert_equal(nullValue << nullValue, 0);
assert_equal(nullValue >> nullValue, 0);
assert_equal(nullValue >>> nullValue, 0);
assert_equal(nullValue | nullValue, 0);
assert_equal(nullValue ^ nullValue, 0);

// true op true
print("=====true op true=====")
assert_equal(trueValue + trueValue, 2);
assert_equal(trueValue - trueValue, 0);
assert_equal(trueValue * trueValue, 1);
assert_equal(trueValue / trueValue, 1);
assert_equal(trueValue % trueValue, 0);
assert_equal(trueValue < trueValue, falseValue);
assert_equal(trueValue <= trueValue, trueValue);
assert_equal(trueValue > trueValue, falseValue);
assert_equal(trueValue >= trueValue, trueValue);
assert_equal(trueValue == trueValue, trueValue);
assert_equal(trueValue === trueValue, trueValue);
assert_equal(trueValue != trueValue, falseValue);
assert_equal(trueValue !== trueValue, falseValue);
assert_equal(trueValue << trueValue, 2);
assert_equal(trueValue >> trueValue, 0);
assert_equal(trueValue >>> trueValue, 0);
assert_equal(trueValue | trueValue, 1);
assert_equal(trueValue ^ trueValue, 0);

// false op false
print("=====false op false=====")
assert_equal(falseValue + falseValue, 0);
assert_equal(falseValue - falseValue, 0);
assert_equal(falseValue * falseValue, 0);
assert_true(isNaN(falseValue / falseValue));
assert_true(isNaN(falseValue % falseValue));
assert_equal(falseValue < falseValue, falseValue);
assert_equal(falseValue <= falseValue, trueValue);
assert_equal(falseValue > falseValue, falseValue);
assert_equal(falseValue >= falseValue, trueValue);
assert_equal(falseValue == falseValue, trueValue);
assert_equal(falseValue === falseValue, trueValue);
assert_equal(falseValue != falseValue, falseValue);
assert_equal(falseValue !== falseValue, falseValue);
assert_equal(falseValue << falseValue, 0);
assert_equal(falseValue >> falseValue, 0);
assert_equal(falseValue >>> falseValue, 0);
assert_equal(falseValue | falseValue, 0);
assert_equal(falseValue ^ falseValue, 0);