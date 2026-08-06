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

let firstRoot = {
    x: 11,
};
let firstMiddle = Object.create(firstRoot);
firstMiddle.firstMiddleTag = 1;
let first = Object.create(firstMiddle);
first.firstReceiverTag = 1;

let secondRoot = {
    padding: 1,
    x: 22,
};
let secondMiddle = Object.create(secondRoot);
secondMiddle.secondMiddleTag = 1;
secondMiddle.extraTag = 2;
let second = Object.create(secondMiddle);
second.secondReceiverTag = 1;

for (let i = 0; i < 20; i++) {
    LoadX(first);
    LoadX(second);
}

print(ArkTools.arkSteedCompileSync(LoadX));
print(LoadX(first));
print(LoadX(second));

firstMiddle.x = 33;
print(LoadX(first));
print(LoadX(second));
