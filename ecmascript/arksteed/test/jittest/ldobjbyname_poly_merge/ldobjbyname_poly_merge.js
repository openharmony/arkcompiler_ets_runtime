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

function LoadX(obj)
{
    return obj.x;
}

let obj = {
    x: 42,
    y: 1,
};

let obj2 = {
    x: 43,
    z: 1,
};

let warmup = 0;
for (let i = 0; i < 20; i++) {
    warmup += LoadX(obj);
    warmup += LoadX(obj2);
}

ArkTools.jitCompileAsync(LoadX);
print(ArkTools.waitJitCompileFinish(LoadX));
print(warmup);
print(LoadX(obj));
print(LoadX(obj2));
print(LoadX({ q: 1, x: 44 }));
