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

export function add(a, b)
{
    return a + b; 
}

var a = "abcdwodehhhhjjjjkkkkkooodfjsar";
var a1 = a.substring(4, 28);
var b = "efgh"
var c = "我的bbb";
var d = "你的";
var a2 = a + b;
var a3 = a + c;
var a4 = c + d;
var a5 = a1 + d;
var a6 = a2 + a5;
var a7 = d.toLowerCase() + a1.substring(1);
var a8 = a2.toUpperCase() + a3.substring(2);
var a9 = a7 + a8;
print(a1);
print(a2);
print(a3);
print(a4);
print(a5);
print(a6);
print(a7);
print(a8);
print(a9);