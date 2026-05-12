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
class Accumulator {
    static total: number = 0;

    static add(n: number): void {
        Accumulator.total += n;
    }

    static get(): number {
        return Accumulator.total;
    }
}

function static_field_method(a: number, b: number, c: number): number {
    Accumulator.add(a);
    Accumulator.add(b);
    Accumulator.add(c);
    return Accumulator.get();
}

ArkTools.arkSteedCompileAsync(Accumulator);
ArkTools.arkSteedCompileAsync(Accumulator.add);
ArkTools.arkSteedCompileAsync(Accumulator.get);
ArkTools.arkSteedCompileAsync(static_field_method);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(static_field_method(10, 20, 30));
