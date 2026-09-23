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

function do_throw(n: number, k: number) {
    throw {x: n + k, y: n - k};  // JIT -> JIT
}

function exception_throw_number_3(n: number, k: number) {
    try {
        do_throw(9, 7);
    } catch (e) {
        print(typeof(e));
        print((e as {x: number, y: number}).x);
        print((e as {x: number, y: number}).y);
    }
}

ArkTools.arkSteedCompileSync(do_throw);
ArkTools.arkSteedCompileSync(exception_throw_number_3);
exception_throw_number_3();
