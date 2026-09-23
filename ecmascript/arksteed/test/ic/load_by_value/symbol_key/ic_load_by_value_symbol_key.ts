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

// @ts-nocheck
//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD loadSymbol
//! HAS DeoptIfTaggedCondition
//! HAS DeoptIfHClassMismatch
//! HAS LoadTaggedField
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

const cachedKey = Symbol("cached-key");
const otherKey = Symbol("other-key");
const receiver = {
    [cachedKey]: 37,
    [otherKey]: 42,
};

function loadSymbol(object, key) {
    return object[key];
}

for (let i = 0; i < 1000; i++) {
    loadSymbol(receiver, cachedKey);
}

ArkTools.arkSteedCompileSync(loadSymbol);
print(loadSymbol(receiver, cachedKey));
print(loadSymbol(receiver, otherKey));
