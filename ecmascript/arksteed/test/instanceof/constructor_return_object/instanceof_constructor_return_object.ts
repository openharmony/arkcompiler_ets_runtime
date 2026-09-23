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
class InstanceofConstructorReturnObjectBase {
    value: number;

    constructor(value: number) {
        this.value = value;
        return { value: value + 1 };
    }
}

function instanceof_constructor_return_object() {
    let obj = new InstanceofConstructorReturnObjectBase(5);
    print(obj instanceof InstanceofConstructorReturnObjectBase ? 1 : 0);
    print(obj instanceof Object ? 1 : 0);
}

ArkTools.arkSteedCompileSync(instanceof_constructor_return_object);


instanceof_constructor_return_object();
