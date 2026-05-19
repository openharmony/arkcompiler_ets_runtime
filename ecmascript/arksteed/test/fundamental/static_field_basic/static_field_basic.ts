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
class Config {
    static version: number = 42;
    static name: string = "arksteed";
}

function static_field_basic(): number {
    let r = Config.version;
    Config.version = 100;
    r += Config.version;
    return r;
}

ArkTools.arkSteedCompileSync(Config);
ArkTools.arkSteedCompileSync(static_field_basic);


print(static_field_basic());
