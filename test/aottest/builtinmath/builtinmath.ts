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
let len:number = 1;
len = 1 / Math.sqrt(len);
print(len);

len = 9 / len;
len = Math.sqrt(len);
print(len);
print(NaN);
len = Math.sqrt(NaN);
print(len);

len = Math.cos(0); // 1
print(len);
len = Math.cos(1); // 0.5....
print(len);

len = Math.sin(0); // 0
print(len);
len = Math.sin(1); // 0.84
print(len);
len = Math.sin(Math.PI / 2);
print(len);

len = Math.acos(0.5);// 1.0471975511965979
print(len);

len = Math.atan(2); // 1.1071487177940904
print(len);

len = Math.abs(Number.NaN);
print(len);
len = Math.abs(-Number.NaN);
print(len);
len = Math.abs(Number.NEGATIVE_INFINITY);
print(len);
len = Math.abs(Number.POSITIVE_INFINITY);
print(len);
len = Math.abs(9.6);
print(len);
len = Math.abs(6);
print(len);
len = Math.abs(-9.6);
print(len);
len = Math.abs(-6);
print(len);

len = Math.floor(Number.NaN);
print(len);
len = Math.floor(-Number.NaN);
print(len);
len = Math.floor(Number.NEGATIVE_INFINITY);
print(len);
len = Math.floor(Number.POSITIVE_INFINITY);
print(len);
len = Math.floor(9.6);
print(len);
len = Math.floor(-9.6);
print(len);