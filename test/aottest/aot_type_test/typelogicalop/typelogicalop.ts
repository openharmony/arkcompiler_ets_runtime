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

declare function print(str:any):number;
var num1:number = 2
var num2:number = 99

var num5 = "7";
var num6 = "5";

print("operator < test list:")
print(num1 < num2)
if(num1 < num2) {
    print(num1 < num2);
}
print(num5 < num6)

print("operator <= test list:")
print(num1 <= num2)
num1 = num2
if(num1 <= num2) {
    print(num1 <= num2);
}
print(num5 <= num6)

print("operator > test list:")
print(num1 > num2)
num1 = num2 + 1;
if(num1 > num2) {
    print(num1 > num2);
}
print(num5 > num6)

print("operator >= test list:")
print(num1 >= num2)
num1 = num2;
if(num1 >= num2) {
    print(num1 >= num2);
}
print(num5 >= num6)
