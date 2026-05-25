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

const ARRAY_INDEX2: number = 2;
const ARRAY_INDEX3: number = 3;
const ARRAY_INDEX4: number = 4;
const ARRAY_INDEX5: number = 5;
const ARRAY_INDEX6: number = 6;
const ARRAY_INDEX7: number = 7;
const ARRAY_INDEX8: number = 8;
const ARRAY_INDEX9: number = 9;
const ARRAY_INDEX10: number = 10;
const ARRAY_INDEX11: number = 11;
const ARRAY_INDEX12: number = 12;
const ARRAY_INDEX13: number = 13;

const ARRAY_LENGTH16: number = 16;
const ARRAY_LENGTH4: number = 4;

const ITERATION_COUNT3: number = 3;
const ITERATION_COUNT4: number = 4;
const ITERATION_COUNT11: number = 11;

const NORMAL_NUMBER2: number = 2;
const NORMAL_NUMBER3: number = 3;
const NORMAL_NUMBER4: number = 4;
const NORMAL_NUMBER10: number = 10;
const NORMAL_NUMBER17: number = 17;
const NORMAL_NUMBER32: number = 32;
const NORMAL_NUMBER40: number = 40;
const NORMAL_NUMBER1000: number = 1000;
const NORMAL_NUMBER30: number = 30;
const NORMAL_NUMBER20: number = 20;
const NORMAL_NUMBER38: number = 38;
const NORMAL_NUMBER22: number = 22;
const NORMAL_NUMBER23: number = 23;

const NORMAL_FLOAT_0P1: number = 0.1;
const NORMAL_FLOAT_0P4: number = 0.4;
const NORMAL_FLOAT_0P3: number = 0.3;
const NORMAL_FLOAT_0P7: number = 0.7;

const VECTOR_INITVAL_P7: number = 0.7;
const VECTOR_INITVAL_P8: number = 0.8;

const COLOUR_REFLECTION_DOWNLINE: number = 0.001;
const COLOUR_INSECTION_SMALLVAL: number = 0.0001;
const COLOUR_INSECTION_BIGVAL: number = 1000000;
const COLOUR_REFLECTION_DOWNLINE999: number = 0.999999;
const DISTANCE_SMALL_VAL: number = 0.0001;
const ITERATION_RUNCOUNT: number = 80;

/// 矢量类 存储x,y,z三个浮点数，用来表示一个位置，一个像素点
class Vector {
  values: Float64Array;
  reflection?: number;
  colour?: Vector;
  static zero(): Vector {
    return new Vector(0, 0, 0);
  }

  constructor(x: number, y: number, z: number) {
    this.values = new Float64Array([x, y, z]);
  }

  // log打印使用
  get description(): string {
    return `[${this.scriptString}]`;
  }

  // 绘制脚本拼接使用
  get scriptString(): string {
    return `${this.values[0]},${this.values[1]},${this.values[ARRAY_INDEX2]}`;
  }
}

function createVector(x: number, y: number, z: number): Vector {
  return new Vector(x, y, z);
}

function sqrLengthVector(vector: Vector): number {
  return vector.values[0] * vector.values[0] + vector.values[1] * vector.values[1] + vector.values[ARRAY_INDEX2] * vector.values[ARRAY_INDEX2];
}

function lengthVector(vector: Vector): number {
  return Math.sqrt(vector.values[0] * vector.values[0] + vector.values[1] * vector.values[1] + vector.values[ARRAY_INDEX2] * vector.values[ARRAY_INDEX2]);
}

function addVector(vector: Vector, v: Vector): Vector {
  vector.values[0] += v.values[0];
  vector.values[1] += v.values[1];
  vector.values[ARRAY_INDEX2] += v.values[ARRAY_INDEX2];
  return vector;
}

function subVector(vector: Vector, v: Vector): Vector {
  vector.values[0] -= v.values[0];
  vector.values[1] -= v.values[1];
  vector.values[ARRAY_INDEX2] -= v.values[ARRAY_INDEX2];
  return vector;
}

function scaleVector(vector: Vector, scale: number): Vector {
  vector.values[0] *= scale;
  vector.values[1] *= scale;
  vector.values[ARRAY_INDEX2] *= scale;
  return vector;
}

function normaliseVector(vector: Vector): Vector {
  let len = Math.sqrt(vector.values[0] * vector.values[0] + vector.values[1] * vector.values[1] + vector.values[ARRAY_INDEX2] * vector.values[ARRAY_INDEX2]);
  vector.values[0] /= len;
  vector.values[1] /= len;
  vector.values[ARRAY_INDEX2] /= len;
  return vector;
}

function add(v1: Vector, v2: Vector): Vector {
  return new Vector(v1.values[0] + v2.values[0], v1.values[1] + v2.values[1], v1.values[ARRAY_INDEX2] + v2.values[ARRAY_INDEX2]);
}

function sub(v1: Vector, v2: Vector): Vector {
  return new Vector(v1.values[0] - v2.values[0], v1.values[1] - v2.values[1], v1.values[ARRAY_INDEX2] - v2.values[ARRAY_INDEX2]);
}

function scalev(v1: Vector, v2: Vector): Vector {
  return new Vector(v1.values[0] * v2.values[0], v1.values[1] * v2.values[1], v1.values[ARRAY_INDEX2] * v2.values[ARRAY_INDEX2]);
}

function dot(v1: Vector, v2: Vector): number {
  return v1.values[0] * v2.values[0] + v1.values[1] * v2.values[1] + v1.values[ARRAY_INDEX2] * v2.values[ARRAY_INDEX2];
}

function scale(v: Vector, scale: number): Vector {
  return new Vector(v.values[0] * scale, v.values[1] * scale, v.values[ARRAY_INDEX2] * scale);
}

function cross(v1: Vector, v2: Vector): Vector {
  return new Vector(
    v1.values[1] * v2.values[ARRAY_INDEX2] - v1.values[ARRAY_INDEX2] * v2.values[1],
    v1.values[ARRAY_INDEX2] * v2.values[0] - v1.values[0] * v2.values[ARRAY_INDEX2],
    v1.values[0] * v2.values[1] - v1.values[1] * v2.values[0]
  );
}

function normalise(v: Vector): Vector {
  let len = lengthVector(v);
  return new Vector(v.values[0] / len, v.values[1] / len, v.values[ARRAY_INDEX2] / len);
}

function transformMatrix(matrix: Float64Array, v: Vector): Vector {
  let vals = matrix;
  let x = vals[0] * v.values[0] + vals[1] * v.values[1] + vals[ARRAY_INDEX2] * v.values[ARRAY_INDEX2] + vals[ARRAY_INDEX3];
  let y = vals[ARRAY_INDEX4] * v.values[0] + vals[ARRAY_INDEX5] * v.values[1] + vals[ARRAY_INDEX6] * v.values[ARRAY_INDEX2] + vals[ARRAY_INDEX7];
  let z = vals[ARRAY_INDEX8] * v.values[0] + vals[ARRAY_INDEX9] * v.values[1] + vals[ARRAY_INDEX10] * v.values[ARRAY_INDEX2] + vals[ARRAY_INDEX11];
  return new Vector(x, y, z);
}

function invertMatrix(matrix: Float64Array): Float64Array {
  let temp = new Float64Array(ARRAY_LENGTH16);
  let tx = -matrix[ARRAY_INDEX3];
  let ty = -matrix[ARRAY_INDEX7];
  let tz = -matrix[ARRAY_INDEX11];
  for (let h = 0; h < ITERATION_COUNT3; h++) {
    for (let v = 0; v < ITERATION_COUNT3; v++) {
      temp[h + v * NORMAL_NUMBER4] = matrix[v + h * NORMAL_NUMBER4];
    }
  }
  for (let i = 0; i < ITERATION_COUNT11; i++) {
    matrix[i] = temp[i];
  }
  matrix[ARRAY_INDEX3] = tx * matrix[0] + ty * matrix[1] + tz * matrix[ARRAY_INDEX2];
  matrix[ARRAY_INDEX7] = tx * matrix[ARRAY_INDEX4] + ty * matrix[ARRAY_INDEX5] + tz * matrix[ARRAY_INDEX6];
  matrix[ARRAY_INDEX11] = tx * matrix[ARRAY_INDEX8] + ty * matrix[ARRAY_INDEX9] + tz * matrix[ARRAY_INDEX10];
  return matrix;
}

// 三角形
class Triangle {
  axis: number;
  normal: Vector;
  nu: number;
  nv: number;
  nd: number;
  eu: number;
  ev: number;
  nu1: number;
  nv1: number;
  nu2: number;
  nv2: number;
  material: Vector;
  shader?: (triangle: Triangle, v1: Vector, v2: Vector) => Vector;

  constructor(p1: Vector, p2: Vector, p3: Vector) {
    let edge1 = sub(p3, p1);
    let edge2 = sub(p2, p1);
    let normal = cross(edge1, edge2);
    if (Math.abs(normal.values[0]) > Math.abs(normal.values[1])) {
      if (Math.abs(normal.values[0]) > Math.abs(normal.values[ARRAY_INDEX2])) {
        this.axis = 0;
      } else {
        this.axis = 2;
      }
    } else if (Math.abs(normal.values[1]) > Math.abs(normal.values[ARRAY_INDEX2])) {
      this.axis = 1;
    } else {
      this.axis = 2;
    }
    let u = (this.axis + 1) % NORMAL_NUMBER3;
    let v = (this.axis + NORMAL_NUMBER2) % NORMAL_NUMBER3;
    let u1 = edge1.values[u];
    let v1 = edge1.values[v];
    let u2 = edge2.values[u];
    let v2 = edge2.values[v];
    this.normal = normalise(normal);
    this.nu = normal.values[u] / normal.values[this.axis];
    this.nv = normal.values[v] / normal.values[this.axis];
    this.nd = dot(normal, p1) / normal.values[this.axis];
    let det = u1 * v2 - v1 * u2;
    this.eu = p1.values[u];
    this.ev = p1.values[v];
    this.nu1 = u1 / det;
    this.nv1 = -v1 / det;
    this.nu2 = v2 / det;
    this.nv2 = -u2 / det;
    this.material = new Vector(VECTOR_INITVAL_P7, VECTOR_INITVAL_P7, VECTOR_INITVAL_P7);
  }

  intersect(orig: Vector, dir: Vector, near?: number, far?: number): number | null {
    let u = (this.axis + 1) % NORMAL_NUMBER3;
    let v = (this.axis + NORMAL_NUMBER2) % NORMAL_NUMBER3;
    let d = dir.values[this.axis] + this.nu * dir.values[u] + this.nv * dir.values[v];
    let t = (this.nd - orig.values[this.axis] - this.nu * orig.values[u] - this.nv * orig.values[v]) / d;
    if (near != null && far != null) {
      if (t < near || t > far) {
        return null;
      }
    }
    let pu = orig.values[u] + t * dir.values[u] - this.eu;
    let pv = orig.values[v] + t * dir.values[v] - this.ev;
    let a2 = pv * this.nu1 + pu * this.nv1;
    if (a2 < 0) {
      return null;
    }
    let a3 = pu * this.nu2 + pv * this.nv2;
    if (a3 < 0) {
      return null;
    }
    if (a2 + a3 > 1) {
      return null;
    }
    return t;
  }
}

// 场景
class Scene {
  triangles: Triangle[];
  lights: Vector[];
  ambient: Vector;
  background: Vector;

  constructor(aTriangles: Triangle[]) {
    this.triangles = aTriangles;
    this.lights = [Vector.zero(), Vector.zero(), Vector.zero()];
    this.ambient = Vector.zero();
    this.background = new Vector(VECTOR_INITVAL_P8, VECTOR_INITVAL_P8, 1);
  }

  lightBlocked(hit: Vector, normal: Vector, l: Vector): void {
    for (let i = 0; i < this.lights.length; i++) {
      let light = this.lights[i];
      let toLight = sub(light, hit);
      let distance = lengthVector(toLight);
      scaleVector(toLight, 1.0 / distance);
      distance -= DISTANCE_SMALL_VAL;
      if (this.blocked(hit, toLight, distance)) {
        continue;
      }
      let nl = dot(normal, toLight);
      if (nl > 0) {
        addVector(l, scale(light.colour!, nl));
      }
    }
  }

  intersect(origin: Vector, dir: Vector, near?: number, far?: number): Vector {
    let closest: Triangle | null = null;
    let rfar = far;
    for (let i = 0; i < this.triangles.length; i++) {
      let triangle = this.triangles[i];
      let d = triangle.intersect(origin, dir, near, far);
      if (d == null) {
        continue;
      }
      rfar = d;
      closest = triangle;
    }

    if (closest == null || rfar == null) {
      return new Vector(this.background.values[0], this.background.values[1], this.background.values[ARRAY_INDEX2]);
    }
    let normal = closest.normal;
    let hit = add(origin, scale(dir, rfar));
    if (dot(dir, normal) > 0) {
      normal = new Vector(-normal.values[0], -normal.values[1], -normal.values[ARRAY_INDEX2]);
    }
    let colour: Vector | null = null;
    if (closest.shader) {
      colour = closest.shader(closest, hit, dir);
    } else {
      colour = closest.material;
    }

    // do reflection
    let reflected: Vector | null = null;
    if (colour.reflection) {
      if (colour.reflection > COLOUR_REFLECTION_DOWNLINE) {
        let reflection = addVector(scale(normal, -NORMAL_NUMBER2 * dot(dir, normal)), dir);
        reflected = this.intersect(hit, reflection, COLOUR_INSECTION_SMALLVAL, COLOUR_INSECTION_BIGVAL);
        if (colour.reflection >= COLOUR_REFLECTION_DOWNLINE999) {
          return reflected!;
        }
      }
    }

    let l = new Vector(this.ambient.values[0], this.ambient.values[1], this.ambient.values[ARRAY_INDEX2]);
    this.lightBlocked(hit, normal, l);
    
    l = scalev(l, colour);
    if (reflected) {
      l = addVector(scaleVector(l, 1 - colour.reflection!), scaleVector(reflected, colour.reflection!));
    }
    return l;
  }

  blocked(o: Vector, d: Vector, far: number): boolean {
    let near = 0.0001;
    for (let i = 0; i < this.triangles.length; i++) {
      let triangle = this.triangles[i];
      let d1 = triangle.intersect(o, d, near, far);
      if (d1 == null || d1 > far || d1 < near) {
        continue;
      }
      return true;
    }
    return false;
  }
}

// 光线
class Ray {
  origin: Vector;

  dir: Vector;

  constructor(origin: Vector, dir: Vector) {
    this.origin = origin;
    this.dir = dir;
  }

  static zero(): Ray {
    return new Ray(Vector.zero(), Vector.zero());
  }
}

// 相机
class Camera {
  directions: Vector[];
  origin: Vector;
  rays: Ray[] | null;

  constructor(origin: Vector, lookat: Vector, up: Vector) {
    let zaxis = normaliseVector(subVector(lookat, origin));
    let xaxis = normaliseVector(cross(up, zaxis));
    let yaxis = normaliseVector(cross(xaxis, subVector(Vector.zero(), zaxis)));
    let m = new Float64Array(ARRAY_LENGTH16);
    m[0] = xaxis.values[0];
    m[1] = xaxis.values[1];
    m[ARRAY_INDEX2] = xaxis.values[ARRAY_INDEX2];
    m[ARRAY_INDEX4] = yaxis.values[0];
    m[ARRAY_INDEX5] = yaxis.values[1];
    m[ARRAY_INDEX6] = yaxis.values[ARRAY_INDEX2];
    m[ARRAY_INDEX8] = zaxis.values[0];
    m[ARRAY_INDEX9] = zaxis.values[1];
    m[ARRAY_INDEX10] = zaxis.values[ARRAY_INDEX2];
    invertMatrix(m);
    m[ARRAY_INDEX3] = 0;
    m[ARRAY_INDEX7] = 0;
    m[ARRAY_INDEX11] = 0;
    this.origin = origin;
    this.directions = new Array<Vector>(ARRAY_LENGTH4);

    this.directions[0] = normalise(new Vector(-VECTOR_INITVAL_P7, VECTOR_INITVAL_P7, 1));
    this.directions[1] = normalise(new Vector(VECTOR_INITVAL_P7, VECTOR_INITVAL_P7, 1));
    this.directions[ARRAY_INDEX2] = normalise(new Vector(VECTOR_INITVAL_P7, -VECTOR_INITVAL_P7, 1));
    this.directions[ARRAY_INDEX3] = normalise(new Vector(-VECTOR_INITVAL_P7, -VECTOR_INITVAL_P7, 1));
    this.directions[0] = transformMatrix(m, this.directions[0]);
    this.directions[1] = transformMatrix(m, this.directions[1]);
    this.directions[ARRAY_INDEX2] = transformMatrix(m, this.directions[ARRAY_INDEX2]);
    this.directions[ARRAY_INDEX3] = transformMatrix(m, this.directions[ARRAY_INDEX3]);
    this.rays = [];
  }

  /// 生成光线
  generateRayPair(y: number): Ray[] {
    let dir1 = addVector(scale(this.directions[0], y), scale(this.directions[ARRAY_INDEX3], 1 - y));
    let ray1 = new Ray(this.origin, dir1);
    let dir2 = addVector(scale(this.directions[1], y), scale(this.directions[ARRAY_INDEX2], 1 - y));
    let ray2 = new Ray(this.origin, dir2);
    this.rays = [ray1, ray2];
    return this.rays;
  }

  render(scene: Scene, pixels: Vector[][], width: number, height: number): void {
    renderRows(this, scene, pixels, width, height, 0, height);
  }
}

function renderRows(camera: Camera, scene: Scene, pixels: Vector[][], width: number, height: number, starty: number, stopy: number): void {
  let pointNum = 1;
  for (let y = starty; y < stopy; y++) {
    let rays = camera.generateRayPair(y / height);
    for (let x = 0; x < width; x++) {
      let xp = x / width;
      let origin = addVector(scale(rays[0].origin, xp), scale(rays[1].origin, 1 - xp));
      let dir = normaliseVector(addVector(scale(rays[0].dir, xp), scale(rays[1].dir, 1 - xp)));
      let l = scene.intersect(origin, dir);
      pixels[y][x] = l;
      if (DEBUG) {
        print(`point ${pointNum}: [${l.description}]`);
        pointNum += 1;
      }
    }
  }
}

/// 构建场景
function raytraceScene(): void {
  /// 创建矢量，通过矢量构建三角形
  let tfl = createVector(-NORMAL_NUMBER10, NORMAL_NUMBER10, -NORMAL_NUMBER10);
  let tfr = createVector(NORMAL_NUMBER10, NORMAL_NUMBER10, -NORMAL_NUMBER10);
  let tbl = createVector(-NORMAL_NUMBER10, NORMAL_NUMBER10, NORMAL_NUMBER10);
  let tbr = createVector(NORMAL_NUMBER10, NORMAL_NUMBER10, NORMAL_NUMBER10);
  let bfl = createVector(-NORMAL_NUMBER10, -NORMAL_NUMBER10, -NORMAL_NUMBER10);
  let bfr = createVector(NORMAL_NUMBER10, -NORMAL_NUMBER10, -NORMAL_NUMBER10);
  let bbl = createVector(-NORMAL_NUMBER10, -NORMAL_NUMBER10, NORMAL_NUMBER10);
  let bbr = createVector(NORMAL_NUMBER10, -NORMAL_NUMBER10, NORMAL_NUMBER10);
  let triangles = [
    new Triangle(tfl, tfr, bfr),
    new Triangle(tfl, bfr, bfl),
    new Triangle(tbl, tbr, bbr),
    new Triangle(tbl, bbr, bbl),
    new Triangle(tbl, tfl, bbl),
    new Triangle(tfl, bfl, bbl),
    new Triangle(tbr, tfr, bbr),
    new Triangle(tfr, bfr, bbr),
    new Triangle(tbl, tbr, tfr),
    new Triangle(tbl, tfr, tfl),
    new Triangle(bbl, bbr, bfr),
    new Triangle(bbl, bfr, bfl)
  ];
  let floorShader = (tri: Triangle, pos: Vector, view: Vector): Vector => {
    let green = createVector(0.0, NORMAL_FLOAT_0P4, 0.0);
    let grey = createVector(NORMAL_FLOAT_0P4, NORMAL_FLOAT_0P4, NORMAL_FLOAT_0P4);
    grey.reflection = 1.0;
    let x = (((pos.values[0] / NORMAL_NUMBER32) % NORMAL_NUMBER2) + NORMAL_NUMBER2) % NORMAL_NUMBER2;
    let z = (((pos.values[ARRAY_INDEX2] / NORMAL_NUMBER32 + NORMAL_FLOAT_0P3) % NORMAL_NUMBER2) + NORMAL_NUMBER2) % NORMAL_NUMBER2;
    if (x < 1 !== z < 1) {
      return grey;
    } else {
      return green;
    }
  };
  let ffl = createVector(-NORMAL_NUMBER1000, -NORMAL_NUMBER30, -NORMAL_NUMBER1000);
  let ffr = createVector(NORMAL_NUMBER1000, -NORMAL_NUMBER30, -NORMAL_NUMBER1000);
  let fbl = createVector(-NORMAL_NUMBER1000, -NORMAL_NUMBER30, NORMAL_NUMBER1000);
  let fbr = createVector(NORMAL_NUMBER1000, -NORMAL_NUMBER30, NORMAL_NUMBER1000);
  let tr13 = new Triangle(fbl, fbr, ffr);
  tr13.shader = floorShader;
  triangles.push(tr13);
  let tr14 = new Triangle(fbl, ffr, ffl);
  tr14.shader = floorShader;
  triangles.push(tr14);
  /// 通过三角形数组构建场景
  let _scene = new Scene(triangles);
  /// 设置场景灯光
  _scene.lights[0] = createVector(NORMAL_NUMBER20, NORMAL_NUMBER38, -NORMAL_NUMBER22);
  _scene.lights[0].colour = createVector(NORMAL_FLOAT_0P7, NORMAL_FLOAT_0P3, NORMAL_FLOAT_0P3);
  _scene.lights[1] = createVector(-NORMAL_NUMBER23, NORMAL_NUMBER40, NORMAL_NUMBER17);
  _scene.lights[1].colour = createVector(NORMAL_FLOAT_0P7, NORMAL_FLOAT_0P3, NORMAL_FLOAT_0P3);
  _scene.lights[ARRAY_INDEX2] = createVector(NORMAL_NUMBER23, NORMAL_NUMBER20, NORMAL_NUMBER17);
  _scene.lights[ARRAY_INDEX2].colour = createVector(NORMAL_FLOAT_0P7, NORMAL_FLOAT_0P7, NORMAL_FLOAT_0P7);
  /// 设置背景
  _scene.ambient = createVector(NORMAL_FLOAT_0P1, NORMAL_FLOAT_0P1, NORMAL_FLOAT_0P1);
  let size = 30;
  /// 初始化像素数组
  let pixels: Vector[][] = new Array(size);
  for (let y = 0; y < size; y++) {
    pixels[y] = new Array(size);
    for (let x = 0; x < size; x++) {
      pixels[y][x] = Vector.zero();
    }
  }
  /// 初始化相机
  let _camera = new Camera(createVector(-NORMAL_NUMBER40, NORMAL_NUMBER40, NORMAL_NUMBER40), createVector(0, 0, 0), createVector(0, 1, 0));
  /// 执行3d-raytrace算法，生成像素数组
  _camera.render(_scene, pixels, size, size);
  return pixels;
}

function arrayToCanvasCommands(pixels: Vector[][]): void {
  let s = '<canvas id="renderCanvas" width="30px" height="30px"></canvas><scr' + 'ipt>\nvar pixels = [';
  let size = 30;
  for (let y = 0; y < size; y++) {
    s += '[';
    for (let x = 0; x < size; x++) {
      s += '[' + pixels[y][x].scriptString + '],';
    }
    s += '],';
  }
  s +=
    '];\n    var canva = document.getElementById("renderCanvas").getContext("2d");\n\
\n\
\n\
    var size = 30;\n\
    canva.fillStyle = "red";\n\
    canva.fillRect(0, 0, size, size);\n\
    canva.scale(1, -1);\n\
    canva.translate(0, -size);\n\
\n\
    if (!canva.setFillColor)\n\
    canva.setFillColor = function(r, g, b, a) {\n\
            this.fillStyle = "rgb("+[Math.floor(r * 255), Math.floor(g * 255), Math.floor(b * 255)]+")";\n}\n\
\n\
for (var y = 0; y < size; y++) {\n\
  for (var x = 0; x < size; x++) {\n\
    var l = pixels[y][x];\n\
    canva.setFillColor(l[0], l[1], l[2], 1);\n\
    canva.fillRect(x, y, 1, 1);\n\
  }\n\
}</scr' + 'ipt>';
  return s;
}

const DEBUG = false;

const showRenderJs = true;

/*
 * @State
 * @Tags Jetstream2
 */
class RayTraceBenchMark {
  runCount = ITERATION_RUNCOUNT;

  /*
   * @Benchmark
   */
  run(): void {
    let startTime = ArkTools.timeInUs() / NORMAL_NUMBER1000;
    for (let i = 0; i < this.runCount; i++) {
      const result = arrayToCanvasCommands(raytraceScene());
      if (DEBUG && showRenderJs) {
        // 字符串过长，并不能打印完整
        print(result);
      }
    }
    
    let endTime = ArkTools.timeInUs() / NORMAL_NUMBER1000;
    print(`3d-raytrace: ms = ${endTime - startTime}`);
  }
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  new RayTraceBenchMark().run();
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

new RayTraceBenchMark().run();
