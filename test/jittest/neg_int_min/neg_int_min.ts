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

// @ts-nocheck
declare function print(arg: any): string;

function NegValue(value1, value2) {
    for (var i = value1; i >= -37908; --i) {
        var value = ((i * 991002474) & value2);
        print("value: " + value + ", -value: " + (-value));
    }
}

NegValue(-37905, -15590699);
ArkTools.jitCompileAsync(NegValue);
print(ArkTools.waitJitCompileFinish(NegValue));
NegValue(-37906, -155906996);
