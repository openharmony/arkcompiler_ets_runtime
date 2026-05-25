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

let checkNumber: number = 0;

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------

// The rest of this file is the actual ray tracer written by Adam
// Burmister. It's a concatenation of the following files:
//
//   flog/color.js
//   flog/light.js
//   flog/vector.js
//   flog/ray.js
//   flog/scene.js
//   flog/material/basematerial.js
//   flog/material/solid.js
//   flog/material/chessboard.js
//   flog/shape/baseshape.js
//   flog/shape/sphere.js
//   flog/shape/plane.js
//   flog/intersectioninfo.js
//   flog/camera.js
//   flog/background.js
//   flog/engine.js

let debug: boolean = false;
function debugLog(msg: string): void {
  if (debug) {
    print(msg);
  }
}
const MAX_COLOR_VALUE: number = 255;
const WEIGHT_RED: number = 77;
const WEIGHT_GREEN: number = 150;
const WEIGHT_BLUE: number = 29;

const NUMBER_TWO: number = 2.0;
const NUMBER_THREE: number = 3;
const NUMBER_FIVE: number = 5;
const NUMBER_EIGHT: number = 8;
const NUMBER_TEN: number = 10.0;
const NUMBER_ONE_POINT_TWO: number = 1.2;
const NUMBER_ONE_POINT_FIVE: number = 1.5;
const NUMBER_POINT_ONE: number = 0.1;
const NUMBER_POINT_TWO: number = 0.2;
const NUMBER_POINT_TWO_FIVE: number = 0.25;
const NUMBER_POINT_THREE: number = 0.3;
const NUMBER_POINT_FOUR: number = 0.4;
const NUMBER_POINT_FIVE: number = 0.5;
const NUMBER_POINT_SEVEN: number = 0.7;
const NUMBER_POINT_EIGHT: number = 0.8;
const NUMBER_POINT_NINE: number = 0.9;
const NUMBER_ONE_THOUSAND: number = 1000;
const NUMBER_ONE_HUNDRED: number = 100;

const CHECK_NUMBER: number = 2321;
const MAX_DISTANCE: number = 2000;
const DEFAULT_CANVAS_HEIGHT: number = 100;
const DEFAULT_CANVAS_WIDTH: number = 100;
const DEFAULT_VECTOR_Z: number = -15;

const MAX_RUN_TIMES: number = 20;
// Defined class Color
class Color {
  red: number = 0.0;
  green: number = 0.0;
  blue: number = 0.0;

  constructor(r: number | null, g: number | null, b: number | null) {
    this.red = r ?? 0.0;
    this.green = g ?? 0.0;
    this.blue = b ?? 0.0;
  }

  static add(c1: Color, c2: Color): Color {
    let result = new Color(0, 0, 0);

    result.red = c1.red + c2.red;
    result.green = c1.green + c2.green;
    result.blue = c1.blue + c2.blue;

    return result;
  }

  static addScalar(c1: Color, s: number): Color {
    let result = new Color(0, 0, 0);

    result.red = c1.red + s;
    result.green = c1.green + s;
    result.blue = c1.blue + s;

    result.limit();

    return result;
  }

  subtract(c1: Color, c2: Color): Color {
    let result = new Color(0, 0, 0);

    result.red = c1.red - c2.red;
    result.green = c1.green - c2.green;
    result.blue = c1.blue - c2.blue;

    return result;
  }

  static multiply(c1: Color, c2: Color): Color {
    let result = new Color(0, 0, 0);

    result.red = c1.red * c2.red;
    result.green = c1.green * c2.green;
    result.blue = c1.blue * c2.blue;

    return result;
  }

  static multiplyScalar(c1: Color, f: number): Color {
    let result = new Color(0, 0, 0);

    result.red = c1.red * f;
    result.green = c1.green * f;
    result.blue = c1.blue * f;

    return result;
  }

  divideFactor(c1: Color, f: number): Color {
    let result = new Color(0, 0, 0);

    result.red = c1.red / f;
    result.green = c1.green / f;
    result.blue = c1.blue / f;

    return result;
  }

  limit(): void {
    this.red = this.red > 0.0 ? (this.red > 1.0 ? 1.0 : this.red) : 0.0;
    this.green = this.green > 0.0 ? (this.green > 1.0 ? 1.0 : this.green) : 0.0;
    this.blue = this.blue > 0.0 ? (this.blue > 1.0 ? 1.0 : this.blue) : 0.0;
  }

  distance(color: Color): number {
    let d =
      Math.abs(this.red - color.red) +
      Math.abs(this.green - color.green) +
      Math.abs(this.blue - color.blue);
    return d;
  }

  static blend(c1: Color, c2: Color, w: number): Color {
    let result = new Color(0, 0, 0);
    result = Color.add(
      Color.multiplyScalar(c1, 1 - w),
      Color.multiplyScalar(c2, w)
    );
    return result;
  }

  brightness(): number {
    let r = Math.floor(this.red * MAX_COLOR_VALUE);
    let g = Math.floor(this.green * MAX_COLOR_VALUE);
    let b = Math.floor(this.blue * MAX_COLOR_VALUE);

    return (
      (r * WEIGHT_RED + g * WEIGHT_GREEN + b * WEIGHT_BLUE) >> NUMBER_EIGHT
    );
  }

  toString(): String {
    let r = Math.floor(this.red * MAX_COLOR_VALUE);
    let g = Math.floor(this.green * MAX_COLOR_VALUE);
    let b = Math.floor(this.blue * MAX_COLOR_VALUE);
    return 'rgb(' + r + ',' + g + ',' + b + ')';
  }
}

// Defined class Light
class Light {
  position: Vector;
  color: Color;
  intensity: number = NUMBER_TEN;

  constructor(pos: Vector, c: Color, i: number = NUMBER_TEN) {
    this.position = pos;
    this.color = c;
    this.intensity = i;
  }

  toString(): string {
    return (
      'Light [' +
      this.position.x +
      ',' +
      this.position.y +
      ',' +
      this.position.z +
      ']'
    );
  }
}

// Defined class Vector
class Vector {
  x: number = 0.0;
  y: number = 0.0;
  z: number = 0.0;

  constructor(xF: number | null, yF: number | null, zF: number | null) {
    this.x = xF ?? 0;
    this.y = yF ?? 0;
    this.z = zF ?? 0;
  }

  copy(vector: Vector): void {
    this.x = vector.x;
    this.y = vector.y;
    this.z = vector.z;
  }

  normalize(): Vector {
    let m = this.magnitude();
    return new Vector(this.x / m, this.y / m, this.z / m);
  }

  magnitude(): number {
    return Math.sqrt(this.x * this.x + this.y * this.y + this.z * this.z);
  }

  cross(w: Vector): Vector {
    return new Vector(
      -this.z * w.y + this.y * w.z,
      this.z * w.x - this.x * w.z,
      -this.y * w.x + this.x * w.y
    );
  }

  dot(w: Vector): number {
    return this.x * w.x + this.y * w.y + this.z * w.z;
  }

  static add(v: Vector, w: Vector): Vector {
    return new Vector(w.x + v.x, w.y + v.y, w.z + v.z);
  }

  static subtract(v: Vector, w: Vector): Vector {
    if (!w || !v) {
      print('Vectors must be defined');
    }
    return new Vector(v.x - w.x, v.y - w.y, v.z - w.z);
  }

  multiplyVector(v: Vector, w: Vector): Vector {
    return new Vector(v.x * w.x, v.y * w.y, v.z * w.z);
  }

  static multiplyScalar(v: Vector, w: number): Vector {
    return new Vector(v.x * w, v.y * w, v.z * w);
  }

  toString(): String {
    return 'Vector [' + this.x + ',' + this.y + ',' + this.z + ']';
  }
}

// Defined class Ray
class Ray {
  position: Vector;
  direction: Vector;

  constructor(pos: Vector, dir: Vector) {
    this.position = pos;
    this.direction = dir;
  }

  toString(): String {
    return 'Ray [' + this.position + ',' + this.direction + ']';
  }
}

// Defined class Scene
class Scene {
  camera = new Camera(
    new Vector(0, 0, -NUMBER_FIVE),
    new Vector(0, 0, 1),
    new Vector(0, 1, 0)
  );
  shapes = Array<Baseshape>();
  lights = Array<Light>();
  background = new Background(
    new Color(0, 0, NUMBER_POINT_FIVE),
    NUMBER_POINT_TWO
  );
}

// Defined class BaseMaterial
class BaseMaterial {
  gloss: number = NUMBER_TWO;
  transparency: number = 0.0;
  reflection: number = 0.0;
  refraction: number = NUMBER_POINT_FIVE;
  hasTexture = false;

  getColor(u: number, v: number): Color {
    return new Color(0, 0, 0);
  }

  wrapUp(t: number): number {
    t = t % NUMBER_TWO;
    if (t < -1) {
      t += NUMBER_TWO;
    }
    if (t >= 1) {
      t -= NUMBER_TWO;
    }
    return t;
  }

  toString(): String {
    return (
      'Material [gloss=' +
      this.gloss +
      ', transparency=' +
      this.transparency +
      ', hasTexture=' +
      this.hasTexture +
      ']'
    );
  }
}

// Defined class Solid
class Solid extends BaseMaterial {
  color: Color;

  constructor(c: Color, refle: number, refra: number, tra: number, g: number) {
    super();
    this.color = c;
    this.reflection = refle;
    this.transparency = tra;
    this.gloss = g;
    this.hasTexture = false;
  }

  getColor(u: number, v: number): Color {
    return this.color;
  }

  toString(): String {
    return (
      'SolidMaterial [gloss=' +
      this.gloss +
      ', transparency=' +
      this.transparency +
      ', hasTexture=' +
      this.hasTexture +
      ']'
    );
  }
}

// Defined class Chessboard
class Chessboard extends BaseMaterial {
  colorEven: Color;
  colorOdd: Color;
  density: number = NUMBER_POINT_FIVE;

  constructor(
    cEven: Color,
    cOdd: Color,
    refle: number,
    tra: number,
    g: number,
    den: number
  ) {
    super();
    this.colorEven = cEven;
    this.colorOdd = cOdd;
    this.reflection = refle;
    this.transparency = tra;
    this.gloss = g;
    this.density = den;
    this.hasTexture = true;
  }

  getColor(u: number, v: number): Color {
    let t = this.wrapUp(u * this.density) * this.wrapUp(v * this.density);

    if (t < 0.0) {
      return this.colorEven;
    } else {
      return this.colorOdd;
    }
  }

  toString(): String {
    return (
      'ChessMaterial [gloss=' +
      this.gloss +
      ', transparency=' +
      this.transparency +
      ', hasTexture=' +
      this.hasTexture +
      ']'
    );
  }
}

// Defined class Baseshape
class Baseshape {
  position: Vector = new Vector(0, 0, 0);
  radius: number = 0.0;
  material: BaseMaterial = new BaseMaterial();

  intersect(ray: Ray): IntersectionInfo | null {
    return null;
  }
}

// Defined class Sphere
class Sphere extends Baseshape {
  constructor(pos: Vector, rad: number, mater: BaseMaterial) {
    super();
    this.radius = rad;
    this.position = pos;
    this.material = mater;
  }

  intersect(ray: Ray): IntersectionInfo {
    let info = new IntersectionInfo();
    info.shape = this;
    let dst = Vector.subtract(ray.position, this.position);
    let B = dst.dot(ray.direction);
    let C = dst.dot(dst) - this.radius * this.radius;
    let D = B * B - C;
    if (D > 0) {
      info.isHit = true;
      info.distance = -B - Math.sqrt(D);
      info.position = Vector.add(
        ray.position,
        Vector.multiplyScalar(ray.direction, info.distance)
      );
      info.normal = Vector.subtract(info.position, this.position).normalize();
      info.color = this.material?.getColor(0, 0);
    } else {
      info.isHit = false;
    }
    return info;
  }

  toString(): string {
    return (
      'Sphere [position=' + this.position + ', radius=' + this.radius + ']'
    );
  }
}

// Defined class Plane
class Plane extends Baseshape {
  d: number = 0.0;

  constructor(pos: Vector, dF: number, mater: BaseMaterial) {
    super();
    this.position = pos;
    this.d = dF;
    this.material = mater;
  }

  intersect(ray: Ray): IntersectionInfo {
    let info = new IntersectionInfo();
    let vD = this.position?.dot(ray.direction) ?? 0;
    if (vD === 0) {
      return info;
    }
    let t = -(this.position?.dot(ray.position) + this.d) / vD;
    if (t <= 0) {
      return info;
    }
    info.shape = this;
    info.isHit = true;

    info.position = Vector.add(
      ray.position,
      Vector.multiplyScalar(ray.direction!, t)
    );
    info.normal = this.position;
    info.distance = t;

    if (this.material?.hasTexture != null) {
      let vU = new Vector(this.position.y, this.position.z, -this.position.x);
      let vV = vU.cross(this.position);
      let u = info.position?.dot(vU);
      let v = info.position?.dot(vV);
      info.color = this.material?.getColor(u, v);
    } else {
      info.color = this.material?.getColor(0, 0);
    }
    return info;
  }

  toString(): string {
    return 'Plane [' + this.position + ', d=' + this.d + ']';
  }
}

// Defined class IntersectionInfo
class IntersectionInfo {
  isHit: boolean = false;
  hitCount: number = 0;
  shape: Baseshape = new Baseshape();
  position: Vector = new Vector(0, 0, 0);
  normal: Vector = new Vector(0, 0, 0);
  color: Color = new Color(0, 0, 0);
  distance: number = 0;

  toString(): String {
    return 'Intersection [' + this.position + ']';
  }
}

// Defined class Camera
class Camera {
  position: Vector;
  lookAt: Vector;
  equator: Vector;
  up: Vector;
  screne: Vector;

  constructor(pos: Vector, lookV: Vector, upV: Vector) {
    this.position = pos;
    this.lookAt = lookV;
    this.up = upV;
    this.equator = this.lookAt.normalize().cross(this.up);
    this.screne = Vector.add(this.position, this.lookAt);
  }

  getRay(vx: number, vy: number): Ray {
    let pos = Vector.subtract(
      this.screne,
      Vector.subtract(
        Vector.multiplyScalar(this.equator, vx),
        Vector.multiplyScalar(this.up, vy)
      )
    );
    pos.y *= -1;
    let dir = Vector.subtract(pos, this.position);
    let ray = new Ray(pos, dir.normalize());
    return ray;
  }

  toString(): string {
    return 'Ray []';
  }
}

// Defined class Background
class Background {
  color: Color;
  ambience: number = 0.0;

  constructor(c: Color, amb: number) {
    this.color = c;
    this.ambience = amb;
  }
}

// Defined class Engine
class Engine {
  options: Options;

  constructor(opt: Options) {
    this.options = opt;
    this.options.canvasHeight /= this.options.pixelHeight;
    this.options.canvasWidth /= this.options.pixelWidth;
  }

  setPixel(x: number, y: number, color: Color): void {
    if (x === y) {
      checkNumber += color.brightness();
    }
  }

  renderScene(scene: Scene): void {
    checkNumber = 0;

    let canvasHeight = this.options.canvasHeight;
    let canvasWidth = this.options.canvasWidth;

    for (let y = 0; y < canvasHeight; y++) {
      for (let x = 0; x < canvasWidth; x++) {
        let yp = ((y * 1.0) / canvasHeight) * NUMBER_TWO - 1;
        let xp = ((x * 1.0) / canvasWidth) * NUMBER_TWO - 1;

        let ray = scene.camera.getRay(xp, yp);

        let color = this.getPixelColor(ray, scene);

        this.setPixel(x, y, color);

        // debugLog("y = " + y + ", x = " + x + ", ray.position = " + ray.position?.toString());
        // debugLog("y = " + y + ", x = " + x + ", ray.direction = " + ray.direction?.toString());
        // debugLog("y = " + y + ", x = " + x + ", color = " + color.toString());
      }
    }

    // debugLog("checkNumber = " + checkNumber);
    if (checkNumber !== CHECK_NUMBER) {
      print('Scene rendered incorrectly');
    }
  }

  getPixelColor(ray: Ray, scene: Scene): Color {
    let info = this.testIntersection(ray, scene, null);
    if (info.isHit) {
      let color = this.rayTrace(info, ray, scene, 0);
      return color;
    }
    return scene.background.color;
  }

  testIntersection(
    ray: Ray,
    scene: Scene,
    exclude: Baseshape | null
  ): IntersectionInfo {
    let hits = 0;
    let best = new IntersectionInfo();
    best.distance = MAX_DISTANCE;

    for (let i = 0; i < scene.shapes.length; i++) {
      let shape = scene.shapes[i];

      if (shape !== exclude) {
        let info = shape.intersect(ray);
        if (
          info != null &&
          info.isHit &&
          info.distance >= 0 &&
          info.distance < best.distance
        ) {
          best = info;
          hits++;
        }
      }
    }
    best.hitCount = hits;
    return best;
  }

  getReflectionRay(p: Vector, n: Vector, v: Vector): Ray {
    let c1 = -n.dot(v);
    let R1 = Vector.add(Vector.multiplyScalar(n, NUMBER_TWO * c1), v);
    return new Ray(p, R1);
  }

  rayTrace(
    info: IntersectionInfo,
    ray: Ray,
    scene: Scene,
    depth: number
  ): Color {
    // Calc ambient
    let color = Color.multiplyScalar(info.color!, scene.background.ambience);
    let oldColor = color;
    let shininess = Math.pow(NUMBER_TEN, info.shape.material.gloss + 1);

    for (let i = 0; i < scene.lights.length; i++) {
      let light = scene.lights[i];

      // Calc diffuse lighting
      let v = Vector.subtract(light.position, info.position).normalize();
      if (this.options.renderDiffuse) {
        let L = v.dot(info.normal!);
        if (L > 0.0) {
          color = Color.add(
            color,
            Color.multiply(info.color!, Color.multiplyScalar(light.color!, L))
          );
        }
      }
      color = this.getColorByDepth(info, ray, scene, depth, color);
      // Render shadows and highlights
      let shadowInfo = new IntersectionInfo();
      color = this.renderShadowsAndHighlights(
        info,
        scene,
        shadowInfo,
        color,
        v,
        light,
        shininess
      );
    }
    color.limit();
    return color;
  }

  getColorByDepth(
    info: IntersectionInfo,
    ray: Ray,
    scene: Scene,
    depth: number,
    color: Color
  ): Color {
    // The greater the depth the more accurate the colours, but
    // this is exponentially (!) expensive
    if (depth <= this.options.rayDepth) {
      // calculate reflection ray
      if (
        this.options.renderReflections &&
        info.shape?.material!.reflection! > 0
      ) {
        let reflectionRay = this.getReflectionRay(
          info.position!,
          info.normal!,
          ray.direction!
        );
        let refl = this.testIntersection(reflectionRay, scene, info.shape!);

        if (refl.isHit && refl.distance! > 0) {
          refl.color = this.rayTrace(refl, reflectionRay, scene, depth + 1);
        } else {
          refl.color = scene.background.color;
        }
        color = Color.blend(
          color,
          refl.color!,
          info.shape?.material!.reflection!
        );
      }
    }
    return color;
  }

  renderShadowsAndHighlights(
    info: IntersectionInfo,
    scene: Scene,
    shadowInfo: IntersectionInfo,
    color: Color,
    v: Vector,
    light: Light,
    shininess: number
  ): Color {
    if (this.options.renderShadows) {
      let shadowRay = new Ray(info.position!, v);
      shadowInfo = this.testIntersection(shadowRay, scene, info.shape);
      if (shadowInfo.isHit && shadowInfo.shape !== info.shape) {
        let vA = Color.multiplyScalar(color, NUMBER_POINT_FIVE);
        let dB =
          NUMBER_POINT_FIVE *
          Math.pow(
            shadowInfo.shape?.material!.transparency!,
            NUMBER_POINT_FIVE
          );
        color = Color.addScalar(vA, dB);
      }
    }
    // Phong specular highlights
    if (
      this.options.renderHighlights &&
      !shadowInfo.isHit &&
      info.shape?.material!.gloss! > 0
    ) {
      let lV = Vector.subtract(info.shape.position, light.position).normalize();
      let eVector = Vector.subtract(
        scene.camera.position,
        info.shape?.position
      ).normalize();
      let hVector = Vector.subtract(eVector, lV).normalize();
      let glossWeight = Math.pow(
        Math.max(info.normal?.dot(hVector)!, 0.0),
        shininess
      );
      color = Color.add(Color.multiplyScalar(light.color!, glossWeight), color);
    }
    return color;
  }
}

// Defined class Options
class Options {
  canvasHeight: number = DEFAULT_CANVAS_HEIGHT;
  canvasWidth: number = DEFAULT_CANVAS_WIDTH;
  pixelWidth: number = NUMBER_TWO;
  pixelHeight: number = NUMBER_TWO;
  renderDiffuse: boolean = false;
  renderShadows: boolean = false;
  renderHighlights: boolean = false;
  renderReflections: boolean = false;
  rayDepth: number = NUMBER_TWO;
}

function initShapes(scene: Scene): Scene {
  let sphere = new Sphere(
    new Vector(-NUMBER_ONE_POINT_FIVE, NUMBER_ONE_POINT_FIVE, NUMBER_TWO),
    NUMBER_ONE_POINT_FIVE,
    new Solid(
      new Color(0, NUMBER_POINT_FIVE, NUMBER_POINT_FIVE),
      NUMBER_POINT_THREE,
      0.0,
      0.0,
      NUMBER_TWO
    )
  );

  let sphere1 = new Sphere(
    new Vector(1, NUMBER_POINT_TWO_FIVE, 1),
    NUMBER_POINT_FIVE,
    new Solid(
      new Color(NUMBER_POINT_NINE, NUMBER_POINT_NINE, NUMBER_POINT_NINE),
      NUMBER_POINT_ONE,
      0.0,
      0.0,
      NUMBER_ONE_POINT_FIVE
    )
  );

  let plane = new Plane(
    new Vector(
      NUMBER_POINT_ONE,
      NUMBER_POINT_NINE,
      -NUMBER_POINT_FIVE
    ).normalize(),
    NUMBER_ONE_POINT_TWO,
    new Chessboard(
      new Color(1, 1, 1),
      new Color(0, 0, 0),
      NUMBER_POINT_TWO,
      0.0,
      1,
      NUMBER_POINT_SEVEN
    )
  );

  scene.shapes.push(sphere);
  scene.shapes.push(sphere1);
  scene.shapes.push(plane);

  return scene;
}

function initLights(scene: Scene): Scene {
  let light = new Light(
    new Vector(NUMBER_FIVE, NUMBER_TEN, -1),
    new Color(NUMBER_POINT_EIGHT, NUMBER_POINT_EIGHT, NUMBER_POINT_EIGHT),
    0
  );

  let light1 = new Light(
    new Vector(-NUMBER_THREE, NUMBER_FIVE, DEFAULT_VECTOR_Z),
    new Color(NUMBER_POINT_EIGHT, NUMBER_POINT_EIGHT, NUMBER_POINT_EIGHT),
    NUMBER_ONE_HUNDRED
  );

  scene.lights.push(light);
  scene.lights.push(light1);

  return scene;
}

function renderScene(): void {
  let scene = new Scene();

  scene.camera = new Camera(
    new Vector(0, 0, DEFAULT_VECTOR_Z),
    new Vector(-NUMBER_POINT_TWO, 0, NUMBER_FIVE),
    new Vector(0, 1, 0)
  );
  scene.background = new Background(
    new Color(NUMBER_POINT_FIVE, NUMBER_POINT_FIVE, NUMBER_POINT_FIVE),
    NUMBER_POINT_FOUR
  );

  scene = initShapes(scene);
  scene = initLights(scene);

  let option = new Options();
  option.canvasWidth = DEFAULT_CANVAS_WIDTH;
  option.canvasHeight = DEFAULT_CANVAS_HEIGHT;
  option.pixelWidth = NUMBER_FIVE;
  option.pixelHeight = NUMBER_FIVE;
  option.renderDiffuse = true;
  option.renderHighlights = true;
  option.renderShadows = true;
  option.renderReflections = true;
  option.rayDepth = NUMBER_TWO;

  let raytracer = new Engine(option);
  raytracer.renderScene(scene);

  // debugLog("Create camera: ");
  // debugLog(scene.camera.toString());
  // debugLog("Create Background： ");
  // debugLog(scene.background.toString());
  // debugLog("Create sphere: ");
  // debugLog(sphere.toString());
  // debugLog("Create sphere1: ");
  // debugLog(sphere1.toString());
  // debugLog("Create plane: ");
  // debugLog(plane.toString());
  // debugLog("Create light: ");
  // debugLog(light.toString());
  // debugLog("Create light1: ");
  // debugLog(light1.toString());
  // debugLog("Create option ");
  // debugLog(String(option));
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
    for (let i = 0; i < NUMBER_FIVE; i++) {
      renderScene();
    }
  }
}

export default function run(): void {
  let startTime = ArkTools.timeInUs() / NUMBER_ONE_THOUSAND;
  let raytrace = new Benchmark();
  for (let index = 0; index < MAX_RUN_TIMES; index++) {
    raytrace.runIteration();
  }
  let endTime = ArkTools.timeInUs() / NUMBER_ONE_THOUSAND;
  print("raytrace: ms = " + (endTime - startTime));
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
