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
function replace(a : number)
{
    return a;
}
let len = 0;
function cos()
{
    Math.cos = replace;
    len = Math.cos(1);
    print(len);
}
cos()

len = Math.sin(0); // 0
print(len);
len = Math.sin(1); // 0.84
print(len);
len = Math.sin(Math.PI / 2);
print(len);
function sin()
{
    Math.sin = replace;
    len = Math.sin(1)
    print(len);
}
sin()

len = Math.acos(0.5);// 1.0471975511965979
print(len);

