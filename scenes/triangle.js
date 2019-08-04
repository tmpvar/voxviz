//// ---- lib ---
async function lib() {
  const flatbuffers = require('flatbuffers').flatbuffers
  const proto = require('../proto/renderpipe_generated').renderpipe.proto
  const debug = require('debug')('renderpipe-node')
  const hex = require('hex')
  const crypto = require('crypto')
  const path = require('path')
  const builder = new flatbuffers.Builder(1024);
  const scene = proto.Scene
  const B = proto.Buffer
  const Stage = proto.Stage
  const Shader = proto.Shader
  const Program = proto.Program
  const ProgramInvocation = proto.ProgramInvocation

  function uvec3(x, y, z) {
    return proto.uvec3.createuvec3(builder, x, y, z)
  }

  function createHash(str) {
    return crypto.createHash('sha256').update(str).digest('hex')
  }

  const buffers = []
  const programs = []
  const stages = {}
  function addBuffer(options) {
    var length = 0;
    var value = [];
    if (typeof options === 'number') {
      length = options;
    } else if (options.length) {
      length = options.length
      value = initialValue
    } else {
      throw new Error("invalid initialValue")
    }

    // dims[0] = dims[0]||1
    // dims[1] = dims[1]||0
    // dims[2] = dims[2]||0
    //
    // length = Math.max(
    //   length,
    //   (dims[0]||1) * (dims[1]||1) * (dims[2]||1)
    // )

    debug("creating bufferlength: %s init: %s",
      length, value)

    const id = buffers.length
    const handle = {
      length: length,
      value: value,
      id: id
    }
    buffers.push(handle)

    return handle;
  }

  // For right now we assume that addProgram is called when
  // a program is completely ready to be added.
  function addProgram(stage, obj) {
    debug("add program", stage, obj)
    if (!stages[stage]) {
      stages[stage] = []
    }

    obj.hash = createHash(obj.shaders.map((shader) => {
      return shader.hash
    }).join(' '))

    //stages[stage].push(obj)
    // TODO: dedupe?
    programs.push(obj)

    return function(dims) {
      if (typeof dims == 'number') {
        dims = [dims, 0, 0]
      }

      if (!stages[stage]) {
        stages[stage] = []
      }

      stages[stage].push({
        programHash: obj.hash,
        dims: dims
      })
    }
  }

  // bind the stage into the program function so we can tie it back to where
  // it came from later on. The alternative is to pass this into the function
  // and require the user to do the wiring for us - not really what im going for
  // here.
  const program = addProgram.bind(null, "frame")

  function createShader(type, src) {
    debug("CREATE SHADER", type, src)
    return {
      type: type,
      hash: createHash(src),
      source: src
    }
  }

  program.frag = createShader.bind(null, proto.ShaderType.Fragment)
  program.vert = createShader.bind(null, proto.ShaderType.Vertex)
  program.compute = createShader.bind(null, proto.ShaderType.Compute)

  await frame(program, addBuffer)

  const buffersArray = buffers.map(buffer => {
    const initialOffset = buffer.value
    ? B.createInitialVector(builder, buffer.value)
    : null

    B.startBuffer(builder);
    B.addLength(builder, buffer.length)
    B.addId(builder, buffer.id)
    B.addInitial(builder, initialOffset)

    return B.endBuffer(builder)
  })

  const programsArray = programs.map((program) => {
    const shaderArray = program.shaders.map((shader) => {
      const sourceOffset = builder.createString(shader.source)
      const hash = createHash(shader.source)
      const hashOffset = builder.createString(hash)
      const filenameOffset = builder.createString("mem://shader/" + hash)
      Shader.startShader(builder)
      Shader.addType(builder, shader.type)
      Shader.addHash(builder, hashOffset)
      Shader.addFilename(builder, filenameOffset)
      Shader.addSource(builder, sourceOffset)
      return Shader.endShader(builder)
    })

    const shadersOffset = Program.createShadersVector(builder, shaderArray)
    const typeOffset = builder.createString("frame")
    const hashOffset = builder.createString(program.hash)
    Program.startProgram(builder)
    Program.addType(builder, typeOffset)
    Program.addHash(builder, hashOffset)
    Program.addShaders(builder, shadersOffset)
    return Program.endProgram(builder)
  })

  const stagesArray = Object.keys(stages).map(stageName => {
    const invocations = stages[stageName]
    const invocationArray = invocations.map(o => {
      // TODO: consider reusing this hash...
      const hashOffset = builder.createString(o.programHash)

      ProgramInvocation.startProgramInvocation(builder)
      ProgramInvocation.addProgram(builder, hashOffset)
      ProgramInvocation.addDims(builder, uvec3(
        o.dims[0],
        o.dims[1],
        o.dims[2]
      ))
      return ProgramInvocation.endProgramInvocation(builder)
    })

    const invocationsOffset = Stage.createInvocationsVector(builder, invocationArray)
    const nameOffset = builder.createString(stageName)

    Stage.startStage(builder)
    Stage.addName(builder, nameOffset)
    Stage.addInvocations(builder, invocationsOffset)
    return Stage.endStage(builder)
  })

  debug("stages", stages)

  const programsOffset = scene.createProgramsVector(builder, programsArray)
  const buffersOffset = scene.createBuffersVector(builder, buffers)
  const nameOffset = builder.createString(path.basename(__filename))
  const stagesOffset = scene.createStagesVector(builder, stagesArray)

  scene.startScene(builder);
  scene.addName(builder, nameOffset)
  scene.addBuffers(builder, buffersOffset)
  scene.addPrograms(builder, programsOffset)
  scene.addStages(builder, stagesOffset)

  const sv = scene.endScene(builder);

  builder.finish(sv)

  const data = builder.asUint8Array()

  // debug hex output
  if (process.env.DEBUG) {
    hex(Buffer.from(data, 0, data.length))
  } else {
    process.stdout.write(data)
  }
}
lib()

///// ---- script ---
async function frame(program, buffer) {
  buffer(Math.pow(1024, 2));

  const triangle = program({
    shaders: [
      program.vert(`
        #version 410
        void main() {
          if (gl_VertexID == 0) gl_Position = vec4( 0.5, -0.5, 0, 1);
          if (gl_VertexID == 1) gl_Position = vec4( 0.0,  0.5, 0, 1);
          if (gl_VertexID == 2) gl_Position = vec4(-0.5, -0.5, 0, 1);
        }
      `),
      program.frag(`
        #version 410
        out vec4 color;
        void main() {
          color = vec4(1.0, 0.0, 1.0, 1.0);
        }
      `)
    ]
  })

  triangle(3)

  return new Promise((y, m) => {
    setTimeout(y, 10);
  })
}
