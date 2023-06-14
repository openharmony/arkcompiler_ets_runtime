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

//declare function print(str:string):string;
declare function print(str:any):number;

var num1:number = 99
var num2:number = 2
var res:number = num1 % num2
print(res)
var num3: number = 50
var num4: number = 45.5
print(num3 % num4)
var num5:number = Number.NaN;
var num6:number = Number.NEGATIVE_INFINITY;
var num7:number = Number.POSITIVE_INFINITY;
var num8:number = 0.0;
print(num8 % num4)
print(num4 % num8)
print(num4 % num5)
print(num4 % num6)
print(num4 % num7)
print(num5 % num4)
print(num6 % num4)
print(num7 % num4)
print(num5 % num5)
print(num5 % num6)
print(num5 % num7)
print(num5 % num8)
print(num6 % num5)
print(num6 % num6)
print(num6 % num7)
print(num6 % num8)
print(num7 % num5)
print(num7 % num6)
print(num7 % num7)
print(num7 % num8)
print(num8 % num5)
print(num8 % num6)
print(num8 % num7)
print(num8 % num8)
