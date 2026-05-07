/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
import { B2RayCastInput } from '../Collision/b2Collision';
import { B2FixtureProxy } from '../Dynamics/b2Fixture';
interface B2QueryWrapper {
  queryCallback(proxyId: number): boolean;
}
interface B2RayCastWrapper {
  rayCastCallback(input: B2RayCastInput, proxyId: number): number;
}
interface B2BroadPhaseWrapper {
  addPair(proxyUserDataA: B2FixtureProxy, proxyUserDataB: B2FixtureProxy);
}

export { B2QueryWrapper, B2BroadPhaseWrapper, B2RayCastWrapper };
