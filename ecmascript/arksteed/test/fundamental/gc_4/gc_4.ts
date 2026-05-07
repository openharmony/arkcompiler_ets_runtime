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
type Point = {
    x: number;
    y: number;
};

let seed = 1;
function myRandom() {
    seed = (seed * 1103515245 + 12345) & 0x7fffffff;
    return seed;
}

function consumePoint(dest: Point, p: Point): void {
    dest.x = (dest.x + p.y) & 0x7FFF_FFFF;
    dest.y = (dest.y + p.x) & 0x7FFF_FFFF;
}

function gc_4(): Point {
    const LOOP_COUNT = 50_000_000;
    const dest: Point = {
        x: 0,
        y: 0
    };
    for (let i = 0; i < LOOP_COUNT; i++) {
        const p: Point = {x: myRandom(), y: myRandom()};
        consumePoint(dest, p);
    }   
    return dest;
}

ArkTools.arkSteedCompileAsync(myRandom);
ArkTools.arkSteedCompileAsync(consumePoint);
ArkTools.arkSteedCompileAsync(gc_4);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

const result: Point = gc_4();
print(result.x);
print(result.y);
