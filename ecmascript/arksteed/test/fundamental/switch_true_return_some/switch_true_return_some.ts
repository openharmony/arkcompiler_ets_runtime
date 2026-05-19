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
function switch_true_return_some(x, a, b, c, d)
{
    let result = a;
    switch (true) {
        case x > 0 && x <= 10:
            result = a + b;
            break;
        case x > 10 && x <= 20:
            return b * 2 + c;
        case x > 20:
            result = c + d;
            break;
    }
    return result + 5;
}

ArkTools.arkSteedCompileSync(switch_true_return_some);


print(switch_true_return_some(5, 10, 20, 30, 40));
print(switch_true_return_some(15, 10, 20, 30, 40));
print(switch_true_return_some(25, 10, 20, 30, 40));
print(switch_true_return_some(-1, 10, 20, 30, 40));

print(switch_true_return_some(5, 1, 20, 300, 4000));
print(switch_true_return_some(15, 1, 20, 300, 4000));
print(switch_true_return_some(25, 1, 20, 300, 4000));
print(switch_true_return_some(-1, 1, 20, 300, 4000));
