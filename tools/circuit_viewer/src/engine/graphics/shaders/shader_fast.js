/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

const shaderFastVs = `
attribute vec3 position;
attribute vec2 aTexCoord;
attribute vec4 ext1;//x,y,sw,sh,
attribute vec4 ext2;//ra,ox,oy,i
attribute float inColor;
uniform mat4 uMat;

varying vec2 TexCoord;
varying float tex_point;
varying vec4 clr_filter;
void main()
{
    mat4 tmpMat=mat4(ext1[0],ext1[2],0.0,ext2[1],
                      ext1[1],ext1[3],0.0,ext2[2],
                      0.0,0.0,ext2[0],0.0,
                      0.0,0.0,0.0,1.0);
    
    vec4 tv=vec4(position.x, position.y, position.z, 1.0)*tmpMat*uMat;
    gl_Position = tv;
    TexCoord = aTexCoord;
    tex_point = ext2[3];

    clr_filter=vec4(0.0,0.0,0.0,1.0);
    int tt=int(inColor);

    int tt1=tt-tt/64*64;
    clr_filter.b=0.015873015873*float(tt1);
    tt=tt/64;

    int tt2=tt-tt/64*64;
    clr_filter.g=0.015873015873*float(tt2);
    tt=tt/64;

    int tt3=tt-tt/64*64;
    clr_filter.r=0.015873015873*float(tt3);
    tt=tt/64;
    
    clr_filter.a=0.015873015873*float(tt-tt/64*64);
}
`;

const shaderFastFs = `
precision mediump float;

varying vec2 TexCoord;
varying float tex_point;
varying vec4 clr_filter;
uniform sampler2D tex0;
uniform sampler2D tex1;
uniform sampler2D tex2;
uniform sampler2D tex3;
uniform sampler2D tex4;
uniform sampler2D tex5;
uniform sampler2D tex6;
uniform sampler2D tex7;
uniform sampler2D tex8;
uniform sampler2D tex9;
uniform sampler2D tex10;
uniform sampler2D tex11;
uniform sampler2D tex12;
uniform sampler2D tex13;
uniform sampler2D tex14;
uniform sampler2D tex15;

void main()
{
    if(tex_point<0.5)gl_FragColor = texture2D(tex0, TexCoord);
    else if(tex_point<1.5)gl_FragColor = texture2D(tex1, TexCoord);
    else if(tex_point<2.5)gl_FragColor = texture2D(tex2, TexCoord);
    else if(tex_point<3.5)gl_FragColor = texture2D(tex3, TexCoord);
    else if(tex_point<4.5)gl_FragColor = texture2D(tex4, TexCoord);
    else if(tex_point<5.5)gl_FragColor = texture2D(tex5, TexCoord);
    else if(tex_point<6.5)gl_FragColor = texture2D(tex6, TexCoord);
    else if(tex_point<7.5)gl_FragColor = texture2D(tex7, TexCoord);
    else if(tex_point<8.5)gl_FragColor = texture2D(tex8, TexCoord);
    else if(tex_point<9.5)gl_FragColor = texture2D(tex9, TexCoord);
    else if(tex_point<10.5)gl_FragColor = texture2D(tex10, TexCoord);
    else if(tex_point<11.5)gl_FragColor = texture2D(tex11, TexCoord);
    else if(tex_point<12.5)gl_FragColor = texture2D(tex12, TexCoord);
    else if(tex_point<13.5)gl_FragColor = texture2D(tex13, TexCoord);
    else if(tex_point<14.5)gl_FragColor = texture2D(tex14, TexCoord);
    else if(tex_point<15.5)gl_FragColor = texture2D(tex15, TexCoord);
    gl_FragColor=gl_FragColor * clr_filter;
}`;

module.exports = {
  shaderFastVs,
  shaderFastFs,
};
