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

//! PARAMS --compiler-jit-hotness-threshold=6000       # These params are passed to ark_js_vm

//! METHOD      innerProduct
//! COUNT_GE    InitialValue        4
//! HAS         I32AddWithOverflow
//! HAS_NOT     CallCommonStub      GetPropertyByName
//! COUNT       I32MulWithOverflow  2

const REP = 6000;

function innerProduct(x1: number, y1: number, x2: number, y2: number) {
    return x1 * x2 + y1 * y2;
}

for (let i = 0; i < REP; i++) {
    innerProduct(i, 1 - i, 2 - i, 3 + i);
}

ArkTools.arkSteedCompileSync(innerProduct);

print(innerProduct(2, 3, 4, 5));
print(innerProduct(30000, 40000, 40000, 50000))
print(innerProduct(0.5, 1.5, 1.5, 2.5));
