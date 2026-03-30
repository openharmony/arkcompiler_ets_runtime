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

/*
 * @tc.name:weakmap
 * @tc.desc:test weakmap function
 * @tc.type: FUNC
 */

let mp = new WeakMap();

let registry = new FinalizationRegistry((result) => {
    print(result);
});

{
    function f() {
        let k = {};
        let v = {};
        v.x = k;
        registry.register(k, "collect1 success");
        mp.set(k, v);
    }
    f();
    ArkTools.forceFullGC();
}