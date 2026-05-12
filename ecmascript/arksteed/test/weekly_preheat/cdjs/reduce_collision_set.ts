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

import { Constants } from './constants';
import { Vector2D } from './vector_2d';
import { Vector3D } from './vector_3d';
import { RedBlackTree } from './red_black_tree';
import { Motion } from './motion';

const RADIUS_DIVISOR: number = 2;

export const drawMotionOnVoxelMap = ((): ((voxelMap: RedBlackTree, motion: Motion) => void) => {
  const voxelSize = Constants.goodVoxelSize;
  const horizontal = new Vector2D(voxelSize, 0);
  const vertical = new Vector2D(0, voxelSize);

  let voxelHash = (position: Vector3D): Vector2D => {
    let xDiv = (position.x / voxelSize) | 0;
    let yDiv = (position.y / voxelSize) | 0;

    let result = new Vector2D(voxelSize * xDiv, voxelSize * yDiv);

    if (position.x < 0) {
      result.x -= voxelSize;
    }
    if (position.y < 0) {
      result.y -= voxelSize;
    }

    return result;
  };

  return (voxelMap: RedBlackTree, motion: Motion): void => {
    let seen: RedBlackTree = new RedBlackTree();

    let putIntoMap = (voxel: Vector2D): void => {
      let array = voxelMap.get(voxel) as Array<Motion> | null;
      if (array === null) {
        array = [];
      }
      voxelMap.put(voxel, array);
      array.push(motion);
    };

    let isInVoxel = (voxel: Vector2D): boolean => {
      return inVoxelByCondition(voxel, motion, voxelSize);
    };

    let recurse = (nextV: Vector2D): void => {
      if (!isInVoxel(nextV)) {
        return;
      }
      if (seen.put(nextV, true)) {
        return;
      }

      putIntoMap(nextV);
      recurse(nextV.minus(horizontal));
      recurse(nextV.plus(horizontal));
      recurse(nextV.minus(vertical));
      recurse(nextV.plus(vertical));
      recurse(nextV.minus(horizontal).minus(vertical));
      recurse(nextV.minus(horizontal).plus(vertical));
      recurse(nextV.plus(horizontal).minus(vertical));
      recurse(nextV.plus(horizontal).plus(vertical));
    };

    recurse(voxelHash(motion.posOne));
  };
})();

function inVoxelByCondition(voxel: Vector2D, motion: Motion, voxelSize: number): boolean {
  if (voxel.x > Constants.maxX || voxel.x < Constants.minX || voxel.y > Constants.maxY || voxel.y < Constants.minY) {
    return false;
  }
  let ini = motion.posOne;
  let fin = motion.posTwo;
  let vS = voxelSize;
  let r = Constants.proximityRadius / RADIUS_DIVISOR;
  let vX = voxel.x;
  let x0 = ini.x;
  let xv = fin.x - ini.x;
  let vY = voxel.y;
  let y0 = ini.y;
  let yv = fin.y - ini.y;
  let lowX: number = (vX - r - x0) / xv;
  let highX: number = (vX + vS + r - x0) / xv;
  if (xv < 0) {
    let tmp = lowX;
    lowX = highX;
    highX = tmp;
  }
  let lowY: number = (vY - r - y0) / yv;
  let highY: number = (vY + vS + r - y0) / yv;
  if (yv < 0) {
    let tmp = lowY;
    lowY = highY;
    highY = tmp;
  }

  let condition1 = xv === 0 && vX <= x0 + r && x0 - r <= vX + vS;
  let condition2 = (lowX <= 1 && 1 <= highX) || (lowX <= 0 && 0 <= highX) || (0 <= lowX && highX <= 1);
  let condition3 = yv === 0 && vY <= y0 + r && y0 - r <= vY + vS;
  let condition4 = getCondition4(lowY, highY);
  let condition5 = getCondition5(xv, yv, lowX, highX, lowY, highY);

  return (condition1 || condition2) && (condition3 || condition4) && condition5;
}

function getCondition4(lowY: number, highY: number): boolean {
  return (lowY <= 1 && 1 <= highY) || (lowY <= 0 && 0 <= highY) || (0 <= lowY && highY <= 1);
}

function getCondition5(xv: number, yv: number, lowX: number, highX: number, lowY: number, highY: number): boolean {
  return xv === 0 || yv === 0 || (lowY <= highX && highX <= highY) || (lowY <= lowX && lowX <= highY) || (lowX <= lowY && highY <= highX);
}

export function reduceCollisionSet(motions: Array<Motion>): Array<Array<Motion>> {
  let voxelMap: RedBlackTree = new RedBlackTree();
  for (const motion of motions) {
    drawMotionOnVoxelMap(voxelMap, motion);
  }

  let result: Array<Array<Motion>> = [];
  voxelMap.forEach((key, value) => {
    let newValue = value as Array<Motion>;
    if (newValue.length > 1) {
      result.push(newValue);
    }
  });
  return result;
}
