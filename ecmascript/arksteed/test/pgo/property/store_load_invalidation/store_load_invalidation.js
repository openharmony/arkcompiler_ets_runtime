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

//! PARAMS --compiler-jit-hotness-threshold=10000
//! METHOD storeThenLoadProperty
//! HAS_NOT CallCommonStub GetPropertyByName
//! HAS_NOT CallCommonStub SetPropertyByName

function Holder(value)
{
    this.value = value;
    this.marker = 1;
}

function storeThenLoadProperty(holder, value)
{
    let first = holder.value;
    holder.value = value;
    return first + holder.value;
}

const holder = new Holder(2);
for (let i = 0; i < 1000; i++) {
    storeThenLoadProperty(holder, i);
}

ArkTools.arkSteedCompileSync(storeThenLoadProperty);
ArkTools.waitJitCompileFinish(storeThenLoadProperty);

holder.value = 2;
let ok = ArkTools.isAOTCompiled(storeThenLoadProperty);
ok = ok && storeThenLoadProperty(holder, 20) === 22;
ok = ok && holder.value === 20;
ok = ok && holder.marker === 1;

print(ok ? "PASS" : "FAIL");
