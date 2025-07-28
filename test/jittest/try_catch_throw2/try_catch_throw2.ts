/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
function assert(b) {
    if (!b)
        throw new Error("bad value")
}

let expString = "a";
let exponentialBlowup = false;
let shouldBreak = false;
function foo(fun, left, right) {
    let x = fun();
    let r = left + right;
    try {
        expString = expString + expString;
    } catch(e) {
        shouldBreak = true;
    }
    return x + r;
}

function blah() { return "blah" };

for (let i = 0; i < 1; i++) {
    assert(foo(blah, "b", "a") === "blahba");
    if (!exponentialBlowup)
        expString = "a";
}
ArkTools.jitCompileAsync(foo);
print(ArkTools.waitJitCompileFinish(foo));
exponentialBlowup = true;
while(true) {
    assert(foo(blah, "a", "b") === "blahab");
    if (shouldBreak) {
        break;
    }
}