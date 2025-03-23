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

class TestClass {
    constructor(one) {
        this._one = one;
    }
}

function Test() {
    let testClass = new TestClass('3.14');
    print(testClass instanceof TestClass);
    for (; testClass_.one < 5; (testClass_.one)++) {
        print(testClass_.one);
    }
}

Test();

ArkTools.jitCompileAsync(Test);
print(ArkTools.waitJitCompileFinish(Test));

Test();
