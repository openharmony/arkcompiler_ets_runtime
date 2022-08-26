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
var num1:number = 99
var num2:number = 2
var res:number = num1 % num2
print(res)

var num3: number = 50
var num4: number = 45.5
print(num3 % num4)

function foo(arg0:number, arg1:number) {
    print(arg0 % arg1);
}
foo(2.1, 2.1)

print(3 % 4);

function foo1(arg0:number, arg1:number) {
    return arg0 % arg1;
}
print(foo1(3, 4))

var num5: number = 3
var num6 = 0;
if (num5 > 0) {
    num6 = num5 % 2;
}
print(num6);

