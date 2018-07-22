const fs = require('fs')
const path = require('path')
const glob = require('glob')

const files = glob.sync(path.join(__dirname, '*.{vert,frag}'))
const out = []

const init = files.map((file) => {
  const lines = fs.readFileSync(file, 'utf8').split(/\r?\n/)
  const relpath = path.relative(__dirname, file)

  const type = file.indexOf('vert') > -1 ? 'GL_VERTEX_SHADER' : 'GL_FRAGMENT_SHADER'

  return `Shaders::instances["${relpath}"] = new Shader(\n` + lines.filter(Boolean).map((line) => {
    return `        "${line.trim()}\\n"`
  }).join('\r\n') + `, "${relpath}", ${type});\n`
}).join('\r\n      ')

out.unshift(`
#ifndef SHADER_H
#define SHADER_H

#include <map>

#include "gl-wrap.h"

class Shaders {
  public:
    static std::map<std::string, Shader *> instances;
    static void init() {
      ${init}
    }

    static Shader *get(const std::string file) {
      return Shaders::instances.at(file);
    }

    static void destroy() {
      auto it = Shaders::instances.begin();
      for (; it != Shaders::instances.end(); ++it) {
        delete it->second;
      }
    }
};

std::map<std::string, Shader *> Shaders::instances;
`)

out.push('#endif')

fs.writeFileSync(process.argv[2], out.join('\n') + '\n')
