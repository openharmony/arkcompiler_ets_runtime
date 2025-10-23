/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

class N extends Number {}
function Test() {
    let n1 = new N(10.2);
    if (!(n1 instanceof N)) {
        throw "n1 has incorrect prototype chain";
    }
    let n2 = new N("3.14");
    if (!(n2 instanceof N)) {
        throw "n2 has incorrect prototype chain";
    }
}

for (let i = 0; i < 200; i++) {
    Test();
}

print(ArkTools.waitAllJitCompileFinish());

Test();
