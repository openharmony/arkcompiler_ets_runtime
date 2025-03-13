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

const v1 = new Map();
function f2(a3) {
    const o8 = {
        __proto__: v1,
        "c": a3,
    };
    return o8;
}
let v9 = f2(Map);
let v10;
try { v10 = v9.entries(); } catch(e) { print(e) }
function f13() {

}
let v24 = f13();
try { [v24, v9] = v10 } catch(e) { print(e) }
