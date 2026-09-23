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
//! METHOD load
//! HAS DeoptIfTaggedCondition
//! HAS DeoptIfHClassMismatch
//! HAS LoadTaggedField
//! HAS_NOT CallCommonStub GetPropertyByValue
declare function print(arg: any): string;

class Box {
    x = 81;
    y = 64;

    load(key) {
        return this[key];
    }
}

const box = new Box();
for (let i = 0; i < 1000; i++) {
    box.load("x");
}

ArkTools.arkSteedCompileSync(Box.prototype.load);
print(box.load("x"));
print(box.load("y"));
