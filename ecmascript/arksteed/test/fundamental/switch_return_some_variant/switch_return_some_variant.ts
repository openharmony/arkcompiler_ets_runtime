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
function switch_return_some_variant(x, a, b, c, d)
{
    let base = a * 10;
    let result = 0;
    switch (x) {
        case 1:
            result = a + 10;
            break;
        case 2:
            result = b * 2 + 5;
            break;
        default:
            result = base;
            break;
    }
    return result + c - d;
}

ArkTools.arkSteedCompileSync(switch_return_some_variant);


print(switch_return_some_variant(1, 10, 20, 30, 5));
print(switch_return_some_variant(2, 10, 20, 30, 5));
print(switch_return_some_variant(5, 10, 20, 30, 5));