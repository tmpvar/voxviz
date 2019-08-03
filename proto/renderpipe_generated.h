// automatically generated by the FlatBuffers compiler, do not modify


#ifndef FLATBUFFERS_GENERATED_RENDERPIPE_RENDERPIPE_PROTO_H_
#define FLATBUFFERS_GENERATED_RENDERPIPE_RENDERPIPE_PROTO_H_

#include "flatbuffers/flatbuffers.h"

namespace renderpipe {
namespace proto {

struct uvec3;

struct Buffer;

struct Shader;

struct Program;

struct ProgramInvocation;

struct Stage;

struct Scene;

enum ShaderType {
  ShaderType_Fragment = 0,
  ShaderType_Vertex = 1,
  ShaderType_Compute = 2,
  ShaderType_MIN = ShaderType_Fragment,
  ShaderType_MAX = ShaderType_Compute
};

inline const ShaderType (&EnumValuesShaderType())[3] {
  static const ShaderType values[] = {
    ShaderType_Fragment,
    ShaderType_Vertex,
    ShaderType_Compute
  };
  return values;
}

inline const char * const *EnumNamesShaderType() {
  static const char * const names[] = {
    "Fragment",
    "Vertex",
    "Compute",
    nullptr
  };
  return names;
}

inline const char *EnumNameShaderType(ShaderType e) {
  if (e < ShaderType_Fragment || e > ShaderType_Compute) return "";
  const size_t index = static_cast<size_t>(e);
  return EnumNamesShaderType()[index];
}

FLATBUFFERS_MANUALLY_ALIGNED_STRUCT(4) uvec3 FLATBUFFERS_FINAL_CLASS {
 private:
  uint32_t x_;
  uint32_t y_;
  uint32_t z_;

 public:
  uvec3() {
    memset(static_cast<void *>(this), 0, sizeof(uvec3));
  }
  uvec3(uint32_t _x, uint32_t _y, uint32_t _z)
      : x_(flatbuffers::EndianScalar(_x)),
        y_(flatbuffers::EndianScalar(_y)),
        z_(flatbuffers::EndianScalar(_z)) {
  }
  uint32_t x() const {
    return flatbuffers::EndianScalar(x_);
  }
  void mutate_x(uint32_t _x) {
    flatbuffers::WriteScalar(&x_, _x);
  }
  uint32_t y() const {
    return flatbuffers::EndianScalar(y_);
  }
  void mutate_y(uint32_t _y) {
    flatbuffers::WriteScalar(&y_, _y);
  }
  uint32_t z() const {
    return flatbuffers::EndianScalar(z_);
  }
  void mutate_z(uint32_t _z) {
    flatbuffers::WriteScalar(&z_, _z);
  }
};
FLATBUFFERS_STRUCT_END(uvec3, 12);

struct Buffer FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_ID = 4,
    VT_LENGTH = 6,
    VT_INITIAL = 8
  };
  uint32_t id() const {
    return GetField<uint32_t>(VT_ID, 0);
  }
  bool mutate_id(uint32_t _id) {
    return SetField<uint32_t>(VT_ID, _id, 0);
  }
  uint32_t length() const {
    return GetField<uint32_t>(VT_LENGTH, 0);
  }
  bool mutate_length(uint32_t _length) {
    return SetField<uint32_t>(VT_LENGTH, _length, 0);
  }
  const flatbuffers::Vector<uint8_t> *initial() const {
    return GetPointer<const flatbuffers::Vector<uint8_t> *>(VT_INITIAL);
  }
  flatbuffers::Vector<uint8_t> *mutable_initial() {
    return GetPointer<flatbuffers::Vector<uint8_t> *>(VT_INITIAL);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint32_t>(verifier, VT_ID) &&
           VerifyField<uint32_t>(verifier, VT_LENGTH) &&
           VerifyOffset(verifier, VT_INITIAL) &&
           verifier.VerifyVector(initial()) &&
           verifier.EndTable();
  }
};

struct BufferBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_id(uint32_t id) {
    fbb_.AddElement<uint32_t>(Buffer::VT_ID, id, 0);
  }
  void add_length(uint32_t length) {
    fbb_.AddElement<uint32_t>(Buffer::VT_LENGTH, length, 0);
  }
  void add_initial(flatbuffers::Offset<flatbuffers::Vector<uint8_t>> initial) {
    fbb_.AddOffset(Buffer::VT_INITIAL, initial);
  }
  explicit BufferBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  BufferBuilder &operator=(const BufferBuilder &);
  flatbuffers::Offset<Buffer> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Buffer>(end);
    return o;
  }
};

inline flatbuffers::Offset<Buffer> CreateBuffer(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t id = 0,
    uint32_t length = 0,
    flatbuffers::Offset<flatbuffers::Vector<uint8_t>> initial = 0) {
  BufferBuilder builder_(_fbb);
  builder_.add_initial(initial);
  builder_.add_length(length);
  builder_.add_id(id);
  return builder_.Finish();
}

inline flatbuffers::Offset<Buffer> CreateBufferDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    uint32_t id = 0,
    uint32_t length = 0,
    const std::vector<uint8_t> *initial = nullptr) {
  auto initial__ = initial ? _fbb.CreateVector<uint8_t>(*initial) : 0;
  return renderpipe::proto::CreateBuffer(
      _fbb,
      id,
      length,
      initial__);
}

struct Shader FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_TYPE = 4,
    VT_HASH = 6,
    VT_FILENAME = 8,
    VT_SOURCE = 10
  };
  ShaderType type() const {
    return static_cast<ShaderType>(GetField<uint8_t>(VT_TYPE, 0));
  }
  bool mutate_type(ShaderType _type) {
    return SetField<uint8_t>(VT_TYPE, static_cast<uint8_t>(_type), 0);
  }
  const flatbuffers::String *hash() const {
    return GetPointer<const flatbuffers::String *>(VT_HASH);
  }
  flatbuffers::String *mutable_hash() {
    return GetPointer<flatbuffers::String *>(VT_HASH);
  }
  const flatbuffers::String *filename() const {
    return GetPointer<const flatbuffers::String *>(VT_FILENAME);
  }
  flatbuffers::String *mutable_filename() {
    return GetPointer<flatbuffers::String *>(VT_FILENAME);
  }
  const flatbuffers::String *source() const {
    return GetPointer<const flatbuffers::String *>(VT_SOURCE);
  }
  flatbuffers::String *mutable_source() {
    return GetPointer<flatbuffers::String *>(VT_SOURCE);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<uint8_t>(verifier, VT_TYPE) &&
           VerifyOffset(verifier, VT_HASH) &&
           verifier.VerifyString(hash()) &&
           VerifyOffset(verifier, VT_FILENAME) &&
           verifier.VerifyString(filename()) &&
           VerifyOffset(verifier, VT_SOURCE) &&
           verifier.VerifyString(source()) &&
           verifier.EndTable();
  }
};

struct ShaderBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_type(ShaderType type) {
    fbb_.AddElement<uint8_t>(Shader::VT_TYPE, static_cast<uint8_t>(type), 0);
  }
  void add_hash(flatbuffers::Offset<flatbuffers::String> hash) {
    fbb_.AddOffset(Shader::VT_HASH, hash);
  }
  void add_filename(flatbuffers::Offset<flatbuffers::String> filename) {
    fbb_.AddOffset(Shader::VT_FILENAME, filename);
  }
  void add_source(flatbuffers::Offset<flatbuffers::String> source) {
    fbb_.AddOffset(Shader::VT_SOURCE, source);
  }
  explicit ShaderBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ShaderBuilder &operator=(const ShaderBuilder &);
  flatbuffers::Offset<Shader> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Shader>(end);
    return o;
  }
};

inline flatbuffers::Offset<Shader> CreateShader(
    flatbuffers::FlatBufferBuilder &_fbb,
    ShaderType type = ShaderType_Fragment,
    flatbuffers::Offset<flatbuffers::String> hash = 0,
    flatbuffers::Offset<flatbuffers::String> filename = 0,
    flatbuffers::Offset<flatbuffers::String> source = 0) {
  ShaderBuilder builder_(_fbb);
  builder_.add_source(source);
  builder_.add_filename(filename);
  builder_.add_hash(hash);
  builder_.add_type(type);
  return builder_.Finish();
}

inline flatbuffers::Offset<Shader> CreateShaderDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    ShaderType type = ShaderType_Fragment,
    const char *hash = nullptr,
    const char *filename = nullptr,
    const char *source = nullptr) {
  auto hash__ = hash ? _fbb.CreateString(hash) : 0;
  auto filename__ = filename ? _fbb.CreateString(filename) : 0;
  auto source__ = source ? _fbb.CreateString(source) : 0;
  return renderpipe::proto::CreateShader(
      _fbb,
      type,
      hash__,
      filename__,
      source__);
}

struct Program FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_TYPE = 4,
    VT_HASH = 6,
    VT_SHADERS = 8
  };
  const flatbuffers::String *type() const {
    return GetPointer<const flatbuffers::String *>(VT_TYPE);
  }
  flatbuffers::String *mutable_type() {
    return GetPointer<flatbuffers::String *>(VT_TYPE);
  }
  const flatbuffers::String *hash() const {
    return GetPointer<const flatbuffers::String *>(VT_HASH);
  }
  flatbuffers::String *mutable_hash() {
    return GetPointer<flatbuffers::String *>(VT_HASH);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Shader>> *shaders() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Shader>> *>(VT_SHADERS);
  }
  flatbuffers::Vector<flatbuffers::Offset<Shader>> *mutable_shaders() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<Shader>> *>(VT_SHADERS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_TYPE) &&
           verifier.VerifyString(type()) &&
           VerifyOffset(verifier, VT_HASH) &&
           verifier.VerifyString(hash()) &&
           VerifyOffset(verifier, VT_SHADERS) &&
           verifier.VerifyVector(shaders()) &&
           verifier.VerifyVectorOfTables(shaders()) &&
           verifier.EndTable();
  }
};

struct ProgramBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_type(flatbuffers::Offset<flatbuffers::String> type) {
    fbb_.AddOffset(Program::VT_TYPE, type);
  }
  void add_hash(flatbuffers::Offset<flatbuffers::String> hash) {
    fbb_.AddOffset(Program::VT_HASH, hash);
  }
  void add_shaders(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Shader>>> shaders) {
    fbb_.AddOffset(Program::VT_SHADERS, shaders);
  }
  explicit ProgramBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ProgramBuilder &operator=(const ProgramBuilder &);
  flatbuffers::Offset<Program> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Program>(end);
    return o;
  }
};

inline flatbuffers::Offset<Program> CreateProgram(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> type = 0,
    flatbuffers::Offset<flatbuffers::String> hash = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Shader>>> shaders = 0) {
  ProgramBuilder builder_(_fbb);
  builder_.add_shaders(shaders);
  builder_.add_hash(hash);
  builder_.add_type(type);
  return builder_.Finish();
}

inline flatbuffers::Offset<Program> CreateProgramDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *type = nullptr,
    const char *hash = nullptr,
    const std::vector<flatbuffers::Offset<Shader>> *shaders = nullptr) {
  auto type__ = type ? _fbb.CreateString(type) : 0;
  auto hash__ = hash ? _fbb.CreateString(hash) : 0;
  auto shaders__ = shaders ? _fbb.CreateVector<flatbuffers::Offset<Shader>>(*shaders) : 0;
  return renderpipe::proto::CreateProgram(
      _fbb,
      type__,
      hash__,
      shaders__);
}

struct ProgramInvocation FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_PROGRAM = 4,
    VT_DIMS = 6
  };
  const flatbuffers::String *program() const {
    return GetPointer<const flatbuffers::String *>(VT_PROGRAM);
  }
  flatbuffers::String *mutable_program() {
    return GetPointer<flatbuffers::String *>(VT_PROGRAM);
  }
  const uvec3 *dims() const {
    return GetStruct<const uvec3 *>(VT_DIMS);
  }
  uvec3 *mutable_dims() {
    return GetStruct<uvec3 *>(VT_DIMS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_PROGRAM) &&
           verifier.VerifyString(program()) &&
           VerifyField<uvec3>(verifier, VT_DIMS) &&
           verifier.EndTable();
  }
};

struct ProgramInvocationBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_program(flatbuffers::Offset<flatbuffers::String> program) {
    fbb_.AddOffset(ProgramInvocation::VT_PROGRAM, program);
  }
  void add_dims(const uvec3 *dims) {
    fbb_.AddStruct(ProgramInvocation::VT_DIMS, dims);
  }
  explicit ProgramInvocationBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  ProgramInvocationBuilder &operator=(const ProgramInvocationBuilder &);
  flatbuffers::Offset<ProgramInvocation> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<ProgramInvocation>(end);
    return o;
  }
};

inline flatbuffers::Offset<ProgramInvocation> CreateProgramInvocation(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> program = 0,
    const uvec3 *dims = 0) {
  ProgramInvocationBuilder builder_(_fbb);
  builder_.add_dims(dims);
  builder_.add_program(program);
  return builder_.Finish();
}

inline flatbuffers::Offset<ProgramInvocation> CreateProgramInvocationDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *program = nullptr,
    const uvec3 *dims = 0) {
  auto program__ = program ? _fbb.CreateString(program) : 0;
  return renderpipe::proto::CreateProgramInvocation(
      _fbb,
      program__,
      dims);
}

struct Stage FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_INVOCATIONS = 6
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  flatbuffers::String *mutable_name() {
    return GetPointer<flatbuffers::String *>(VT_NAME);
  }
  const flatbuffers::Vector<flatbuffers::Offset<ProgramInvocation>> *invocations() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<ProgramInvocation>> *>(VT_INVOCATIONS);
  }
  flatbuffers::Vector<flatbuffers::Offset<ProgramInvocation>> *mutable_invocations() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<ProgramInvocation>> *>(VT_INVOCATIONS);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyOffset(verifier, VT_INVOCATIONS) &&
           verifier.VerifyVector(invocations()) &&
           verifier.VerifyVectorOfTables(invocations()) &&
           verifier.EndTable();
  }
};

struct StageBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Stage::VT_NAME, name);
  }
  void add_invocations(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ProgramInvocation>>> invocations) {
    fbb_.AddOffset(Stage::VT_INVOCATIONS, invocations);
  }
  explicit StageBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  StageBuilder &operator=(const StageBuilder &);
  flatbuffers::Offset<Stage> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Stage>(end);
    return o;
  }
};

inline flatbuffers::Offset<Stage> CreateStage(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<ProgramInvocation>>> invocations = 0) {
  StageBuilder builder_(_fbb);
  builder_.add_invocations(invocations);
  builder_.add_name(name);
  return builder_.Finish();
}

inline flatbuffers::Offset<Stage> CreateStageDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    const std::vector<flatbuffers::Offset<ProgramInvocation>> *invocations = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto invocations__ = invocations ? _fbb.CreateVector<flatbuffers::Offset<ProgramInvocation>>(*invocations) : 0;
  return renderpipe::proto::CreateStage(
      _fbb,
      name__,
      invocations__);
}

struct Scene FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  enum FlatBuffersVTableOffset FLATBUFFERS_VTABLE_UNDERLYING_TYPE {
    VT_NAME = 4,
    VT_BUFFERS = 6,
    VT_PROGRAMS = 8,
    VT_STAGES = 10
  };
  const flatbuffers::String *name() const {
    return GetPointer<const flatbuffers::String *>(VT_NAME);
  }
  flatbuffers::String *mutable_name() {
    return GetPointer<flatbuffers::String *>(VT_NAME);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Buffer>> *buffers() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Buffer>> *>(VT_BUFFERS);
  }
  flatbuffers::Vector<flatbuffers::Offset<Buffer>> *mutable_buffers() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<Buffer>> *>(VT_BUFFERS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Program>> *programs() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Program>> *>(VT_PROGRAMS);
  }
  flatbuffers::Vector<flatbuffers::Offset<Program>> *mutable_programs() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<Program>> *>(VT_PROGRAMS);
  }
  const flatbuffers::Vector<flatbuffers::Offset<Stage>> *stages() const {
    return GetPointer<const flatbuffers::Vector<flatbuffers::Offset<Stage>> *>(VT_STAGES);
  }
  flatbuffers::Vector<flatbuffers::Offset<Stage>> *mutable_stages() {
    return GetPointer<flatbuffers::Vector<flatbuffers::Offset<Stage>> *>(VT_STAGES);
  }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyOffset(verifier, VT_NAME) &&
           verifier.VerifyString(name()) &&
           VerifyOffset(verifier, VT_BUFFERS) &&
           verifier.VerifyVector(buffers()) &&
           verifier.VerifyVectorOfTables(buffers()) &&
           VerifyOffset(verifier, VT_PROGRAMS) &&
           verifier.VerifyVector(programs()) &&
           verifier.VerifyVectorOfTables(programs()) &&
           VerifyOffset(verifier, VT_STAGES) &&
           verifier.VerifyVector(stages()) &&
           verifier.VerifyVectorOfTables(stages()) &&
           verifier.EndTable();
  }
};

struct SceneBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_name(flatbuffers::Offset<flatbuffers::String> name) {
    fbb_.AddOffset(Scene::VT_NAME, name);
  }
  void add_buffers(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Buffer>>> buffers) {
    fbb_.AddOffset(Scene::VT_BUFFERS, buffers);
  }
  void add_programs(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Program>>> programs) {
    fbb_.AddOffset(Scene::VT_PROGRAMS, programs);
  }
  void add_stages(flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Stage>>> stages) {
    fbb_.AddOffset(Scene::VT_STAGES, stages);
  }
  explicit SceneBuilder(flatbuffers::FlatBufferBuilder &_fbb)
        : fbb_(_fbb) {
    start_ = fbb_.StartTable();
  }
  SceneBuilder &operator=(const SceneBuilder &);
  flatbuffers::Offset<Scene> Finish() {
    const auto end = fbb_.EndTable(start_);
    auto o = flatbuffers::Offset<Scene>(end);
    return o;
  }
};

inline flatbuffers::Offset<Scene> CreateScene(
    flatbuffers::FlatBufferBuilder &_fbb,
    flatbuffers::Offset<flatbuffers::String> name = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Buffer>>> buffers = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Program>>> programs = 0,
    flatbuffers::Offset<flatbuffers::Vector<flatbuffers::Offset<Stage>>> stages = 0) {
  SceneBuilder builder_(_fbb);
  builder_.add_stages(stages);
  builder_.add_programs(programs);
  builder_.add_buffers(buffers);
  builder_.add_name(name);
  return builder_.Finish();
}

inline flatbuffers::Offset<Scene> CreateSceneDirect(
    flatbuffers::FlatBufferBuilder &_fbb,
    const char *name = nullptr,
    const std::vector<flatbuffers::Offset<Buffer>> *buffers = nullptr,
    const std::vector<flatbuffers::Offset<Program>> *programs = nullptr,
    const std::vector<flatbuffers::Offset<Stage>> *stages = nullptr) {
  auto name__ = name ? _fbb.CreateString(name) : 0;
  auto buffers__ = buffers ? _fbb.CreateVector<flatbuffers::Offset<Buffer>>(*buffers) : 0;
  auto programs__ = programs ? _fbb.CreateVector<flatbuffers::Offset<Program>>(*programs) : 0;
  auto stages__ = stages ? _fbb.CreateVector<flatbuffers::Offset<Stage>>(*stages) : 0;
  return renderpipe::proto::CreateScene(
      _fbb,
      name__,
      buffers__,
      programs__,
      stages__);
}

inline const renderpipe::proto::Scene *GetScene(const void *buf) {
  return flatbuffers::GetRoot<renderpipe::proto::Scene>(buf);
}

inline const renderpipe::proto::Scene *GetSizePrefixedScene(const void *buf) {
  return flatbuffers::GetSizePrefixedRoot<renderpipe::proto::Scene>(buf);
}

inline Scene *GetMutableScene(void *buf) {
  return flatbuffers::GetMutableRoot<Scene>(buf);
}

inline bool VerifySceneBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifyBuffer<renderpipe::proto::Scene>(nullptr);
}

inline bool VerifySizePrefixedSceneBuffer(
    flatbuffers::Verifier &verifier) {
  return verifier.VerifySizePrefixedBuffer<renderpipe::proto::Scene>(nullptr);
}

inline void FinishSceneBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<renderpipe::proto::Scene> root) {
  fbb.Finish(root);
}

inline void FinishSizePrefixedSceneBuffer(
    flatbuffers::FlatBufferBuilder &fbb,
    flatbuffers::Offset<renderpipe::proto::Scene> root) {
  fbb.FinishSizePrefixed(root);
}

}  // namespace proto
}  // namespace renderpipe

#endif  // FLATBUFFERS_GENERATED_RENDERPIPE_RENDERPIPE_PROTO_H_