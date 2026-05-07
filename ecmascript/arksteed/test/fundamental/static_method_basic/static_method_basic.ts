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
class MathUtil {
    static add(a: number, b: number): number {
        return a + b;
    }

    static mul(a: number, b: number): number {
        return a * b;
    }
}

function static_method_basic(a: number, b: number): number {
    return MathUtil.add(a, b) + MathUtil.mul(a, b);
}

ArkTools.arkSteedCompileAsync(MathUtil);
ArkTools.arkSteedCompileAsync(MathUtil.add);
ArkTools.arkSteedCompileAsync(MathUtil.mul);
ArkTools.arkSteedCompileAsync(static_method_basic);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(static_method_basic(3, 4));
