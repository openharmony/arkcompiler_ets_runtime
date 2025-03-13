/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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


function _inheritsLoose(subClass, superClass) {
    subClass.prototype = Object.create(superClass.prototype);
    subClass.prototype.constructor = subClass;
    _setPrototypeOf(subClass, superClass);
}

function _setPrototypeOf(o, p) {
    _setPrototypeOf = Object.setPrototypeOf ? Object.setPrototypeOf.bind() : function _setPrototypeOf(o, p) {
      o.__proto__ = p;
      return o;
    };
    return _setPrototypeOf(o, p);
}

var TemporalAccessor = function () {
  function TemporalAccessor() {}
  return TemporalAccessor;
}();

var Month = function (_TemporalAccessor) {
  _inheritsLoose(Month, _TemporalAccessor);
  function Month(value, name) {
    var _this;
    _this = _TemporalAccessor.call(this) || this;
    _this._value = value;
    _this._name = name;
    return _this;
  }
  var _proto = Month.prototype;
  _proto.value = function value() {
    return this._value;
  };
  return Month;
}(TemporalAccessor);

function nodeopt() {
    new Month(1, 'January');
}

nodeopt();
print(ArkTools.isAOTCompiled(nodeopt))
print(ArkTools.isAOTDeoptimized(nodeopt))