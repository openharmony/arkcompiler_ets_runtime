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
function exception_call_catch() {
    try {
        throw new Error("error");
    } catch (e) {
        catchcnt++;
    }
}

function exception_call() {
    exception_call_catch();
    let arr = [];
    for(var i = 0; i < 10; i++){
        arr.push('exception_call')
    }
    for(var i = 0; i < 10; i++){
        arr.push('push............')
    }
    exception_call_catch();
}

for (let i = 0; i < 10; i++) {
    exception_call();
}
ArkTools.arkSteedCompileAsync(exception_call_catch);
ArkTools.arkSteedCompileAsync(exception_call);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}
for (let i = 0; i < 20; i++) {
    exception_call();
    ArkTools.gc(0);
}

print(catchcnt);
