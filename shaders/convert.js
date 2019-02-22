const fs = require('fs')
const path = require('path')
const glob = require('glob')
const chokidar = require('chokidar')
const argv = require('yargs').argv

const sourceGlob = path.join(__dirname, '*.{vert,frag,comp}')
const files = glob.sync(sourceGlob)
const includeExp = /#include +['"<](.*)['">]/
const splitExp = /\r?\n/
const outBase = argv.o

const types = {
  '.frag': 'GL_FRAGMENT_SHADER',
  '.vert': 'GL_VERTEX_SHADER',
  '.comp': 'GL_COMPUTE_SHADER'
}

const dependencyMap = {
  // file: [dep, dep]
}


if (argv.w) {
  chokidar.watch(
    path.join(__dirname, '..'), {
      ignored: path.join(__dirname, '..', 'build')
    }
  ).on('change', (p) => {
    if (p.indexOf(".git") > -1) {
      return
    }

    console.log('CHANGE', p)
    // is this a leaf?
    if (dependencyMap[p]) {
      console.log('REBUILD', p)
      processFile(p)
    } else {
      Object.keys(dependencyMap).forEach((leaf) => {
        const deps = dependencyMap[leaf]
        if (deps.indexOf(p) > -1) {
          console.log('REBUILD', leaf)
          processFile(leaf)
        }
      })
    }
    console.log('')
  })
}


function outputLine(line) {
  if (Array.isArray(line)) {
    const r = line.map(outputLine).join('\n')
    return r;
  }
  return (" ").repeat(8) + '"' + line.replace(/"/g,'\\"') + '\\n"'
}

function processFile(file) {
  const outFile = path.join(outBase, path.basename(file))
  dependencyMap[file] = []
  const lines = processIncludes(file, dependencyMap[file], fs.readFileSync(file, 'utf8').split(splitExp), 0)
  fs.writeFileSync(outFile, lines.join('\n'))
  return lines
}

const init = files.map((file) => {
  const lines = processFile(file)
  const type = types[path.extname(file)]
  if (!type) {
    throw new Error(file + " could not be associated with a shader type")
  }
  const relpath = path.relative(__dirname, file)
  return `Shaders::instances["${relpath}"] = new Shader(\n` + lines.filter(Boolean).map(outputLine).join('\n') + `, "${relpath}", ${type});\n`
}).join('\n      ')

const s = `
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
#endif
`
fs.writeFileSync(path.join(outBase, 'built.h'), s + '\n')

function processIncludes(file, deps, lines) {
  let out = []
  const baseDir = path.dirname(file)
  lines.unshift(`// start: ${path.basename(file)}`)

  lines.forEach(line => {
    if (line.indexOf('#include') > -1) {
      const matches = line.match(includeExp)
      if (!matches) {
        out.push(line)
      }
      const includeFile = path.join(baseDir, path.normalize(matches[1]))
      deps.push(includeFile)
      const includeLines = fs.readFileSync(includeFile, "utf8").split(splitExp)
      const includes = processIncludes(includeFile, deps, includeLines)
      Array.prototype.push.apply(out, includes)
    } else {
      out.push(line)
    }
  })
  out.push(`// end: ${path.basename(file)}`)
  return out
}
