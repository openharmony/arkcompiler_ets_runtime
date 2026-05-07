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

const MAX_VALUE: number = 1000;
const MAX_Z_VALUE: number = 10;
const SIZE_DOUBLE: number = 2;

export class Constants {
  static minX: number = 0;
  static minY: number = 0;
  static maxX: number = MAX_VALUE;
  static maxY: number = MAX_VALUE;
  static minZ: number = 0;
  static maxZ: number = MAX_Z_VALUE;
  static proximityRadius: number = 1;
  static goodVoxelSize: number = Constants.proximityRadius * SIZE_DOUBLE;
}
