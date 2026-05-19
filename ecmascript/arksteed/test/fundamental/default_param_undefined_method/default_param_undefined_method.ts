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
class DefaultParamUndefinedMethod {
    scale: number;

    constructor(scale: number) {
        this.scale = scale;
    }

    compute(value?: number, extra?: number): number {
        let left = value === undefined ? 100 : value * this.scale;
        let right = extra === undefined ? 200 : extra + this.scale;
        return left + right;
    }
}

ArkTools.arkSteedCompileSync(DefaultParamUndefinedMethod);


let worker = new DefaultParamUndefinedMethod(3);
print(worker.compute());
print(worker.compute(2));
print(worker.compute(2, 5));
