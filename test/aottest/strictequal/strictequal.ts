/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

let num1:any = 1;
let num2:any = 1;

let str1:any = "1";
let str2:any = "1";

let boolTrue1:any = true;
let boolTrue2:any = true;

let undefine1:any = undefined;
let undefine2:any = undefined;

let bigInt1:any = 111111111111;
let bigInt2:any = 111111111112;
let bigInt3:any = 111111111111;

let double1:any = 1.0;
let double2:any = 1.0;
let double3:any = 1.2;


print(num1 === num2);

print(str1 === str2);

print(boolTrue1 === boolTrue2);

print(undefine1 === undefine2);

print(num1 === str1);

print(num1 === boolTrue1);

print(str1 === boolTrue1);

print(bigInt1 === bigInt2);

print(bigInt1 === bigInt3);

print(double1 === double2);

print(double1 === double3);

print(double1 === num1);

print(double3 === num1);

print(NaN === NaN);

print(undefined == NaN);

print(undefined == undefined);

print(undefined == null);

print(null == null);
