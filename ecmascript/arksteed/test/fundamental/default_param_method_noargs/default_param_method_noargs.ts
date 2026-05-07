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
class DefaultParamMethodNoargs {
    prefix: string;

    constructor(prefix: string) {
        this.prefix = prefix;
    }

    buildLabel(
        make: string = "Unknown",
        model: string = "Model",
        year: number = 2020
    ): string {
        return `${this.prefix}:${year}:${make}:${model}`;
    }
}

ArkTools.arkSteedCompileAsync(DefaultParamMethodNoargs.prototype.buildLabel);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let builder = new DefaultParamMethodNoargs("CAR");
print(builder.buildLabel());
