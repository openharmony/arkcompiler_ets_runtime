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
class DefaultParamUndefinedConstructor {
    firstIsMissing: number;
    secondIsMissing: number;

    constructor(first?: string, second?: number) {
        this.firstIsMissing = first === undefined ? 1 : 0;
        this.secondIsMissing = second === undefined ? 1 : 0;
    }
}

ArkTools.arkSteedCompileSync(DefaultParamUndefinedConstructor);


let a = new DefaultParamUndefinedConstructor();
print(a.firstIsMissing);
print(a.secondIsMissing);

let b = new DefaultParamUndefinedConstructor("ok");
print(b.firstIsMissing);
print(b.secondIsMissing);

let c = new DefaultParamUndefinedConstructor("ok", 8);
print(c.firstIsMissing);
print(c.secondIsMissing);
