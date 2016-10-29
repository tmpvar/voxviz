const fs = require('fs')
const path = require('path')
const glob = require('glob')

const files = glob.sync(path.join(__dirname, '*.{vert,frag}'))
const out = []

const init = files.map((file) => {
  const lines = fs.readFileSync(file, 'utf8').split('\n')
  const relpath = path.relative(__dirname, file)

  const type = file.indexOf('vert') > -1 ? 'GL_VERTEX_SHADER' : 'GL_FRAGMENT_SHADER'

  return `Shaders::instances["${relpath}"] = new Shader(\n` + lines.filter(Boolean).map((line) => {
    return `        "${line}\\n"`
  }).join('\n') + `, ${type});\n`
}).join('\n      ')

out.unshift(`
#ifndef SHADER_H
#define SHADER_H

#include <map>

#include "gl-wrap.h"

class Shaders {
  public:
    static std::map<const char *, Shader *> instances;
    static void init() {
      ${init}
    }

    static Shader *get(const char *file) {
      return Shaders::instances.at(file);
    }

    static void destroy() {
      auto it = Shaders::instances.begin();
      for (; it != Shaders::instances.end(); ++it) {
        delete it->second;
      }
    }
};

// Define bytecount in file scope.
std::map<const char *, Shader *> Shaders::instances;
`)

out.push('#endif')

// const src = fs.readFileSync(, 'utf8')

// console.log(src)



fs.writeFileSync(process.argv[2], out.join('\n') + '\n')
