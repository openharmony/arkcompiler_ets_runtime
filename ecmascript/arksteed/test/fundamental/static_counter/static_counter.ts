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
class Counter {
    static count: number = 0;

    static inc(): void {
        Counter.count++;
    }

    static dec(): void {
        Counter.count--;
    }
}

function static_counter(n: number): number {
    for (let i = 0; i < n; i++) {
        Counter.inc();
    }
    for (let i = 0; i < n / 2; i++) {
        Counter.dec();
    }
    return Counter.count;
}

ArkTools.arkSteedCompileAsync(Counter);
ArkTools.arkSteedCompileAsync(Counter.inc);
ArkTools.arkSteedCompileAsync(Counter.dec);
ArkTools.arkSteedCompileAsync(static_counter);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

print(static_counter(100));
