/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
let num1 = 0;
let num2 = 0.01;
let num3 = -0.01;
let num4 = 10;
let num5 = -10;
let num6 = 100;
let num7 = -100;
let num8 = undefined;

let str = "sd-93u498thikefnow8y3-0rh1nalksfnwo8y3t19-3r8hoiefnw";

print(str.substring(0));

print(str.substring(-0.01));

print(str.substring(0.01));

print(str.substring(100));

print(str.substring(-100));

print(str.substring(10));

print(str.substring(-10));

print(str.substring(undefined));

print(str.substring(num4));

print(str.substring(1, 2));

print(str.substring(5, 7));

print(str.substring(0, 0.01));

print(str.substring(-0.01, 100));

print(str.substring(-100, undefined));

print(str.substring(num1, num4));

print(str.substr(0));

print(str.substr(-0.01));

print(str.substr(0.01));

print(str.substr(100));

print(str.substr(-100));

print(str.substr(10));

print(str.substr(-10));

print(str.substr(undefined));

print(str.substr(num4));

print(str.substr(1, 2));

print(str.substr(5, 7));

print(str.substr(0, 0.01));

print(str.substr(-0.01, 100));

print(str.substr(-100, undefined));

print(str.substr(num1, num4));

print(str.slice(0));

print(str.slice(-0.01));

print(str.slice(0.01));

print(str.slice(100));

print(str.slice(-100));

print(str.slice(10));

print(str.slice(-10));

print(str.slice(undefined));

print(str.slice(num4));

print(str.slice(1, 2));

print(str.slice(5, 7));

print(str.slice(0, 0.01));

print(str.slice(-0.01, 100));

print(str.slice(-100, undefined));

print(str.slice(num1, num4));