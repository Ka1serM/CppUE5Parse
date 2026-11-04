# SPIR-V to expression programs

This folder turns a SPIR-V shader into a small, vectorized instruction program that another system can rebuild, such
as a Blender node tree or a GLSL shader. Nothing here knows about Unreal: it needs only C++23 and the
[SPIRV-Headers](https://github.com/KhronosGroup/SPIRV-Headers) include directory.

```text
SPIR-V words ──evaluateSpirv──▶ ExpressionGraph ──ProgramWriter──▶ ExpressionProgram ──your code──▶ Blender nodes
                  ▲
           your ShaderInputs
```

`tests/SpirvProgramTests.cpp` is this whole path as a program that runs, with the shader written out by hand.

## Using it

With CMake, link `CppUE5Parse::Spirv` (it builds with `UE5PARSE_BUILD_CONVERSION`). To take the code on its own, copy
this folder to `<root>/Conversion/Spirv`, compile its `.cpp` files with `-std=c++23`, and put `<root>` and the
SPIRV-Headers `include` folder on the include path.

| Read this | For |
| --- | --- |
| `SpirvEvaluator.h` | `evaluateSpirv` and the `ShaderInputs` you implement |
| `ExpressionGraph.h` | The scalar expressions it produces, and the `GeometryAttribute` list |
| `ProgramWriter.h` | Turning expressions into a program |
| `ExpressionProgram.h` | The program: `ProgramOp`, `ProgramInstruction`, `ExpressionProgram` |

```cpp
ExpressionGraph graph;
MyInputs inputs(graph);
const ShaderOutputs outputs = evaluateSpirv(spirvWords, graph, inputs);
if (graph.isFailed())
    return *graph.getFailure();

const std::array<ExprId, 3> color{outputs.targets[0][0], outputs.targets[0][1], outputs.targets[0][2]};
ProgramWriter writer(graph, parameterWidths);
const uint32_t colorOutput = writer.write(std::span<const ExprId, 3>(color));
if (writer.getFailure())
    return *writer.getFailure();
const ExpressionProgram program = writer.finish();
const ProgramInstruction& result = program.instructions[program.outputs[colorOutput]];
```

## Connecting a shader's inputs

`ShaderInputs` says what each thing the shader reads means. Return an expression built on the graph you were given.

| Method | Called for | Usual answer |
| --- | --- | --- |
| `constantBuffer(binding, offset)` | A uniform, by `Binding` and byte offset | `graph.parameter(index, component)` or `graph.constant(value)` |
| `input(location, component)` | A stage input, by `Location` | `graph.geometry(attribute, component)` |
| `frontFacing()`, `fragCoord(component)` | Those built-ins | `graph.boolean(true)`, `graph.unresolved("frag coord")` |
| `sample(binding, coordinate)` | A texture sample | `graph.texture(slot, component, u, v)` for each of the 4 components |
| `fetch(binding, index)` | A buffer or texel fetch | Your data, or `graph.unresolved(name)` |
| `textureSize(binding, lod)` | A texture size query | `graph.constant(size)` for each axis |

`graph.unresolved(name)` stands for a value you can't express. It folds away if the shader doesn't really depend on it.
A value that still depends on it makes `ProgramWriter` fail, so check `graph.unresolvedInputs(id)` on your outputs
first to name what is missing.

`ShaderOutputs` gives `targets[location][component]`, `position` for a vertex shader, `discard` as a condition, and
`notes` about approximations that were made.

## What SPIR-V it accepts

- One fragment or vertex entry point. The module may have no other functions, so inline them first
  (`spirv-opt --inline-entry-points-exhaustive`).
- Structured control flow. Branches become selects, so both sides are evaluated.
- Loops run until their condition is a known constant, up to 1024 iterations. A loop that exits on a runtime value runs
  `maxUnknownLoopIterations` times and adds a note.
- Only the `GLSL.std.450` extended instructions.

Anything else fails the graph with a message. Check `graph.isFailed()` and `graph.getFailure()`.

## Reading a program

`instructions` is in dependency order: an operand's id is always lower than the id of the instruction using it, so one
pass from the start rebuilds everything, and an id can be read as a node's output socket. Unused instructions have been
removed. `outputs[slot]` is the instruction of the slot that `ProgramWriter::write` returned.

Each instruction has `op`, `width` (1 to 4 components of its result), `index`, and the operand ids `a`, `b`, `c`.

| `op` | Result |
| --- | --- |
| `Constant` | `width` floats from `constants[index]` on |
| `Parameter` | Parameter `index`, whose width you gave to `ProgramWriter` |
| `Texture` | RGBA of texture slot `index`, sampled at the vec2 `a` |
| `Geometry` | `GeometryAttribute` `index` |
| `GeometryDx`, `GeometryDy` | Its screen-space derivative |
| `Time`, `Frame` | Scene seconds, and the frame number |
| `Extract` | Component `index` of `a` |
| `Combine` | A vector of the scalars `a`, `b`, and for width 3, `c` |
| `Add`, `Sub`, `Mul`, `Div`, `Min`, `Max` | Per component, on `a` and `b` |
| `Pow` | `a` to the power `b`, and 0 where `a` is 0 or below |
| `Atan2` | `atan2(a, b)` |
| `Mod` | `a - b * floor(a / b)` |
| `Abs`, `Floor`, `Ceil`, `Fract`, `Trunc`, `Sign`, `Exp2`, `Log2`, `Sin`, `Cos`, `Tan`, `Atan` | Per component, on `a` |
| `Round` | Halves round to the nearest even number |
| `Sqrt` | Of `max(a, 0)` |
| `Asin`, `Acos` | Of `a` clamped to -1 to 1 |
| `Random` | One float that is a hash of `a` |
| `Less`, `LessEqual`, `Equal`, `NotEqual` | 1 or 0, comparing `a` to `b` |
| `Select` | `b` where `a` isn't 0, otherwise `c` |
| `Dot`, `Distance` | One float, from the vectors `a` and `b` |
| `Length` | One float, from the vector `a` |
| `Normalize` | `a` with length 1, or 0 for a zero vector |
| `Cross` | `a × b`, width 3 |
| `Reflect` | `a` reflected about the normal `b` |
| `TangentToWorld` | The tangent-space vec3 `a` in world space, normalized |
| `Clamp` | `min(max(a, b), c)` |
| `Mix` | `a + (b - a) * c` |
| `Smoothstep` | Value `a`, between edges `b` and `c` |
| `MulAdd` | `a * b + c` |

An operand can be narrower than the instruction: a width-1 operand of a wider instruction applies to every component,
as in `Mul` of a vec3 by a scalar. For Blender, `Mul` and the like map to a Vector Math node when `width` is above 1
and a Math node otherwise, and `Combine` and `Extract` to Combine XYZ and Separate XYZ.

`ProgramWriter` reads scalars and vec3s: write a vec4 target as a vec3 and a scalar. It finds the vector forms behind
the scalar expressions, including `Dot`, `Cross`, `Normalize`, `Clamp`, `Smoothstep`, and `TangentToWorld`, and
vectorizes lanes of the same operation.

## Files

| File | Contents |
| --- | --- |
| `SpirvEvaluator.cpp` | The entry point, and the outputs after a run |
| `SpirvInterpreter.h` | The interpreter's state (private to this folder) |
| `SpirvModule.cpp` | Parsing: types, constants, decorations, variables |
| `SpirvControlFlow.cpp` | Blocks, branches, phis, loops |
| `SpirvInstructions.cpp` | Each instruction, one case at a time |
| `SpirvGlsl.cpp` | `GLSL.std.450` |
| `SpirvIntegers.cpp` | Integer and bit arithmetic on float expressions |
| `SpirvMemory.cpp` | Variables, access chains, loads, stores |
| `ExpressionGraph.cpp` | Node interning, constant folding, simplification |
| `ProgramWriter.cpp` | Emitting instructions, lanes, and the final cleanup |
| `ProgramPatterns.cpp` | Finding vector forms in scalar lanes |
| `ProgramSimplification.cpp` | Simplifying an instruction before it is added |

`Conversion/Materials` is the Unreal user of this folder, and `MaterialEvaluator.cpp` is a full `ShaderInputs`.
