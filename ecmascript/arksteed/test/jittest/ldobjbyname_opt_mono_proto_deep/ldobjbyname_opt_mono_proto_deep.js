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

function LoadScore(obj)
{
    return obj.score;
}

function Record()
{
    this.tag = 1;
}

let expected = { value: 41 };
let root = { score: expected };
let middle = Object.create(root);
Record.prototype = Object.create(middle);

let receiver = new Record();
let hits = 0;
for (let i = 0; i < 20; i++) {
    if (LoadScore(receiver) === expected) {
        hits++;
    }
}

ArkTools.jitCompileAsync(LoadScore);
print(ArkTools.waitJitCompileFinish(LoadScore));

print(hits);
print(LoadScore(receiver).value);

export {};
