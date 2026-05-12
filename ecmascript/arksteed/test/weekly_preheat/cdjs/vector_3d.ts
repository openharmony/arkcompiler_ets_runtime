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

import { Vector2D } from './vector_2d';

export class Vector3D {
  x: number;
  y: number;
  z: number;

  constructor(x: number, y: number, z: number) {
    this.x = x;
    this.y = y;
    this.z = z;
  }

  plus(other: Vector3D): Vector3D {
    return new Vector3D(this.x + other.x, this.y + other.y, this.z + other.z);
  }

  minus(other: Vector3D): Vector3D {
    return new Vector3D(this.x - other.x, this.y - other.y, this.z - other.z);
  }

  dot(other: Vector3D): number {
    return this.x * other.x + this.y * other.y + this.z * other.z;
  }

  squaredMagnitude(): number {
    return this.dot(this);
  }

  magnitude(): number {
    return Math.sqrt(this.squaredMagnitude());
  }

  times(amount: number): Vector3D {
    return new Vector3D(this.x * amount, this.y * amount, this.z * amount);
  }

  as2D(): Vector2D {
    return new Vector2D(this.x, this.y);
  }

  toString(): string {
    return '[' + this.x + ', ' + this.y + ', ' + this.z + ']';
  }
}
