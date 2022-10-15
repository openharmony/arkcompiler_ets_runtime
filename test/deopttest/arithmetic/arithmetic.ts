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

function foo0(a:number, b:number)
{
    return a + b;
}

print(foo0(1, 2.0));
print(foo0(2.0, 1));
print(foo0(1, 1));
print(foo0(2.0, 2.0));

function foo1(n:number)
{
    return n + 6;
}

print(foo1(6.0));
print(foo1(9));

function foo2(n:number)
{
    return 9.0 + n;
}

print(foo2(6.0));
print(foo2(9));

print(1 + 2);
print(1 + 1.0);
print(1.0 + 2.0);