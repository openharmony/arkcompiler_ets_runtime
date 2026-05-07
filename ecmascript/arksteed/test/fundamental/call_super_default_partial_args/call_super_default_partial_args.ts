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
class CallSuperDefaultPartialArgsBase {
    make: string;
    model: string;
    year: number;

    constructor(make: string = "Unknown", model: string = "Model", year: number = 2020) {
        this.make = make;
        this.model = model;
        this.year = year;
    }
}

class CallSuperDefaultPartialArgsDerived extends CallSuperDefaultPartialArgsBase {
    constructor(make?: string, model?: string) {
        super(make, model);
    }
}

ArkTools.arkSteedCompileAsync(CallSuperDefaultPartialArgsBase);

let time = Date.now();
for (let cur = Date.now(); cur - time < 1000; cur = Date.now()) {}

let car1 = new CallSuperDefaultPartialArgsDerived("Toyota");
print(car1.make);
print(car1.model);
print(car1.year);

let car2 = new CallSuperDefaultPartialArgsDerived("Honda", "Civic");
print(car2.make);
print(car2.model);
print(car2.year);
