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
let catchcnt = 0;
function exception_throw_catch() {
    try {
        throw new Error("error");
    } catch (e) {
        catchcnt++;
    }
}

function exception_throw_call() {
    exception_throw_catch();
}

function exception_throw() {
    exception_throw_call();
}

for (let i = 0; i < 10; i++) {
    exception_throw();
}
ArkTools.arkSteedCompileSync(exception_throw_catch);
ArkTools.arkSteedCompileSync(exception_throw_call);
ArkTools.arkSteedCompileSync(exception_throw);


for (let i = 0; i < 20; i++) {
    exception_throw();
    ArkTools.gc(0);
}

print(catchcnt);
