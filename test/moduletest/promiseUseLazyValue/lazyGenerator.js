/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

/*
 * @tc.name:promiseUseLazyValue
 * @tc.desc:test promiseUseLazyValue
 * @tc.type: FUNC
 * @tc.require: issueIBBVLX
 */
import lazy { LazyLogForLazyGenerator } from "./lazyLogForLazyGenerator"
export function* lazyGenerator() {
    LazyLogForLazyGenerator.log("lazyGenerator before 1 return");
    yield 1;
    LazyLogForLazyGenerator.log("lazyGenerator before 2 return");
    yield 2;
    LazyLogForLazyGenerator.log("lazyGenerator before 3 return");
    yield 3;
    LazyLogForLazyGenerator.log("lazyGenerator before 4 return");
    yield 4;
    LazyLogForLazyGenerator.log("lazyGenerator before 5 return");
    return 5;
}