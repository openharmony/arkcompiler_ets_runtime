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
function if_nested_return_inner_only(x, y, a, b, c)
{
    if (x > 0) {
        if (y > 0) {
            return a;
        }
        return b;
    }
    return c;
}

ArkTools.arkSteedCompileSync(if_nested_return_inner_only);


print(if_nested_return_inner_only(10, 5, 100, 200, 300));
print(if_nested_return_inner_only(10, -1, 100, 200, 300));
print(if_nested_return_inner_only(-1, 5, 100, 200, 300));