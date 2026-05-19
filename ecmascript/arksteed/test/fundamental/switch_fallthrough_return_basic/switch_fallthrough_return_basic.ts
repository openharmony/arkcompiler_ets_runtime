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
function switch_fallthrough_return_basic(x, a, b, c, d)
{
    let base = a;
    switch (x) {
        case 1:
            base = base + b;
        case 2:
            return base + b;
        case 3: {
            let t = base + c;
            return t;
        }
        default: {
            let t = base + d + a;
            return t;
        }
    }
}

ArkTools.arkSteedCompileSync(switch_fallthrough_return_basic);


print(switch_fallthrough_return_basic(1, 10, 5, 3, 7));
print(switch_fallthrough_return_basic(2, 10, 5, 3, 7));
print(switch_fallthrough_return_basic(3, 10, 5, 3, 7));
print(switch_fallthrough_return_basic(9, 10, 5, 3, 7));