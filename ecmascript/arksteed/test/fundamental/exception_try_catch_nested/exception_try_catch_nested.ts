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
function exception_try_catch_nested(inner_throw, outer_throw)
{
    let result = 0;

    try {
        result += 3;
        if (outer_throw) {
            throw new Error("outer error");
        }
        try {
            if (inner_throw) {
                throw new Error("inner error");
            }
        } catch (e) {
            result += 7;
        }
    } catch (e) {
        result += 1;
    }

    return result;
}

ArkTools.arkSteedCompileSync(exception_try_catch_nested);


let output = exception_try_catch_nested(true, false);
print(output);
output = exception_try_catch_nested(false, false);
print(output);
output = exception_try_catch_nested(false, true);
print(output);