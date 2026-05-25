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
function switch_case_fallthrough_return(x, a, b, c, d)
{
    let result = 0;
    switch (x) {
        case 1:
            result += a;
        case 2:
            result += b;
            return result;
        case 3:
            result += c;
            break;
        default:
            result += d;
            return result;
    }
    return result * 10;
}

ArkTools.arkSteedCompileSync(switch_case_fallthrough_return);


print(switch_case_fallthrough_return(1, 10, 20, 30, 40));
print(switch_case_fallthrough_return(2, 10, 20, 30, 40));
print(switch_case_fallthrough_return(3, 10, 20, 30, 40));
print(switch_case_fallthrough_return(5, 10, 20, 30, 40));