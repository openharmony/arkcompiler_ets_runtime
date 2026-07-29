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

function insufficientProfileDeopt(takeHotPath: boolean, input: number): number {
    let value = input;
    if (takeHotPath) {
        value += 1;
    } else {
        value += 40;
    }
    return value + 1;
}

function returnFortyTwo(): number {
    return 42;
}

function insufficientCallProfile(takeHotPath: boolean): number {
    if (takeHotPath) {
        return 1;
    }
    return returnFortyTwo();
}

function insufficientLoopProfile(enterLoop: boolean, input: number): number {
    let value = input;
    while (enterLoop) {
        value += 40;
        enterLoop = false;
    }
    return value + 1;
}

// Warmup enters the loop and executes its prefix, but always breaks before
// the cold tail reaches the loop backedge.
function insufficientLoopBackedgeProfile(breakBeforeBackedge: boolean, input: number): number {
    let value = input;
    while (value < 10) {
        value += 1;
        if (breakBeforeBackedge) {
            break;
        }
        value += 40;
    }
    return value;
}

function throwFromColdPath(): number {
    throw new Error("cold path");
}

function insufficientTryProfile(takeHotPath: boolean): number {
    try {
        if (takeHotPath) {
            return 1;
        }
        return throwFromColdPath();
    } catch (e) {
        return 42;
    }
}

for (let i = 0; i < 10; i++) {
    insufficientProfileDeopt(true, i);
    insufficientCallProfile(true);
    insufficientLoopProfile(false, i);
    insufficientLoopBackedgeProfile(true, i);
    insufficientTryProfile(true);
}

ArkTools.arkSteedCompileSync(insufficientProfileDeopt);
ArkTools.arkSteedCompileSync(insufficientCallProfile);
ArkTools.arkSteedCompileSync(insufficientLoopProfile);
ArkTools.arkSteedCompileSync(insufficientLoopBackedgeProfile);
ArkTools.arkSteedCompileSync(insufficientTryProfile);
print(insufficientProfileDeopt(true, 1));
print(insufficientProfileDeopt(false, 1));
print(insufficientCallProfile(false));
print(insufficientLoopProfile(true, 1));
print(insufficientLoopBackedgeProfile(false, 0));
print(insufficientTryProfile(false));
