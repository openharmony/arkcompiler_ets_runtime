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
class Constants {
    static PI: number = 3;
    static E: number = 2;

    static circleArea(r: number): number {
        return Constants.PI * r * r;
    }

    static exp(x: number): number {
        return Constants.E * x;
    }
}

function static_computation(r: number, x: number): number {
    return Constants.circleArea(r) + Constants.exp(x);
}

ArkTools.arkSteedCompileSync(Constants);
ArkTools.arkSteedCompileSync(Constants.circleArea);
ArkTools.arkSteedCompileSync(Constants.exp);
ArkTools.arkSteedCompileSync(static_computation);


print(static_computation(5, 10));
