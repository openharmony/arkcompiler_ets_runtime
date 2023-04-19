/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

declare function print(str:any):string;
function envScope1() {
    let a = 1
    function envScope2() {
        let b = 2
        async function getDsHelper() {
            await 1
            print(a)
            print(b)
            return new Promise((resolve) => {
                resolve(this.dsHelper)
            })
        }
        getDsHelper()
    }
    envScope2()
}
envScope1()
