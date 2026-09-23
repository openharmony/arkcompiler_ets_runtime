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

//! METHOD      typeof_stub_fallback
//! HAS         CallCommonStub TypeOf
// Test: typeof on an untyped parameter keeps the generic TypeOf stub call and
// produces the correct runtime result for every input kind.

declare function print(arg: string): string;

declare class ArkTools {
    static arkSteedCompileSync<T extends Function>(func: T): T;
}

function typeof_stub_fallback(value: Object | null): string {
    return typeof value;
}

ArkTools.arkSteedCompileSync(typeof_stub_fallback);

print(typeof_stub_fallback(null));
print(typeof_stub_fallback({ a: 1 }));
print(typeof_stub_fallback([1, 2]));
