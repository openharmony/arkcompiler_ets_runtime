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

function random() {
    return "blah";
}

function foo(o) {
    let x = o.g;
    let y = o.y;
    let j = random();
    try {
        j = o.f;
    } catch(e) {
        assert(j === "blah");
        return x + y + 1;
    }
    return x + y;
}

var flag = false;

let o2 = {
    g: "g",
    y: "y",
    get f() {
        if (flag)
            throw new Error("blah");
        return "get f";
    }
}

for (let i = 0; i < 1; i++) {
    assert(foo(o2) === "gy");
}
ArkTools.jitCompileAsync(foo);
print(ArkTools.waitJitCompileFinish(foo));
flag = true;
print(foo(o2));