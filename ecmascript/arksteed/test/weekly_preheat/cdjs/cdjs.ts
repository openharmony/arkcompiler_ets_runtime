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

import { Simulator } from './simulator';
import { CollisionDetector } from './collision_detector';
import { currentTime } from './util';
import { Collision } from './collision';

const TIME_DIVISOR: number = 10.0;
const VERBOSITY_TWO: number = 2;
const VERBOSITY_THREE: number = 3;
const AIRCRAFT_NUM: number = 1000;
const FRAMES_NUM: number = 18;
const EXPECTED_COLLISIONS: number = 1336;
const LOOP_TIMES: number = 5;
const NUMBER_ONE_THOUSAND: number = 1000;

let isDebug: boolean = false;
function debugLog(msg: string): void {
  if (isDebug) {
    print(msg);
  }
}

class Configuration {
  verbosity: number;
  numAircraft: number;
  numFrames: number;
  expectedCollisions: number;
  exclude: number;

  constructor(verbosity: number, numAircraft: number, numFrames: number, expectedCollisions: number, exclude: number) {
    this.verbosity = verbosity;
    this.numAircraft = numAircraft;
    this.numFrames = numFrames;
    this.expectedCollisions = expectedCollisions;
    this.exclude = exclude;
  }
}

class Result {
  time: number;
  numCollisions: number;
  collisions: Array<Collision> = [];

  constructor(time: number, numCollisions: number) {
    this.time = time;
    this.numCollisions = numCollisions;
  }
}

function benchmarkImpl(configuration: Configuration): void {
  // debugLog(`Initialization parameters： ` + `${JSON.stringify(configuration)}`);
  let verbosity = configuration.verbosity;
  let numAircraft = configuration.numAircraft;
  let numFrames = configuration.numFrames;
  let expectedCollisions = configuration.expectedCollisions;
  let exclude = configuration.exclude;
  let simulator = new Simulator(numAircraft);
  let detector = new CollisionDetector();
  let lastTime = currentTime();
  let results: Array<Result> = [];
  for (let i = 0; i < numFrames; ++i) {
    let time = i / TIME_DIVISOR;
    let collisions = detector.handleNewFrame(simulator.simulate(time));
    // debugLog(`Collision data:  ` + `${JSON.stringify(collisions)}`);
    let before = lastTime;
    let after = currentTime();
    lastTime = after;
    let result = new Result(after - before, collisions.length);
    if (verbosity >= VERBOSITY_TWO) {
      print('CDJS: ' + result.time);
    }
    if (verbosity >= VERBOSITY_THREE) {
      result.collisions = collisions;
    }
    results.push(result);
  }
  results.splice(0, exclude);
  if (verbosity >= 1) {
    for (let i = 0; i < results.length; ++i) {
      let string = 'Frame ' + i + ': ' + results[i].time + ' ms.';
      if (results[i].numCollisions > 0) {
        string += ' (' + results[i].numCollisions + ' collisions.)';
      }
      print(string);
      if (verbosity >= VERBOSITY_TWO && results[i]?.collisions.length) {
        print('    Collisions: ' + results[i].collisions);
      }
    }
  }
  // Check results.
  let actualCollisions = 0;
  for (const result of results) {
    actualCollisions += result.numCollisions;
  }
  // debugLog(`Actual Collision Results: ` + `${actualCollisions}` + ` , Expecting Collision Results： ` + `${expectedCollisions}`);
  if (actualCollisions !== expectedCollisions) {
    throw new Error('Bad number of collisions: ' + actualCollisions + ' (expected ' + expectedCollisions + ')');
  }
}

function benchmark(): void {
  benchmarkImpl(new Configuration(0, AIRCRAFT_NUM, FRAMES_NUM, EXPECTED_COLLISIONS, 0));
}

/**
 * @State
 * @Tags Jetstream2
 */
class Benchmark {
  /**
   * @Benchmark
   */
  runIteration(): void {
    benchmark();
  }
}

function run(): void {
  let startTime = ArkTools.timeInUs() / NUMBER_ONE_THOUSAND;
  let benchmark = new Benchmark();
  for (let index = 0; index < LOOP_TIMES; index++) {
    benchmark.runIteration();
  }
  let endTime = ArkTools.timeInUs() / NUMBER_ONE_THOUSAND;
  print('cdjs: ms = ' + (endTime - startTime));
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  run();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

run();
