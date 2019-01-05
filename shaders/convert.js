const fs = require('fs')
const path = require('path')
const glob = require('glob')
const spawn = require('child_process').spawnSync

const files = glob.sync(path.join(__dirname, '*.{vert,frag,comp}'))
const out = []
const includeExp = /#include +['"<](.*)['">]/
const splitExp = /\r?\n/

const validatorPath = 'D:\\work\\glslang\\build\\StandAlone\\Debug\\glslangValidator.exe'

const types = {
  '.frag': 'GL_FRAGMENT_SHADER',
  '.vert': 'GL_VERTEX_SHADER',
  '.comp': 'GL_COMPUTE_SHADER'
}

function outputLine(line) {
  if (Array.isArray(line)) {
    const r = line.map(outputLine).join('\n')
    return r;
  }
  return (" ").repeat(8) + '"' + line.replace(/"/g,'\\"') + '\\n"'
}

const init = files.map((file) => {
  let lines = processIncludes(file, fs.readFileSync(file, 'utf8').split(splitExp))

  const type = types[path.extname(file)]
  if (!type) {
    throw new Error(file + " could not be associated with a shader type")
  }
  const relpath = path.relative(__dirname, file)
  return `Shaders::instances["${relpath}"] = new Shader(\n` + lines.filter(Boolean).map(outputLine).join('\n') + `, "${relpath}", ${type});\n`
}).join('\n      ')


function processIncludes(file, lines) {
  let out = []
  const baseDir = path.dirname(file)
  lines.forEach(line => {
    if (line.indexOf('#include') > -1) {
      const matches = line.match(includeExp)
      if (!matches) {
        out.push(line)
      }
      const includeFile = path.join(baseDir, path.normalize(matches[1]))
      const includeLines = fs.readFileSync(includeFile, "utf8").split(splitExp)
      const includes = processIncludes(includeFile, includeLines)
      Array.prototype.push.apply(out, includes)
    } else {
      out.push(line)
    }
  })
  return out
}


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
