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
// import { BenchmarkRunner } from "../../../utils/benchmarkTsSuite";
// declare function print(arg:string) : string;

declare interface ArkTools{
  timeInUs(arg:any):number
}

function createVector(x: number, y: number, z: number): number[] {
  return [x, y, z]
}

function lengthVector(self: number[]): number {
  return Math.sqrt(self[0] * self[0] + self[1] * self[1] + self[2] * self[2]);
}

function addVector(self: number[], v: number[]): number[] {
  self[0] += v[0];
  self[1] += v[1];
  self[2] += v[2];
  return self;
}

function subVector(self: number[], v: number[]): number[] {
  self[0] -= v[0];
  self[1] -= v[1];
  self[2] -= v[2];
  return self;
}

function scaleVector(self: number[], scale: number): number[] {
  self[0] *= scale;
  self[1] *= scale;
  self[2] *= scale;
  return self;
}

function normaliseVector(self: number[]): number[] {
  let len = Math.sqrt(self[0] * self[0] + self[1] * self[1] + self[2] * self[2]);
  self[0] /= len;
  self[1] /= len;
  self[2] /= len;
  return self;
}

function add(v1: number[], v2: number[]): number[] {
  return [v1[0] + v2[0], v1[1] + v2[1], v1[2] + v2[2]];
}

function sub(v1: number[], v2: number[]): number[] {
  return [v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2]];
}

function scalev(v1: number[], v2: number[]): number[] {
  return [v1[0] * v2[0], v1[1] * v2[1], v1[2] * v2[2]];
}

function dot(v1: number[], v2: number[]): number {
  return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

function scale(v: number[], scale: number): number[] {
  return [v[0] * scale, v[1] * scale, v[2] * scale];
}

function cross(v1: number[], v2: number[]): number[] {
  return [v1[1] * v2[2] - v1[2] * v2[1],
    v1[2] * v2[0] - v1[0] * v2[2],
    v1[0] * v2[1] - v1[1] * v2[0]];
}

function normalise(v: number[]): number[] {
  let len = lengthVector(v);
  return [v[0] / len, v[1] / len, v[2] / len];
}

function transformMatrix(self:number[], v: number[]): number[] {
  let vals = self;
  let x  = vals[0] * v[0] + vals[1] * v[1] + vals[2] * v[2] + vals[3];
  let y  = vals[4] * v[0] + vals[5] * v[1] + vals[6] * v[2] + vals[7];
  let z  = vals[8] * v[0] + vals[9] * v[1] + vals[10] * v[2] + vals[11];
  return [x, y, z];
}

function invertMatrix(self: number[]): number[] {
  let temp:number[] = new Array(16);
  let tx = -self[3];
  let ty = -self[7];
  let tz = -self[11];
  for (let h = 0; h < 3; h++)
    for (let v = 0; v < 3; v++)
      temp[h + v * 4] = self[v + h * 4];
  for (let i = 0; i < 11; i++)
    self[i] = temp[i];
  self[3] = tx * self[0] + ty * self[1] + tz * self[2];
  self[7] = tx * self[4] + ty * self[5] + tz * self[6];
  self[11] = tx * self[8] + ty * self[9] + tz * self[10];
  return self;
}

class TriangleClass {
  axis: number = 0;
  normal: number[] = new Array();
  nu: number = 0;
  nv: number = 0;
  nd: number = 0;
  eu: number = 0;
  ev: number = 0;
  nu1: number = 0;
  nv1: number = 0;
  nu2: number = 0;
  nv2: number = 0;
  material: number[] = new Array();
  shader: Function = (pos: number[]): number[] => {
    let x = ((pos[0] / 32) % 2 + 2) % 2;
    let z = ((pos[2] / 32 + 0.3) % 2 + 2) % 2;
    if (x < 1 != z < 1) {
      return createVector(0.4, 0.4, 0.4);
    }
    else
      return createVector(0.0, 0.4, 0.0);
    ;
  }
  isEmpty: boolean = false;
  hasShader: boolean = false;
  constructor(p1: number[], p2: number[], p3: number[], isEmpty = false) {
    this.isEmpty = isEmpty;
    let edge1 = sub(p3, p1);
    let edge2 = sub(p2, p1);
    let normal = cross(edge1, edge2);
    if (Math.abs(normal[0]) > Math.abs(normal[1])) {
      if (Math.abs(normal[0]) > Math.abs(normal[2])) {
        this.axis = 0;
      } else {
        this.axis = 2;
      }
    } else {
      if (Math.abs(normal[1]) > Math.abs(normal[2])) {
        this.axis = 1;
      } else {
        this.axis = 2;
      }
    }
    let u = (this.axis + 1) % 3;
    let v = (this.axis + 2) % 3;
    let u1 = edge1[u];
    let v1 = edge1[v];

    let u2 = edge2[u];
    let v2 = edge2[v];
    this.normal = normalise(normal);
    this.nu = normal[u] / normal[this.axis];
    this.nv = normal[v] / normal[this.axis];
    this.nd = dot(normal, p1) / normal[this.axis];
    let det = u1 * v2 - v1 * u2;
    this.eu = p1[u];
    this.ev = p1[v];
    this.nu1 = u1 / det;
    this.nv1 = -v1 / det;
    this.nu2 = v2 / det;
    this.nv2 = -u2 / det;
    this.material = [0.7, 0.7, 0.7];
  }
  intersect(orig: number[], dir: number[], near: number, far: number): Object {
    let u = (this.axis + 1) % 3;
    let v = (this.axis + 2) % 3;
    let d = dir[this.axis] + this.nu * dir[u] + this.nv * dir[v];
    let t = (this.nd - orig[this.axis] - this.nu * orig[u] - this.nv * orig[v]) / d;
    if (t < near || t > far) {
      return null;
    }
    let Pu = orig[u] + t * dir[u] - this.eu;
    let Pv = orig[v] + t * dir[v] - this.ev;
    let a2 = Pv * this.nu1 + Pu * this.nv1;
    if (a2 < 0) {
      return null;
    }
    let a3 = Pu * this.nu2 + Pv * this.nv2;
    if (a3 < 0) {
      return null;
    }
    if ((a2 + a3) > 1) {
      return null;
    }
    return t;
  }
}

class LightsClass{
  arr: number[] = new Array();
  colour: number[] = new Array();
  constructor(){}
}

let closest: TriangleClass = new TriangleClass([0, 0, 0], [0, 0, 0], [0, 0, 0], true);
class SceneClass {
  triangles: TriangleClass[];
  lights: LightsClass[] = new Array(3);
  ambient: number[] = [0,0,0];
  background: number[] = [0.8,0.8,1];
  constructor(a_triangles: TriangleClass[]) {
    this.triangles = a_triangles;
  }
  intersect(origin: number[], dir: number[], near: number, far: number): number[] {
    for (let i = 0; i < this.triangles.length; i++) {
      let triangle = this.triangles[i];
      let d = triangle.intersect(origin, dir, near, far);
      if (d === null || d > far || d < near)
        continue;
      far =Number(d);
      closest = triangle;
    }

    if (closest.isEmpty) {
      return [this.background[0], this.background[1], this.background[2]];
    }

    let normal = closest.normal;
    let hit = add(origin, scale(dir, far));
    if (dot(dir, normal) > 0) {
      normal = [-normal[0], -normal[1], -normal[2]];
    }
    let colour: number[] = [];
    if (closest.hasShader) {
      colour = closest.shader(hit);
    } else {
      colour = closest.material;
    }

    let l = [this.ambient[0], this.ambient[1], this.ambient[2]];
    for (let i = 0; i < this.lights.length; i++) {
      let light = this.lights[i];
      let toLight = sub(light.arr, hit);
      let distance = lengthVector(toLight);
      scaleVector(toLight, 1.0/distance);
      distance -= 0.0001;
      if (this.blocked(hit, toLight, distance)) {
        continue;
      }
      let nl = dot(normal, toLight);
      if (nl > 0) {
        addVector(l, scale(light.colour, nl));
      }
    }
    l = scalev(l, colour);
    return l;
  }

  blocked(O: number[], D: number[], far: number): boolean {
    let near = 0.0001;
    for (let i = 0; i < this.triangles.length; i++) {
      let triangle = this.triangles[i];
      let d = triangle.intersect(O, D, near, far);
      if (d === null || d > far || d < near) {
        continue;
      }
      return true;
    }
    return false;
  }
}

class RayClass{
  origin: number[] = [];
  dir: number[] = [];
  constructor(){}
}

class CameraClass {
  origin: number[] = [];
  directions: number[][] = new Array(4);
  constructor(origin: number[], lookat: number[], up: number[]) {
    let zaxis = normaliseVector(subVector(lookat, origin));
    let xaxis = normaliseVector(cross(up, zaxis));
    let yaxis = normaliseVector(cross(xaxis, subVector([0,0,0], zaxis)));
    let m:number[] = new Array(16);
    m[0] = xaxis[0]; m[1] = xaxis[1]; m[2] = xaxis[2];
    m[4] = yaxis[0]; m[5] = yaxis[1]; m[6] = yaxis[2];
    m[8] = zaxis[0]; m[9] = zaxis[1]; m[10] = zaxis[2];
    invertMatrix(m);
    m[3] = 0; m[7] = 0; m[11] = 0;
    this.origin = origin;
    this.directions[0] = normalise([-0.7,  0.7, 1]);
    this.directions[1] = normalise([ 0.7,  0.7, 1]);
    this.directions[2] = normalise([ 0.7, -0.7, 1]);
    this.directions[3] = normalise([-0.7, -0.7, 1]);
    this.directions[0] = transformMatrix(m, this.directions[0]);
    this.directions[1] = transformMatrix(m, this.directions[1]);
    this.directions[2] = transformMatrix(m, this.directions[2]);
    this.directions[3] = transformMatrix(m, this.directions[3]);
  }

  generateRayPair(y: number): RayClass[] {
    let rays: RayClass[] = new Array(2);
    rays[0] = new RayClass();
    rays[1] = new RayClass();
    rays[0].origin = this.origin;
    rays[1].origin = this.origin;
    rays[0].dir = addVector(scale(this.directions[0], y), scale(this.directions[3], 1 - y));
    rays[1].dir = addVector(scale(this.directions[1], y), scale(this.directions[2], 1 - y));
    return rays;
  }

  render(scene: SceneClass, pixels: number[][][], width: number, height: number) {
    let cam = this;
    renderRows(cam, scene, pixels, width, height, 0, height);
  }
}

function renderRows(camera: CameraClass, scene: SceneClass, pixels: number[][][], width: number, height: number, starty: number, stopy: number) {
  for (let y = starty; y < stopy; y++) {
    let rays = camera.generateRayPair(y / height);
    for (let x = 0; x < width; x++) {
      let xp = x / width;
      let origin: number[] = addVector(scale(rays[0].origin, xp), scale(rays[1].origin, 1 - xp));
      let dir = normaliseVector(addVector(scale(rays[0].dir, xp), scale(rays[1].dir, 1 - xp)));
      let l = scene.intersect(origin, dir, 0, 0);
      pixels[y][x] = l;
    }
  }
}

function raytraceScene()
{
  let triangles: TriangleClass[] = new Array(14);//numTriangles);
  let tfl = createVector(-10,  10, -10);
  let tfr = createVector( 10,  10, -10);
  let tbl = createVector(-10,  10,  10);
  let tbr = createVector( 10,  10,  10);
  let bfl = createVector(-10, -10, -10);
  let bfr = createVector( 10, -10, -10);
  let bbl = createVector(-10, -10,  10);
  let bbr = createVector( 10, -10,  10);

  // cube!!!
  // front
  let i = 0;

  triangles[i++] = new TriangleClass(tfl, tfr, bfr);
  triangles[i++] = new TriangleClass(tfl, bfr, bfl);
  // back
  triangles[i++] = new TriangleClass(tbl, tbr, bbr);
  triangles[i++] = new TriangleClass(tbl, bbr, bbl);
  //        triangles[i-1].material = [0.7,0.2,0.2];
  //            triangles[i-1].material.reflection = 0.8;
  // left
  triangles[i++] = new TriangleClass(tbl, tfl, bbl);
  //            triangles[i-1].reflection = 0.6;
  triangles[i++] = new TriangleClass(tfl, bfl, bbl);
  //            triangles[i-1].reflection = 0.6;
  // right
  triangles[i++] = new TriangleClass(tbr, tfr, bbr);
  triangles[i++] = new TriangleClass(tfr, bfr, bbr);
  // top
  triangles[i++] = new TriangleClass(tbl, tbr, tfr);
  triangles[i++] = new TriangleClass(tbl, tfr, tfl);
  // bottom
  triangles[i++] = new TriangleClass(bbl, bbr, bfr);
  triangles[i++] = new TriangleClass(bbl, bfr, bfl);

  let ffl = createVector(-1000, -30, -1000);
  let ffr = createVector( 1000, -30, -1000);
  let fbl = createVector(-1000, -30,  1000);
  let fbr = createVector( 1000, -30,  1000);
  triangles[i++] = new TriangleClass(fbl, fbr, ffr);
  triangles[i-1].hasShader = true;
  triangles[i++] = new TriangleClass(fbl, ffr, ffl);
  triangles[i-1].hasShader = true;

  let _scene = new SceneClass(triangles);
  _scene.lights[0] = new LightsClass();
  _scene.lights[0].arr = createVector(20, 38, -22);
  _scene.lights[0].colour = createVector(0.7, 0.3, 0.3);
  _scene.lights[1] = new LightsClass();
  _scene.lights[1].arr = createVector(-23, 40, 17);
  _scene.lights[1].colour = createVector(0.7, 0.3, 0.3);
  _scene.lights[2] = new LightsClass();
  _scene.lights[2].arr = createVector(23, 20, 17);
  _scene.lights[2].colour = createVector(0.7, 0.7, 0.7);
  _scene.ambient = createVector(0.1, 0.1, 0.1);
  //  _scene.background = createVector(0.7, 0.7, 1.0);

  let size = 30;
  let pixels = new Array<Array<Array<number>>>(30);
  for (let y = 0; y < size; y++) {
    pixels[y] = new Array(30);
    for (let x = 0; x < size; x++) {
      pixels[y][x] = createVector(0,0,0);
    }
  }
  let _camera = new CameraClass(createVector(-40, 40, 40), createVector(0, 0, 0), createVector(0, 1, 0));
  _camera.render(_scene, pixels, size, size);
  return pixels;
}

export function RunThreeDRaytrace() {
  let start = ArkTools.timeInUs();
  raytraceScene();
  let end = ArkTools.timeInUs();
  let time = (end - start) / 1000
  print("Array Access - RunThreeDRaytrace:\t"+String(time)+"\tms");
  return time;
}

let loopCountForPreheat = 1;
for (let i = 0; i < loopCountForPreheat; i++) {
  RunThreeDRaytrace()
}

function sleep(delay) {
    const start = Date.now();
    while (Date.now() - start < delay) {}
}

sleep(5000);

RunThreeDRaytrace()
// let runner = new BenchmarkRunner("Array Access - RunThreeDRaytrace", RunThreeDRaytrace);
// runner.run();
