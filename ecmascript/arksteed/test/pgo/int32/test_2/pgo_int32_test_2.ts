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

//! PARAMS --compiler-jit-hotness-threshold=6000        # These params are passed to ark_js_vm

//! METHOD      get
//! HAS         Deopt                                   # throw Error("Invalid key")
//! HAS_NOT     Throw
//! COUNT_GE    DeoptIfHClassMismatch   4
//! COUNT_GE    LoadTaggedField         4

//! METHOD      innerProduct
//! COUNT_GE    InitialValue        4
//! HAS         I32AddWithOverflow
//! HAS_NOT     CallCommonStub      GetPropertyByName
//! COUNT       I32MulWithOverflow  2

const REP = 6000;

function get(obj: any, key: 'x1' | 'y1' | 'x2' | 'y2'): number {
    if (key == 'x1') {
        return obj.x1 as number;
    }
    if (key == 'x2') {
        return obj.x2 as number;
    }
    if (key == 'y1') {
        return obj.y1 as number;
    }
    if (key == 'y2') {
        return obj.y2 as number;
    }
    throw Error("Invalid key");
}

function innerProduct(obj: Object): number {
    return get(obj, 'x1') * get(obj, 'x2') + get(obj, 'y1') * get(obj, 'y2');
}

for (let i = 0; i < REP; i++) {
    innerProduct({x1: i, y1: 1 - i, x2: 2 - i, y2: 3 + i});
}

ArkTools.arkSteedCompileSync(get);
ArkTools.arkSteedCompileSync(innerProduct);

print(innerProduct({x1: 2, y1: 3, x2: 4, y2: 5}));
print(innerProduct({x1: 30000, y1: 40000, x2: 40000, y2: 50000}))
print(innerProduct({x1: 0.5, y1: 1.5, x2: 1.5, y2: 2.5}));
