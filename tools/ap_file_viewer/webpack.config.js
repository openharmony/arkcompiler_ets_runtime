/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

const path = require('path');
const childProcess = require('child_process');
const fs = require('fs');

const staticPath = ['/src/img', '/server'];
const staticFiles = [
    '/src/index.html',
    '/src/base-ui/icon.svg'
];

function cpDir(sourcePath, targetPath) {
    fs.readdir(sourcePath, async (err, files) => {
        if (err) {
            console.error('unable read dir', err);
            return;
        }
        for (const file of files) {
            const source = `${sourcePath}/${file}`;
            const target = `${targetPath}/${file}`;
            await cpFile(source, target);
        }
    });
}

async function cpFile(source, target) {
    if (fs.lstatSync(source).isFile()) {
        const dirPath = path.dirname(target);
        if (!fs.existsSync(dirPath)) {
            await fs.promises.mkdir(dirPath, {recursive: true});
        }
        await fs.promises.copyFile(source, target);
    }
}

function clearDirectory(directoryPath) {
    let files = [];
    if (fs.existsSync(directoryPath)) {
        files = fs.readdirSync(directoryPath);
        files.forEach((file, index) => {
            let curPath = directoryPath + '/' + file;
            if (fs.statSync(curPath).isDirectory()) {
                clearDirectory(curPath);
            } else {
                fs.unlinkSync(curPath);
            }
        });
        fs.rmdirSync(directoryPath);
    }
}


module.exports = (env, argv) => {
    const outPath = path.normalize(path.join(__dirname, '/', 'dist'));
    clearDirectory(outPath);
    staticPath.forEach((value) => {
        let pa = path.join(__dirname, value);
        let distPath;
        if (value.startsWith('/src')) {
            distPath = path.join(outPath, value.substring(4, value.length + 1));
        } else if (value.startsWith('/server')) {
            distPath = path.join(outPath, value.substring(7, value.length + 1));
        }
        cpDir(pa, distPath);
    });
    staticFiles.forEach((value) => {
        let filePath = path.join(__dirname, value);
        let distFile;
        if (value.startsWith('/src')) {
            distFile = path.join(outPath, value.substring(4, value.length + 1));
        } else if (value.startsWith('/server')) {
            distFile = path.join(outPath, value.substring(7, value.length + 1));
        }
        cpFile(filePath, distFile);
    });

    return {
        mode: 'production',
        entry: './src/index.js',
        output: {
            path: path.resolve(__dirname, 'dist'),
            filename: 'index.js',
        }
    };
};
