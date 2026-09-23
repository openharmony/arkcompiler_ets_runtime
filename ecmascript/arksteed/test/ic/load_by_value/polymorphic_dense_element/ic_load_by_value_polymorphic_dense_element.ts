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
//! METHOD loadPolymorphicDense
//! HAS LoadTaggedElement
//! HAS LoadInt32Field
//! HAS DeoptIfInt32Condition
//! HAS DeoptIfTaggedCondition
//! HAS DeoptIfHClassNotIn
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

function loadPolymorphicDense(array, index) {
    return array[index];
}

const polymorphicDenseA = ["alpha", "beta", "gamma"];
const polymorphicDenseB = [true, false, true];
polymorphicDenseB.extra = true;
for (let i = 0; i < 1000; i++) {
    loadPolymorphicDense(i & 1 ? polymorphicDenseA : polymorphicDenseB, 1);
}

ArkTools.arkSteedCompileSync(loadPolymorphicDense);
print(loadPolymorphicDense(polymorphicDenseA, 1));
print(loadPolymorphicDense(polymorphicDenseB, 1));
print(loadPolymorphicDense({1: "fallback"}, 1));
