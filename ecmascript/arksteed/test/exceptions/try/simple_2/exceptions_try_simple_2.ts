/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

function foo(n: number)
{
    if (n % 5 == 0) {
        throw new Error("foo() throws");
    }
    return n + 100;
}

function bar(n: number)
{
    if (n % 7 == 0) {
        throw new Error("bar() throws");
    }
    return n + 900;
}

function exception_try_catch_simple_2(n: number)
{
    try {
        return foo(n);
    } catch (e) {}
    try {
        return bar(n);
    } catch (e) {}
    return -1;
}

ArkTools.arkSteedCompileSync(exception_try_catch_simple_2);

print(exception_try_catch_simple_2(22));
print(exception_try_catch_simple_2(25));
print(exception_try_catch_simple_2(28));
print(exception_try_catch_simple_2(35));
