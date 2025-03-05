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

function Test(a9, a10, a11) {
    this.h = a11;
    Math.sqrt(a9);
    function F27(a29) {
        this.g = a29;
    }
    F27.prototype = this;
    new F27(-13);
    this.h = a10;
}
const v43 = new Test("sticky", "undefined", -15);
print(ArkTools.isAOTCompiled(Test));
