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

let data = '一一一一一一一一一一一一一一一一一一一一一一一一一一一一一一一一二二二二二二二二二二二二二二二二二二二二二二二二二二二二二二二二'

function Test(str) {
    var result = '';
    for (var i = 0; i < str.length; ++i) {
        result += str.charAt(i);
    }
    return result;
}

Test(data);
ArkTools.jitCompileAsync(Test);
var compileRet = ArkTools.waitJitCompileFinish(Test);
var testRet = Test(data);
print(compileRet);
print(testRet);
