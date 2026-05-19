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

const SPLAY_TREE_SIZE: number = 8000;
const SPLAY_TREE_MODIFICATIONS: number = 80;
const SPLAY_TREE_PAYLOAD_DEPTH: number = 5;
const RANDOM_INTIAL_NUMBER: number = 49734321;
const TIMES_KILO_MILLISECONDS: number = 1000;
const DEFAULT_MIN_ITERATIONS: number = 32;
const REFERENCE_SCORE_MIN: number = 81491;
const REFERENCE_SCORE_MAX: number = 2739514;
const RANDOM_NUMBER_D16: number = 0x7ed55d16;
const RANDOM_NUMBER_23C: number = 0xc761c23c;
const RANDOM_NUMBER_7B1: number = 0x165667b1;
const RANDOM_NUMBER_46C: number = 0xd3a2646c;
const RANDOM_NUMBER_6C5: number = 0xfd7046c5;
const RANDOM_NUMBER_F09: number = 0xb55a4f09;
const RANDOM_NUMBER_FFF: number = 0xffffffff;
const RANDOM_NUMBER_000: number = 0x10000000;
const RANDOM_NUMBER_12: number = 12;
const RANDOM_NUMBER_19: number = 19;
const RANDOM_NUMBER_5: number = 5;
const RANDOM_NUMBER_9: number = 9;
const RANDOM_NUMBER_3: number = 3;
const RANDOM_NUMBER_16: number = 16;
const PERCENTAGE_RADIO: number = 100;
const TOFIXED_NUMBER2: number = 2;
const TOFIXED_NUMBER3: number = 3;
const TOFIXED_NUMBER0: number = 0;
const INSERT_NODE_VALUE_REC: number = 20;
const NODE_PAYLOAD_NUM_0: number = 0;
const NODE_PAYLOAD_NUM_1: number = 1;
const NODE_PAYLOAD_NUM_2: number = 2;
const NODE_PAYLOAD_NUM_3: number = 3;
const NODE_PAYLOAD_NUM_4: number = 4;
const NODE_PAYLOAD_NUM_5: number = 5;
const NODE_PAYLOAD_NUM_6: number = 6;
const NODE_PAYLOAD_NUM_7: number = 7;
const NODE_PAYLOAD_NUM_8: number = 8;
const NODE_PAYLOAD_NUM_9: number = 9;

let splaySamples: number[] = [];
let seed: number = RANDOM_INTIAL_NUMBER;
let splaySampleTimeStart: number = 0.0;

// Performance.now is used in latency benchmarks, the fallback is Date.now.
function performanceNow(): number {
  return ArkTools.timeInUs() / TIMES_KILO_MILLISECONDS;
}
function deBugLog(str: string): void {
  print(str);
}

interface Runner {
  notifyError: (name: string, error: Error) => void;
  notifyStep?: (result: string) => void;
}

class SuiteData {
  runs: number;
  elapsed: number;
  constructor(runs: number, elapsed: number) {
    this.runs = runs;
    this.elapsed = elapsed;
  }
}

class Benchmark {
  name: string;
  doWarmup: boolean;
  doDeterministic: boolean;
  run: () => void;
  setup: () => void;
  tearDown: () => void;
  latencyResult: () => number[];
  minIterations: number;

  constructor(
    name: string,
    doWarmup: boolean,
    doDeterministic: boolean,
    latencyResult: () => number[],
    run: () => void,
    setup: () => void,
    tearDown: () => void,
    minIterations: number
  ) {
    this.name = name;
    this.doWarmup = doWarmup;
    this.doDeterministic = doDeterministic;
    this.run = run;
    this.setup = setup;
    this.tearDown = tearDown;
    this.latencyResult = latencyResult;
    this.minIterations = minIterations || DEFAULT_MIN_ITERATIONS;
  }
}
// Benchmark results hold the benchmark and the measured time used to
// run the benchmark. The benchmark score is computed later once a
// full benchmark suite has run to completion. If latency is set to 0
// then there is no latency score for this benchmark.

class BenchmarkResult {
  benchmark: Benchmark;
  time: number;
  latency: number;

  constructor(benchmark: Benchmark, time: number, latency: number) {
    this.benchmark = benchmark;
    this.time = time;
    this.latency = latency;
  }
  // Automatically convert results to numbers. Used by the geometric
  // mean computation.
  valueOf(): number {
    return this.time;
  }
}

// Suites of benchmarks consist of a name and the set of benchmarks in
// addition to the reference timing that the final score will be based
// on. This way, all scores are relative to a reference run and higher
// scores implies better performance.

/**
 * @State
 * @Tags Jetstream2
 */
class BenchmarkSuite {
  name: string;
  reference: number[];
  benchmarks: Benchmark[];
  results: BenchmarkResult[] = [];
  runner: Runner | undefined = undefined;
  // Keep track of all declared benchmark suites.
  static suites: BenchmarkSuite[] = [];
  static version: string = '9';
  static scores: number[] = [];
  constructor(name: string, reference: number[], benchmarks: Benchmark[]) {
    this.name = name;
    this.reference = reference;
    this.benchmarks = benchmarks;
    BenchmarkSuite.suites.push(this);
  }
  // To make the benchmark results predictable, we replace Math.random
  // with a 100% deterministic alternative.
  static resetRNG(): void {
    seed = RANDOM_INTIAL_NUMBER;
  }
  static random(): number {
    // Robert Jenkins' 32 bit integer hash function.
    seed = (seed + RANDOM_NUMBER_D16 + (seed << RANDOM_NUMBER_12)) & RANDOM_NUMBER_FFF;
    seed = (seed ^ RANDOM_NUMBER_23C ^ (seed >>> RANDOM_NUMBER_19)) & RANDOM_NUMBER_FFF;
    seed = (seed + RANDOM_NUMBER_7B1 + (seed << RANDOM_NUMBER_5)) & RANDOM_NUMBER_FFF;
    seed = ((seed + RANDOM_NUMBER_46C) ^ (seed << RANDOM_NUMBER_9)) & RANDOM_NUMBER_FFF;
    seed = (seed + RANDOM_NUMBER_6C5 + (seed << RANDOM_NUMBER_3)) & RANDOM_NUMBER_FFF;
    seed = (seed ^ RANDOM_NUMBER_F09 ^ (seed >>> RANDOM_NUMBER_16)) & RANDOM_NUMBER_FFF;
    return (seed & RANDOM_NUMBER_FFF) / RANDOM_NUMBER_000;
  }

  runStep(runner: Runner): (() => void) | null {
    BenchmarkSuite.resetRNG();
    this.results = [];
    this.runner = runner;
    const length = this.benchmarks.length;
    let index = 0;
    let data: SuiteData | null;

    // Run the setup, the actual benchmark, and the tear down in three
    // separate steps to allow the framework to yield between any of the
    // steps.
    /**
     * @Setup
     */
    let runNextSetup = (): (() => void) => {
      if (index < length) {
        this.benchmarks[index].setup();
        return runNextBenchmark;
      }
      return () => {};
    };

    /**
     * @Benchmark
     */
    let runNextBenchmark = (): (() => void) => {
      data = this.runSingleBenchmark(this.benchmarks[index], data);
      // If data is null, we're done with this benchmark.
      return data === null ? runNextTearDown : runNextBenchmark();
    };
    /**
     * @Teardown
     */
    let runNextTearDown = (): (() => void) => {
      this.benchmarks[index++].tearDown();
      return runNextSetup;
    };
    return runNextSetup();
  }
  // Runs all registered benchmark suites and optionally yields between
  // each individual benchmark to avoid running for too long in the
  // context of browsers. Once done, the final score is reported to the
  // runner.
  static runSuites(runner: Runner): void {
    let continuation: (() => void) | null | void = null;
    let suites: BenchmarkSuite[] = BenchmarkSuite.suites;
    let length: number = suites.length;
    BenchmarkSuite.scores = [];
    let index: number = 0;
    let runStep = (): void => {
      while (continuation || index < length) {
        if (continuation) {
          continuation = continuation();
        } else {
          const suite = suites[index++];
          continuation = suite.runStep(runner);
        }
        if (!continuation) {
          runStep();
          return;
        }
      }
    };
    runStep();
  }
  // Runs a single benchmark for at least a second and computes the
  // average time it takes to run a single iteration.
  runSingleBenchmark(benchmark: Benchmark, data: SuiteData | null): SuiteData | null {
    let measure = (data: SuiteData | null): void => {
      let elapsed: number = 0;
      let start: number = performanceNow();
      let i: number = 0;
      // Run either for 1 second or for the number of iterations specified
      // by minIterations, depending on the config flag doDeterministic.
      for (i = 0; benchmark.doDeterministic ? i < benchmark.minIterations : elapsed < TIMES_KILO_MILLISECONDS; i++) {
        benchmark.run();
        elapsed = performanceNow() - start;
      }

      if (data != null) {
        data.runs += i;
        data.elapsed += elapsed;
      }
    };
    // Sets up data in order to skip or not the warmup phase.
    if (!benchmark.doWarmup && data == null) {
      data = new SuiteData(0, 0);
    }
    if (data == null) {
      measure(null);
      return new SuiteData(0, 0);
    } else {
      measure(data);
      // If we've run too few iterations, we continue for another second.
      if (data.runs < benchmark.minIterations) {
        return data;
      }
      let uses = (data.elapsed * TIMES_KILO_MILLISECONDS) / data.runs;
      let latencySamples = benchmark.latencyResult != null ? benchmark.latencyResult() : [0];
      let percentile = 99.5;
      let latency = BenchmarkSuite.averageAbovePercentile(latencySamples, percentile) * TIMES_KILO_MILLISECONDS;
      this.notifyStep(new BenchmarkResult(benchmark, uses, latency));
      latency /= TIMES_KILO_MILLISECONDS;
      print('\nsplay-latency: ms = ' + `${latency.toFixed(TOFIXED_NUMBER2)}`);
      return null;
    }
  }

  // Computes the average of the worst samples. For example, if percentile is 99, this will report the
  // average of the worst 1% of the samples.
  static averageAbovePercentile(numbers: number[], percentile: number): number {
    const numbersCopy = [...numbers];
    numbersCopy.sort((a, b) => a - b);

    const numbersWeWant: number[] = [];
    const originalLength = numbersCopy.length;
    while (numbersCopy.length / originalLength > percentile / PERCENTAGE_RADIO) {
      numbersWeWant.push(numbersCopy.pop()!);
    }

    let sum = 0;
    for (const num of numbersWeWant) {
      sum += num;
    }

    const result = sum / numbersWeWant.length;

    // Do a sanity check.
    if (numbersCopy.length && result < numbersCopy[numbersCopy.length - 1]) {
      return result;
    }
    return result;
  }
  // Converts a score value to a string with at least three significant
  // digits.
  static formatScore(value: number): string {
    if (value > PERCENTAGE_RADIO) {
      return value.toFixed(TOFIXED_NUMBER0);
    } else {
      return value.toPrecision(TOFIXED_NUMBER3);
    }
  }
  // Notifies the runner that we're done running a single benchmark in
  // the benchmark suite. This can be useful to report progress.
  notifyStep(result: BenchmarkResult): void {
    this.results.push(result);
    if (this.runner && typeof this.runner.notifyStep === 'function') {
      this.runner.notifyStep(result.benchmark.name);
    }
  }

  // Notifies the runner that running a benchmark resulted in an error.
  notifyError(error: Error): void {
    if (this.runner) {
      this.runner.notifyError(this.name, error);
      if (this.runner.notifyStep) {
        this.runner.notifyStep(this.name);
      }
    }
  }
}

class SplayTree {
  // Pointer to the root node of the tree.
  root_: Nodes | null = null;
  // @return {boolean} Whether the tree is empty.
  isEmpty(): boolean {
    return !this.root_;
  }

  insert(key: number, value: object | null): void {
    if (this.isEmpty()) {
      this.root_ = new Nodes(key, value);
      return;
    }

    this.splay_(key);

    if (this.root_?.key === key) {
      return;
    }
    let node = new Nodes(key, value);

    if (key > this.root_!.key) {
      node.left = this.root_!;
      node.right = this.root_!.right;
      this.root_!.right = null;
    } else {
      node.right = this.root_!;
      node.left = this.root_!.left;
      this.root_!.left = null;
    }
    this.root_ = node;
  }

  remove(key: number): Nodes {
    this.splay_(key);
    const removed = this.root_;
    if (!this.root_!.left) {
      this.root_ = this.root_!.right;
    } else {
      const right = this.root_!.right;
      this.root_ = this.root_!.left;
      // Splay to make sure that the new root has an empty right child.
      this.splay_(key);
      // Insert the original right child as the right child of the new
      // root.
      this.root_.right = right;
    }
    return removed!;
  }
  /**
   * Returns the node having the specified key or null if the tree doesn't contain
   * a node with the specified key.
   *
   * @param {number} key Key to find in the tree.
   * @return {SplayTree.Node} Node having the specified key.
   */
  find(key: number): Nodes | null {
    if (this.isEmpty()) {
      return null;
    }
    this.splay_(key);
    return this.root_?.key === key ? this.root_ : null;
  }
  /**
   * @return {SplayTree.Node} Node having the maximum key value.
   */
  findMax(optStartNode: Nodes | null = null): Nodes | null {
    if (this.isEmpty()) {
      return null;
    }
    let current = optStartNode || this.root_;
    while (current!.right) {
      current = current!.right;
    }
    return current;
  }
  /**
   * @return {SplayTree.Node} Node having the maximum key value that
   *     is less than the specified key value.
   */
  findGreatestLessThan(key: number): Nodes | null {
    if (this.isEmpty()) {
      return null;
    }
    this.splay_(key);
    if (this.root_!.key < key) {
      return this.root_;
    } else if (this.root_!.left) {
      return this.findMax(this.root_!.left);
    } else {
      return null;
    }
  }
  /**
   * @return {Array<*>} An array containing all the keys of tree's nodes.
   */
  exportKeys(): number[] {
    const result: number[] = [];
    if (!this.isEmpty()) {
      this.root_!.traverse_(node => {
        result.push(node.key);
      });
    }
    return result;
  }

  splay_(key: number): void {
    if (this.isEmpty()) {
      return;
    }
    let dummy: Nodes = new Nodes(0, null);
    let left: Nodes = new Nodes(0, null);
    let right: Nodes = new Nodes(0, null);
    let current = this.root_;

    while (true) {
      if (key < current!.key) {
        if (!current!.left) {
          break;
        }
        if (key < current!.left.key) {
          // Rotate right.
          let tmp = current!.left;
          current!.left = tmp.right;
          tmp.right = current;
          current = tmp;
          // check left
          if (!current.left) {
            break;
          }
        }
        // switch right.
        right.left = current;
        right = current!;
        current = current!.left;
      } else if (key > current!.key) {
        if (!current!.right) {
          break;
        }
        if (key > current!.right.key) {
          // Rotate left.
          let tmp = current!.right;
          current!.right = tmp.left;
          tmp.left = current;
          current = tmp;
          // check is right
          if (!current.right) {
            break;
          }
        }
        // switch left
        left.right = current;
        left = current!;
        current = current!.right;
      } else {
        break;
      }
    }
    // Assemble.
    left.right = current!.left;
    right.left = current!.right;
    current!.left = dummy.right;
    current!.right = dummy.left;
    this.root_ = current;
  }
}

class Nodes {
  key: number;
  value: object | null;
  left: Nodes | null;
  right: Nodes | null;
  constructor(key: number, value: object | null) {
    this.key = key;
    this.value = value;
    this.right = null;
    this.left = null;
  }

  traverse_(f: (nodes: Nodes) => void): void {
    let current: Nodes | null = null;
	  if(this) {
	    let left = this.left;
	    if (left) {
		  left.traverse_(f);
	  }
	  if(this) {
      current = this.right;
    }
	}
    while (current) {
      let left = current.left;
      if (left) {
        left.traverse_(f);
      }
      f(current);
      current = current.right;
    }
  }
}
class PayloadTreeLastNode {
  array: number[];
  string: string;
  constructor(array: number[], string: string) {
    this.array = array;
    this.string = string;
  }
}
class PayloadTreeNode {
  left: object;
  right: object;
  constructor(left: object, right: object) {
    this.left = left;
    this.right = right;
  }
}
class SplayLatency {
  generateKey(): number {
    // The benchmark framework guarantees that Math.random is
    // deterministic; see base.js.
    return BenchmarkSuite.random();
  }
  splayLatency(): number[] {
    return splaySamples;
  }
  splayUpdateStats(time: number): void {
    const pause = time - splaySampleTimeStart;
    splaySampleTimeStart = time;
    splaySamples.push(pause);
  }
  generatePayloadTree(depth: number, tag: number | string): object {
    if (depth === 0) {
      return new PayloadTreeLastNode(
        [
          NODE_PAYLOAD_NUM_0,
          NODE_PAYLOAD_NUM_1,
          NODE_PAYLOAD_NUM_2,
          NODE_PAYLOAD_NUM_3,
          NODE_PAYLOAD_NUM_4,
          NODE_PAYLOAD_NUM_5,
          NODE_PAYLOAD_NUM_6,
          NODE_PAYLOAD_NUM_7,
          NODE_PAYLOAD_NUM_8,
          NODE_PAYLOAD_NUM_9
        ],
        `String for key ${tag} in leaf node`
      );
    } else {
      return new PayloadTreeNode(splayLatency.generatePayloadTree(depth - 1, tag), splayLatency.generatePayloadTree(depth - 1, tag));
    }
  }
  insertNewNode(): number {
    // Insert new node with a unique key.
    let key: number;
    do {
      key = splayLatency.generateKey();
    } while ((splayTree && splayTree.find(key)) !== null);
    const payload = splayLatency.generatePayloadTree(SPLAY_TREE_PAYLOAD_DEPTH, String(key));
    splayTree?.insert(key, payload);
    return key;
  }
  splayRun(): void {
    // Replace a few nodes in the splay tree.
    for (let i = 0; i < SPLAY_TREE_MODIFICATIONS; i++) {
      const key = splayLatency.insertNewNode();
      const greatest = splayTree?.findGreatestLessThan(key);
      if (greatest === null) {
        splayTree?.remove(key);
      } else {
        splayTree?.remove(greatest!.key);
      }
    }
    splayLatency.splayUpdateStats(performanceNow());
  }
  splaySetup(): void {
    splayTree = new SplayTree();
    splaySampleTimeStart = performanceNow();
    for (let i = 0; i < SPLAY_TREE_SIZE; i++) {
      splayLatency.insertNewNode();
      if ((i + 1) % INSERT_NODE_VALUE_REC === INSERT_NODE_VALUE_REC - 1) {
        splayLatency.splayUpdateStats(performanceNow());
      }
    }
  }
  splayTearDown(): void {
    // Allow the garbage collector to reclaim the memory
    // used by the splay tree no matter how we exit the
    // tear down function.
    const keys = splayTree?.exportKeys();
    if (keys == null) {
      return;
    }
    splayTree = null;
    splaySamples = [];
    // Verify that the splay tree has the right size.
    const length = keys.length;
    if (length !== SPLAY_TREE_SIZE) {
      return;
    }
    // Verify that the splay tree has sorted, unique keys.
    for (let i = 0; i < length - 1; i++) {
      if (keys[i] >= keys[i + 1]) {
        return;
      }
    }
  }
  notifyError(name: string, error: Error): void {}
  notifyStep(name: string): void {}
  printResult(name: string, result: Error): void {}
  printScore(score: number): void {}
}

let splayLatency = new SplayLatency();

let splayTree: SplayTree | null = null;

let benchmarkSuite = new BenchmarkSuite(
  'Splay',
  [REFERENCE_SCORE_MIN, REFERENCE_SCORE_MAX],
  [
    new Benchmark(
      'Splay',
      true,
      false,
      splayLatency.splayLatency,
      splayLatency.splayRun,
      splayLatency.splaySetup,
      splayLatency.splayTearDown,
      DEFAULT_MIN_ITERATIONS
    )
  ]
);
function start(): void {
  BenchmarkSuite.runSuites({ notifyError: splayLatency.notifyError });
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  start();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

start();
