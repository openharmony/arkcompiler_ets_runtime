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
declare var ArkTools:any;
import {DsHelper} from "./ds_helper"
export class DataManager {
    private dataHelper: DsHelper = new DsHelper()
    private constructor() {}

    public static getInstance(): DataManager {
        if (!globalThis.dataManager) {
            globalThis.dataManager = new DataManager()
        }
        return globalThis.dataManager
    }

    insertEvent() {
        let promise = this.dataHelper.getDsHelper()
        let res = ArkTools.currentEnvIsGlobal(this.dataHelper.getDsHelper)
        if (res !== true) {
            print("lexical env of async function should not be changed!")
        }
        promise.then((dsHelper) => {
            print("callback")
            print(dsHelper)
        })
    }
}