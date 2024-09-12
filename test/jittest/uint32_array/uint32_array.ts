/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function Test(element, cond) {
    const array = new Uint32Array(element);
    const elementOne = array[array.length / 2];
    const elementTwo = array[array.length % 2];
    var result = undefined;
    if (cond) {
        result = elementOne;
    } else {
        result = elementTwo;
    }
    return result;
}

Test(200, true)

ArkTools.jitCompileAsync(Test);
var compilerResult = ArkTools.waitJitCompileFinish(Test);
print("compilerResult: " + compilerResult);
