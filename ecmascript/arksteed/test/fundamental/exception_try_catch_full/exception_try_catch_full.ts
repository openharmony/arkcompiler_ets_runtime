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
function exception_try_catch_full()
{
    let result = 0;

    // try-catch
    try {
        result = 10;
        throw new Error("test error");
    } catch (e) {
        result = 20;
    }

    // try-catch-finally
    try {
        result += 5;
        // no error thrown
    } catch (e) {
        result += 10;
    } finally {
        result += 2;
    }

    // nested try-catch
    try {
        try {
            result += 3;
            throw new Error("inner error");
        } catch (e) {
            result += 7;
        }
    } catch (e) {
        result += 1;
    }

    return result;
}

ArkTools.arkSteedCompileSync(exception_try_catch_full);

// Spin loop: A temporary approach to wait for ArkSteed JIT compiler

// TODO: Replace the spin loop above to:
// let res = ArkTools.waitJitCompileFinish(exception_try_catch_full);
// print(res);

let output = exception_try_catch_full();
print(output);