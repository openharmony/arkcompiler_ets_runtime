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
class InstanceofInheritanceAnimal {}

class InstanceofInheritanceDog extends InstanceofInheritanceAnimal {}

function instanceof_inheritance() {
    let dog = new InstanceofInheritanceDog();
    print(dog instanceof InstanceofInheritanceDog ? 1 : 0);
    print(dog instanceof InstanceofInheritanceAnimal ? 1 : 0);
}

ArkTools.arkSteedCompileSync(instanceof_inheritance);


instanceof_inheritance();
