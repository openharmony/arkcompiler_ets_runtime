// CREATEOBJECTWITHEXCLUDEDKEYS_IMM8_V8_V8
const v0 = {value : 'value', age: 23, name: 'name'};
const {value, age, ...v1} = v0;
print(value);
print(age);
print(v1.value);
print(v1.age);
print(v1.name);
