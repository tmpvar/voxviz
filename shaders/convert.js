const fs = require('fs')
const path = require('path')
const glob = require('glob')
const chokidar = require('chokidar')
const argv = require('yargs').argv

const sourceGlob = path.join(__dirname, '*.{vert,frag,comp}')
const files = glob.sync(sourceGlob)
const includeExp = /#include +['"<](.*)['">]/
const splitExp = /\r?\n/
const outBase = path.resolve(process.cwd(), argv.o)

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

    p = path.normalize(p)
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

function spath(p) {
  return p.replace(/\\/g, '\\\\')
}

const init = files.map((file) => {
  file = path.normalize(file);
  const outFile = path.join(outBase, path.basename(file))
  const lines = processFile(file)
  const type = types[path.extname(file)]
  if (!type) {
    throw new Error(file + " could not be associated with a shader type")
  }
  const relpath = path.relative(__dirname, file)
  return `Shaders::instances["${relpath}"] = new Shader(\n` + lines.filter(Boolean).map(outputLine).join('\n') + `, "${spath(outFile)}", "${relpath}", ${type});\n`
}).join('\n      ')

const s = `
#ifndef SHADER_H
#define SHADER_H

#include <map>

#include "gl-wrap.h"

#include <stdio.h>
#include <uv.h>

static void on_change(uv_fs_event_t *handle, const char *path, int events, int status);
static uv_fs_event_t shader_watcher_handle;

class Shaders {
  public:
    static std::map<std::string, Shader *> instances;
    static void init() {
      ${init}

      // TODO: setup some libuv junk here and start watching the build dir for changes.
      // when a shader changes we need to mark it as dirty. Every subsequent program that tries
      // to use the old program needs to be rebuilt. This dirty state needs to persist across
      // potentially many frames.
      #ifdef SHADER_HOTRELOAD
      printf("starting shader hot-reload disk watcher\n")
      uv_fs_event_init(uv_default_loop(), &shader_watcher_handle);
      uv_fs_event_start(
        &shader_watcher_handle,
        on_change,
        "${spath(outBase)}",
        UV_FS_EVENT_RECURSIVE
      );
      #endif
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

static void on_change(uv_fs_event_t *handle, const char *path, int events, int status) {
  if (events & UV_CHANGE) {
    Shader *shader = Shaders::get(path);
    if (shader != nullptr) {
      shader->reload();
    }
    printf("CHANGE: %s\\n", path);
  }
}
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
      deps.push(path.normalize(includeFile))
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
