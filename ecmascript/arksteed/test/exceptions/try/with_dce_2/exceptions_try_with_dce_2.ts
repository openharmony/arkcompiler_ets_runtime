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

function exception_try_catch_with_dce_2(n: number)
{
    let result = n;
    try {
        if (true) {
            result += 42;
        } else {
            throw -1;
        }
    } catch (e) {
        result = (e as number);
    }
    return result;
}

ArkTools.arkSteedCompileSync(exception_try_catch_with_dce_2);

print(exception_try_catch_with_dce_2(100));
