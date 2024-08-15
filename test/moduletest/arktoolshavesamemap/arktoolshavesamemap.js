/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

function addprop(o, p) {
    o[p] = 0;
    return o;
}

function propdict(o, sz) {
    for (let i = 0; i < sz; ++i) {
        addprop(o, "prop" + i)
    }
    return o
}

function warmup(fn, num) {
    for (let i = 0; i < num; ++i) {
        print(fn.name + " warmup: " + i);
        fn();
    }
}

warmup((function testICTransition() {
    let x = {};
    x.a = 0;
    let y = {};
    print(!ArkTools.haveSameMap(x, y));
    y.a = 0;
    print(ArkTools.haveSameMap(x, y));
}), 3);

warmup((function testObjLiterals() {
    print(!ArkTools.haveSameMap({ a: 0, b: 0 }, { b: 0, a: 0 }))
    print(ArkTools.haveSameMap({ a: 0, b: 0 }, { a: 0, b: 0 }))
}), 0); // NOTE: broken with force-gc

warmup((function testICByValueTransition() {
    let x = addprop({}, "a");
    let y = {}
    print(!ArkTools.haveSameMap(x, y))
    y.a = 0
    print(ArkTools.haveSameMap(x, y))
}), 3);

warmup((function testDictTransition() {
    const dictSzHeuristic = 1024;
    print(ArkTools.haveSameMap(
        propdict({}, dictSzHeuristic / 2),
        propdict({}, dictSzHeuristic / 2)));
    print(!ArkTools.haveSameMap(
        propdict({}, dictSzHeuristic * 2),
        propdict({}, dictSzHeuristic * 2)));
}), 0); // NOTE: broken with force-gc
