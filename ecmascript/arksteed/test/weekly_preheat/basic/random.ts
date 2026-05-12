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

let RNG_FIXED_SEED: number = 49734321;
let RANDOM_MIN_BOND: number = 0.0;
let RANDOM_MAX_BOND: number = 1.0;
let CONST_ZERO: number = 0;
let MAX_INT_32BIT: number = 0xffffffff;
let MAX_INT_28BIT: number = 0xfffffff;

let RAND_SEED_OFFSET1: number = 0x7ed55d16;
let RAND_SEED_OFFSET2: number = 0xc761c23c;
let RAND_SEED_OFFSET3: number = 0x165667b1;
let RAND_SEED_OFFSET4: number = 0xd3a2646c;
let RAND_SEED_OFFSET5: number = 0xfd7046c5;
let RAND_SEED_OFFSET6: number = 0xb55a4f09;

let RAND_SEED_OFFSET_SHORT1: number = 12;
let RAND_SEED_OFFSET_SHORT2: number = 19;
let RAND_SEED_OFFSET_SHORT3: number = 5;
let RAND_SEED_OFFSET_SHORT4: number = 9;
let RAND_SEED_OFFSET_SHORT5: number = 3;
let RAND_SEED_OFFSET_SHORT6: number = 16;

let RAND_DENOMINATOR: number = 0x10000000;

class RandomGenerator {
  fixedSeed: number;

  constructor() {
    this.fixedSeed = RNG_FIXED_SEED;
  }

  createRNG(): () => number {
    return () => {
      this.fixedSeed = (this.fixedSeed + RAND_SEED_OFFSET1 + (this.fixedSeed << RAND_SEED_OFFSET_SHORT1)) & MAX_INT_32BIT;
      this.fixedSeed = (this.fixedSeed ^ RAND_SEED_OFFSET2 ^ (this.fixedSeed >>> RAND_SEED_OFFSET_SHORT2)) & MAX_INT_32BIT;
      this.fixedSeed = (this.fixedSeed + RAND_SEED_OFFSET3 + (this.fixedSeed << RAND_SEED_OFFSET_SHORT3)) & MAX_INT_32BIT;
      this.fixedSeed = ((this.fixedSeed + RAND_SEED_OFFSET4) ^ (this.fixedSeed << RAND_SEED_OFFSET_SHORT4)) & MAX_INT_32BIT;
      this.fixedSeed = (this.fixedSeed + RAND_SEED_OFFSET5 + (this.fixedSeed << RAND_SEED_OFFSET_SHORT5)) & MAX_INT_32BIT;
      this.fixedSeed = (this.fixedSeed ^ RAND_SEED_OFFSET6 ^ (this.fixedSeed >>> RAND_SEED_OFFSET_SHORT6)) & MAX_INT_32BIT;
      return (this.fixedSeed & MAX_INT_28BIT) / RAND_DENOMINATOR;
    };
  }

  createRNGWithFixedSeed(): () => number {
    return this.createRNG();
  }
}

export { RandomGenerator };
