#include "native_runtime.h"
#include "native_memory.h"

#include <bit>
#include <cmath>

#if defined(__clang__)
#pragma STDC FENV_ACCESS ON
#endif

namespace {
template <uint32_t Value>
struct Absolute {
  static uint32_t Address(const GuestContext&) { return Value; }
};

template <GuestRegister GuestContext::*Register, int32_t Offset>
struct Relative {
  static uint32_t Address(const GuestContext& ctx) {
    return (ctx.*Register).u32 + uint32_t(Offset);
  }
};

template <GuestRegister GuestContext::*Register, int64_t Value>
struct Scratch {
  static void Apply(GuestContext& ctx) { (ctx.*Register).s64 = Value; }
};

template <typename... Registers>
struct ScratchSet {
  static void Apply(GuestContext& ctx) { (Registers::Apply(ctx), ...); }
};

inline bool FiniteSingle(uint32_t bits) {
  return (bits & 0x7F800000u) != 0x7F800000u;
}

inline void StoreSingle(uint8_t* base, uint32_t address, double value) {
  GH_STORE_U32(address, std::bit_cast<uint32_t>(float(value)));
}

enum class BinaryOperation { Add, Subtract, Multiply };

// A finite f32 product fits in f64, and f32 sums cannot double-round at f64
// precision. Use the single operation and preserve the double register image.
template <BinaryOperation Operation, typename Left, typename Right,
          typename Destination, typename Registers>
inline void Binary(GuestContext& ctx, uint8_t* base, GuestFunction* original) {
  const uint32_t left = Left::Address(ctx), right = Right::Address(ctx);
  const uint32_t destination = Destination::Address(ctx);
  if (!NativeMemoryRange(left, 4) || !NativeMemoryRange(right, 4) ||
      !NativeMemoryRange(destination, 4)) {
    original(ctx, base);
    return;
  }
  const uint32_t left_bits = GH_LOAD_U32(left);
  const uint32_t right_bits = GH_LOAD_U32(right);
  if (!FiniteSingle(left_bits) || !FiniteSingle(right_bits)) {
    original(ctx, base);
    return;
  }
  ctx.fpscr.disableFlushMode();
  const float a = std::bit_cast<float>(left_bits);
  const float b = std::bit_cast<float>(right_bits);
  float result;
  if constexpr (Operation == BinaryOperation::Add) result = a + b;
  else if constexpr (Operation == BinaryOperation::Subtract) result = a - b;
  else result = a * b;
  ctx.f0.f64 = double(result);
  ctx.f13.f64 = double(b);
  GH_STORE_U32(destination, std::bit_cast<uint32_t>(result));
  Registers::Apply(ctx);
}

template <typename Left, typename Right, typename Addend,
          typename Destination, typename Registers>
inline void MultiplyAdd(GuestContext& ctx, uint8_t* base, GuestFunction* original) {
  const uint32_t left = Left::Address(ctx), right = Right::Address(ctx);
  const uint32_t addend = Addend::Address(ctx), destination = Destination::Address(ctx);
  if (!NativeMemoryRange(left, 4) || !NativeMemoryRange(right, 4) ||
      !NativeMemoryRange(addend, 4) || !NativeMemoryRange(destination, 4)) {
    original(ctx, base);
    return;
  }
  const uint32_t a_bits = GH_LOAD_U32(left), b_bits = GH_LOAD_U32(right);
  const uint32_t c_bits = GH_LOAD_U32(addend);
  if (!FiniteSingle(a_bits) || !FiniteSingle(b_bits) || !FiniteSingle(c_bits)) {
    original(ctx, base);
    return;
  }
  ctx.fpscr.disableFlushMode();
  const double a = double(std::bit_cast<float>(a_bits));
  const double b = double(std::bit_cast<float>(b_bits));
  const double c = double(std::bit_cast<float>(c_bits));
  const double result = double(float(std::fma(a, b, c)));
  ctx.f0.f64 = result;
  ctx.f13.f64 = b;
  ctx.f12.f64 = c;
  StoreSingle(base, destination, result);
  Registers::Apply(ctx);
}

template <GuestRegister GuestContext::*Register, bool Negate, typename Source,
          typename Registers, typename... Destinations>
inline void Load(GuestContext& ctx, uint8_t* base, GuestFunction* original) {
  const uint32_t source = Source::Address(ctx);
  if (!NativeMemoryRange(source, 4) ||
      !(NativeMemoryRange(Destinations::Address(ctx), 4) && ...)) {
    original(ctx, base);
    return;
  }
  const uint32_t bits = GH_LOAD_U32(source);
  if (!FiniteSingle(bits)) {
    original(ctx, base);
    return;
  }
  ctx.fpscr.disableFlushMode();
  double value = double(std::bit_cast<float>(bits));
  if constexpr (Negate)
    value = std::bit_cast<double>(std::bit_cast<uint64_t>(value) ^ 0x8000000000000000ull);
  (ctx.*Register).f64 = value;
  (StoreSingle(base, Destinations::Address(ctx), value), ...);
  Registers::Apply(ctx);
}

template <GuestRegister GuestContext::*Register, typename Destination>
struct Output {
  static bool Ordinary(const GuestContext& ctx) {
    return NativeMemoryRange(Destination::Address(ctx), 4);
  }
  static bool Finite(const GuestContext& ctx) {
    return ((ctx.*Register).u64 & 0x7FF0000000000000ull) != 0x7FF0000000000000ull;
  }
  static void Store(GuestContext& ctx, uint8_t* base) {
    StoreSingle(base, Destination::Address(ctx), (ctx.*Register).f64);
  }
};

template <typename... Outputs>
inline void Store(GuestContext& ctx, uint8_t* base, GuestFunction* original) {
  if (!(Outputs::Ordinary(ctx) && ...) || !(Outputs::Finite(ctx) && ...)) {
    original(ctx, base);
    return;
  }
  ctx.fpscr.disableFlushMode();
  (Outputs::Store(ctx, base), ...);
}
}  // namespace

GH_EXTERN(__imp__sub_8262AE58);
GH_HOOK(sub_8262AE58) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82374998u>, ScratchSet<Scratch<&GuestContext::r11, -2110324736ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AE58);
}

GH_EXTERN(__imp__sub_8262AE40);
GH_HOOK(sub_8262AE40) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x8237499Cu>, ScratchSet<Scratch<&GuestContext::r11, -2110324736ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AE40);
}

GH_EXTERN(__imp__sub_826F15E8);
GH_HOOK(sub_826F15E8) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Relative<&GuestContext::r3, 576>, ScratchSet<>>(ctx, base, __imp__sub_826F15E8);
}

GH_EXTERN(__imp__sub_82902D60);
GH_HOOK(sub_82902D60) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922540u>, Absolute<0x82334CD4u>, Absolute<0x829CD1E8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902D60);
}

GH_EXTERN(__imp__sub_82903118);
GH_HOOK(sub_82903118) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922D30u>, Absolute<0x8233C34Cu>, Absolute<0x829CD254u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903118);
}

GH_EXTERN(__imp__sub_828ED908);
GH_HOOK(sub_828ED908) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829123B0u>, Absolute<0x82000E5Cu>, Absolute<0x829AB3CCu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED908);
}

GH_EXTERN(__imp__sub_82900418);
GH_HOOK(sub_82900418) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913C64u>, Absolute<0x82016F1Cu>, Absolute<0x829B212Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900418);
}

GH_EXTERN(__imp__sub_82900558);
GH_HOOK(sub_82900558) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82913C84u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829B2154u>>(ctx, base, __imp__sub_82900558);
}

GH_EXTERN(__imp__sub_82900628);
GH_HOOK(sub_82900628) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82913C94u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829B2174u>>(ctx, base, __imp__sub_82900628);
}

GH_EXTERN(__imp__sub_82903158);
GH_HOOK(sub_82903158) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922D70u>, Absolute<0x8233C410u>, Absolute<0x829CD25Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903158);
}

GH_EXTERN(__imp__sub_82903570);
GH_HOOK(sub_82903570) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923390u>, Absolute<0x8233F038u>, Absolute<0x829CD2D4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903570);
}

GH_EXTERN(__imp__sub_8262AF00);
GH_HOOK(sub_8262AF00) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000B48u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AF00);
}

GH_EXTERN(__imp__sub_828ED8B0);
GH_HOOK(sub_828ED8B0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829121C8u>, Absolute<0x82000E5Cu>, Absolute<0x829AB3C0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED8B0);
}

GH_EXTERN(__imp__sub_82900CB0);
GH_HOOK(sub_82900CB0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8291403Cu>, Absolute<0x82018DC8u>, Absolute<0x829CCC40u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900CB0);
}

GH_EXTERN(__imp__sub_82900EA8);
GH_HOOK(sub_82900EA8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82914264u>, Absolute<0x82000E5Cu>, Absolute<0x829CCC8Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900EA8);
}

GH_EXTERN(__imp__sub_829019E8);
GH_HOOK(sub_829019E8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000AE8u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE6Cu>>(ctx, base, __imp__sub_829019E8);
}

GH_EXTERN(__imp__sub_82901BE8);
GH_HOOK(sub_82901BE8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921478u>, Absolute<0x820183ACu>, Absolute<0x829CCEBCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901BE8);
}

GH_EXTERN(__imp__sub_82902298);
GH_HOOK(sub_82902298) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829214DCu>, Absolute<0x82088FDCu>, Absolute<0x829CD0A0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902298);
}

GH_EXTERN(__imp__sub_829027A0);
GH_HOOK(sub_829027A0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829218D4u>, Absolute<0x82328804u>, Absolute<0x829CD124u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829027A0);
}

GH_EXTERN(__imp__sub_82902AC0);
GH_HOOK(sub_82902AC0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921FC8u>, Absolute<0x8232FEC4u>, Absolute<0x829CD188u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902AC0);
}

GH_EXTERN(__imp__sub_82902E20);
GH_HOOK(sub_82902E20) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829226B4u>, Absolute<0x82336764u>, Absolute<0x829CD200u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902E20);
}

GH_EXTERN(__imp__sub_82903218);
GH_HOOK(sub_82903218) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922EA4u>, Absolute<0x8233CB64u>, Absolute<0x829CD274u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903218);
}

GH_EXTERN(__imp__sub_82903650);
GH_HOOK(sub_82903650) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829234F0u>, Absolute<0x8233F900u>, Absolute<0x829CD2F0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903650);
}

GH_EXTERN(__imp__sub_82703788);
GH_HOOK(sub_82703788) {
  GH_PROLOGUE();
  Store<Output<&GuestContext::f1, Relative<&GuestContext::r3, 80>>>(ctx, base, __imp__sub_82703788);
}

GH_EXTERN(__imp__sub_828ED988);
GH_HOOK(sub_828ED988) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829124F0u>, Absolute<0x82011668u>, Absolute<0x829AB3DCu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED988);
}

GH_EXTERN(__imp__sub_829019D0);
GH_HOOK(sub_829019D0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000AE4u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE68u>>(ctx, base, __imp__sub_829019D0);
}

GH_EXTERN(__imp__sub_82901BC8);
GH_HOOK(sub_82901BC8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921474u>, Absolute<0x82016F1Cu>, Absolute<0x829CCEB8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901BC8);
}

GH_EXTERN(__imp__sub_82901E78);
GH_HOOK(sub_82901E78) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829214C0u>, Absolute<0x82018BACu>, Absolute<0x829CCF14u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901E78);
}

GH_EXTERN(__imp__sub_82902780);
GH_HOOK(sub_82902780) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921818u>, Absolute<0x823280D4u>, Absolute<0x829CD120u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902780);
}

GH_EXTERN(__imp__sub_82902AA0);
GH_HOOK(sub_82902AA0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921FC8u>, Absolute<0x8232FEC4u>, Absolute<0x829CD184u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902AA0);
}

GH_EXTERN(__imp__sub_82902E00);
GH_HOOK(sub_82902E00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922638u>, Absolute<0x8233609Cu>, Absolute<0x829CD1FCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902E00);
}

GH_EXTERN(__imp__sub_829031F8);
GH_HOOK(sub_829031F8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922EA4u>, Absolute<0x8233CB64u>, Absolute<0x829CD270u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829031F8);
}

GH_EXTERN(__imp__sub_82903630);
GH_HOOK(sub_82903630) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829234B0u>, Absolute<0x8233F828u>, Absolute<0x829CD2ECu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903630);
}

GH_EXTERN(__imp__sub_82418D38);
GH_HOOK(sub_82418D38) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000B48u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>, Relative<&GuestContext::r3, 4>, Relative<&GuestContext::r3, 0>>(ctx, base, __imp__sub_82418D38);
}

GH_EXTERN(__imp__sub_82902A00);
GH_HOOK(sub_82902A00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921E44u>, Absolute<0x8232F0B4u>, Absolute<0x829CD170u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902A00);
}

GH_EXTERN(__imp__sub_82902D40);
GH_HOOK(sub_82902D40) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829224C4u>, Absolute<0x82334854u>, Absolute<0x829CD1E4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902D40);
}

GH_EXTERN(__imp__sub_829030F8);
GH_HOOK(sub_829030F8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922D30u>, Absolute<0x8233C34Cu>, Absolute<0x829CD250u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829030F8);
}

GH_EXTERN(__imp__sub_825231D0);
GH_HOOK(sub_825231D0) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Absolute<0x82000DA8u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>>(ctx, base, __imp__sub_825231D0);
}

GH_EXTERN(__imp__sub_8262AEE8);
GH_HOOK(sub_8262AEE8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x8237498Cu>, ScratchSet<Scratch<&GuestContext::r11, -2110324736ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AEE8);
}

GH_EXTERN(__imp__sub_82903828);
GH_HOOK(sub_82903828) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923788u>, Absolute<0x82340ED4u>, Absolute<0x829CD320u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903828);
}

GH_EXTERN(__imp__sub_828ED928);
GH_HOOK(sub_828ED928) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829124A0u>, Absolute<0x82000E5Cu>, Absolute<0x829AB3D0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED928);
}

GH_EXTERN(__imp__sub_8262AED0);
GH_HOOK(sub_8262AED0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x8234EB34u>, ScratchSet<Scratch<&GuestContext::r11, -2110455808ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AED0);
}

GH_EXTERN(__imp__sub_828ED828);
GH_HOOK(sub_828ED828) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82912058u>, Absolute<0x8200DEB0u>, Absolute<0x829AB3B0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED828);
}

GH_EXTERN(__imp__sub_82900498);
GH_HOOK(sub_82900498) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829B2134u>, Absolute<0x820024FCu>, Absolute<0x829B213Cu>, ScratchSet<Scratch<&GuestContext::r11, -2103771136ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900498);
}

GH_EXTERN(__imp__sub_82900640);
GH_HOOK(sub_82900640) {
  GH_PROLOGUE();
  MultiplyAdd<Absolute<0x82913C60u>, Absolute<0x82016F24u>, Absolute<0x829B2170u>, Absolute<0x829B2178u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>, Scratch<&GuestContext::r8, -2103771136ll>>>(ctx, base, __imp__sub_82900640);
}

GH_EXTERN(__imp__sub_82900B18);
GH_HOOK(sub_82900B18) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913E38u>, Absolute<0x82018BACu>, Absolute<0x829CC9A4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900B18);
}

GH_EXTERN(__imp__sub_82900D10);
GH_HOOK(sub_82900D10) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914074u>, Absolute<0x82018E24u>, Absolute<0x829CCC4Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900D10);
}

GH_EXTERN(__imp__sub_82900F08);
GH_HOOK(sub_82900F08) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914314u>, Absolute<0x82019700u>, Absolute<0x829CCC98u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900F08);
}

GH_EXTERN(__imp__sub_82901B00);
GH_HOOK(sub_82901B00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921414u>, Absolute<0x82088008u>, Absolute<0x829CCE94u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901B00);
}

GH_EXTERN(__imp__sub_82901DD8);
GH_HOOK(sub_82901DD8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x829214A4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCF00u>>(ctx, base, __imp__sub_82901DD8);
}

GH_EXTERN(__imp__sub_829026E0);
GH_HOOK(sub_829026E0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921770u>, Absolute<0x82089880u>, Absolute<0x829CD10Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829026E0);
}

GH_EXTERN(__imp__sub_829029E0);
GH_HOOK(sub_829029E0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921E44u>, Absolute<0x8232F0B4u>, Absolute<0x829CD16Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829029E0);
}

GH_EXTERN(__imp__sub_82902D20);
GH_HOOK(sub_82902D20) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829224C4u>, Absolute<0x82334854u>, Absolute<0x829CD1E0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902D20);
}

GH_EXTERN(__imp__sub_829030D8);
GH_HOOK(sub_829030D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922CB4u>, Absolute<0x8233B3FCu>, Absolute<0x829CD24Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829030D8);
}

GH_EXTERN(__imp__sub_82903480);
GH_HOOK(sub_82903480) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829232CCu>, Absolute<0x8233EEC4u>, Absolute<0x829CD2CCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903480);
}

GH_EXTERN(__imp__sub_8242A2C8);
GH_HOOK(sub_8242A2C8) {
  GH_PROLOGUE();
  Store<Output<&GuestContext::f1, Relative<&GuestContext::r3, 0>>, Output<&GuestContext::f2, Relative<&GuestContext::r3, 4>>, Output<&GuestContext::f3, Relative<&GuestContext::r3, 8>>>(ctx, base, __imp__sub_8242A2C8);
}

GH_EXTERN(__imp__sub_8262AEA0);
GH_HOOK(sub_8262AEA0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82374990u>, ScratchSet<Scratch<&GuestContext::r11, -2110324736ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AEA0);
}

GH_EXTERN(__imp__sub_82900BD0);
GH_HOOK(sub_82900BD0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913FE8u>, Absolute<0x82018D60u>, Absolute<0x829CCC24u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900BD0);
}

GH_EXTERN(__imp__sub_82900DD0);
GH_HOOK(sub_82900DD0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829140CCu>, Absolute<0x82018ED0u>, Absolute<0x829CCC64u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900DD0);
}

GH_EXTERN(__imp__sub_82900FE8);
GH_HOOK(sub_82900FE8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914520u>, Absolute<0x8201D998u>, Absolute<0x829CCCC4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900FE8);
}

GH_EXTERN(__imp__sub_82901470);
GH_HOOK(sub_82901470) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82920BACu>, Absolute<0x82000E5Cu>, Absolute<0x829CCE0Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901470);
}

GH_EXTERN(__imp__sub_829018B0);
GH_HOOK(sub_829018B0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000AE4u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE40u>>(ctx, base, __imp__sub_829018B0);
}

GH_EXTERN(__imp__sub_82901A40);
GH_HOOK(sub_82901A40) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921394u>, Absolute<0x82087F60u>, Absolute<0x829CCE78u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901A40);
}

GH_EXTERN(__imp__sub_82901D38);
GH_HOOK(sub_82901D38) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x8292149Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCEE8u>>(ctx, base, __imp__sub_82901D38);
}

GH_EXTERN(__imp__sub_82902660);
GH_HOOK(sub_82902660) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292173Cu>, Absolute<0x820896CCu>, Absolute<0x829CD0FCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902660);
}

GH_EXTERN(__imp__sub_82902960);
GH_HOOK(sub_82902960) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921D2Cu>, Absolute<0x8232DA44u>, Absolute<0x829CD15Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902960);
}

GH_EXTERN(__imp__sub_82902CA0);
GH_HOOK(sub_82902CA0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829223C8u>, Absolute<0x82333F18u>, Absolute<0x829CD1D0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902CA0);
}

GH_EXTERN(__imp__sub_82903058);
GH_HOOK(sub_82903058) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922B6Cu>, Absolute<0x82339974u>, Absolute<0x829CD23Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903058);
}

GH_EXTERN(__imp__sub_829033F8);
GH_HOOK(sub_829033F8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292320Cu>, Absolute<0x8233E384u>, Absolute<0x829CD2B0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829033F8);
}

GH_EXTERN(__imp__sub_829038C0);
GH_HOOK(sub_829038C0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292384Cu>, Absolute<0x82341084u>, Absolute<0x829CD328u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829038C0);
}

GH_EXTERN(__imp__sub_824A6CD0);
GH_HOOK(sub_824A6CD0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000B44u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>, Relative<&GuestContext::r3, 1352>>(ctx, base, __imp__sub_824A6CD0);
}

GH_EXTERN(__imp__sub_82900C30);
GH_HOOK(sub_82900C30) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914004u>, Absolute<0x82018D7Cu>, Absolute<0x829CCC30u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900C30);
}

GH_EXTERN(__imp__sub_82900E30);
GH_HOOK(sub_82900E30) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914100u>, Absolute<0x82018F30u>, Absolute<0x829CCC7Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900E30);
}

GH_EXTERN(__imp__sub_82901068);
GH_HOOK(sub_82901068) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914B30u>, Absolute<0x8201E068u>, Absolute<0x829CCCD4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901068);
}

GH_EXTERN(__imp__sub_829017C0);
GH_HOOK(sub_829017C0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000AE4u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE1Cu>>(ctx, base, __imp__sub_829017C0);
}

GH_EXTERN(__imp__sub_82901810);
GH_HOOK(sub_82901810) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8291088Cu>, Absolute<0x82013260u>, Absolute<0x829CCE28u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901810);
}

GH_EXTERN(__imp__sub_82901AE0);
GH_HOOK(sub_82901AE0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921414u>, Absolute<0x82088008u>, Absolute<0x829CCE90u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901AE0);
}

GH_EXTERN(__imp__sub_82901DC0);
GH_HOOK(sub_82901DC0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x829214A8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCEFCu>>(ctx, base, __imp__sub_82901DC0);
}

GH_EXTERN(__imp__sub_829026C0);
GH_HOOK(sub_829026C0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921754u>, Absolute<0x82089850u>, Absolute<0x829CD108u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829026C0);
}

GH_EXTERN(__imp__sub_829029C0);
GH_HOOK(sub_829029C0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921DA8u>, Absolute<0x8232DF9Cu>, Absolute<0x829CD168u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829029C0);
}

GH_EXTERN(__imp__sub_82902D00);
GH_HOOK(sub_82902D00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922408u>, Absolute<0x82333FE4u>, Absolute<0x829CD1DCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902D00);
}

GH_EXTERN(__imp__sub_829030B8);
GH_HOOK(sub_829030B8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922CB4u>, Absolute<0x8233B3FCu>, Absolute<0x829CD248u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829030B8);
}

GH_EXTERN(__imp__sub_82903460);
GH_HOOK(sub_82903460) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829232CCu>, Absolute<0x8233EEC4u>, Absolute<0x829CD2C8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903460);
}

GH_EXTERN(__imp__sub_82903900);
GH_HOOK(sub_82903900) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292388Cu>, Absolute<0x82341148u>, Absolute<0x829CD330u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903900);
}

GH_EXTERN(__imp__sub_828ED848);
GH_HOOK(sub_828ED848) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82912058u>, Absolute<0x8200DEB0u>, Absolute<0x829AB3B4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED848);
}

GH_EXTERN(__imp__sub_82902640);
GH_HOOK(sub_82902640) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921720u>, Absolute<0x820896B4u>, Absolute<0x829CD0F8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902640);
}

GH_EXTERN(__imp__sub_82902940);
GH_HOOK(sub_82902940) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921C70u>, Absolute<0x8232CB7Cu>, Absolute<0x829CD158u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902940);
}

GH_EXTERN(__imp__sub_82902C80);
GH_HOOK(sub_82902C80) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829223ACu>, Absolute<0x82333E2Cu>, Absolute<0x829CD1CCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902C80);
}

GH_EXTERN(__imp__sub_82903038);
GH_HOOK(sub_82903038) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922B6Cu>, Absolute<0x82339974u>, Absolute<0x829CD238u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903038);
}

GH_EXTERN(__imp__sub_829033D8);
GH_HOOK(sub_829033D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829231CCu>, Absolute<0x8233E2ACu>, Absolute<0x829CD2ACu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829033D8);
}

GH_EXTERN(__imp__sub_82901938);
GH_HOOK(sub_82901938) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000AE8u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE54u>>(ctx, base, __imp__sub_82901938);
}

GH_EXTERN(__imp__sub_82901E18);
GH_HOOK(sub_82901E18) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x829214A4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCF08u>>(ctx, base, __imp__sub_82901E18);
}

GH_EXTERN(__imp__sub_82902700);
GH_HOOK(sub_82902700) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921770u>, Absolute<0x82089880u>, Absolute<0x829CD110u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902700);
}

GH_EXTERN(__imp__sub_82902A20);
GH_HOOK(sub_82902A20) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921ED0u>, Absolute<0x8232F24Cu>, Absolute<0x829CD174u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902A20);
}

GH_EXTERN(__imp__sub_82902D80);
GH_HOOK(sub_82902D80) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922540u>, Absolute<0x82334CD4u>, Absolute<0x829CD1ECu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902D80);
}

GH_EXTERN(__imp__sub_82903138);
GH_HOOK(sub_82903138) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922D70u>, Absolute<0x8233C410u>, Absolute<0x829CD258u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903138);
}

GH_EXTERN(__imp__sub_82903550);
GH_HOOK(sub_82903550) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923390u>, Absolute<0x8233F038u>, Absolute<0x829CD2D0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903550);
}

GH_EXTERN(__imp__sub_828ED9A8);
GH_HOOK(sub_828ED9A8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, true, Absolute<0x829125BCu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829AB3E0u>>(ctx, base, __imp__sub_828ED9A8);
}

GH_EXTERN(__imp__sub_82901048);
GH_HOOK(sub_82901048) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914B30u>, Absolute<0x8201E068u>, Absolute<0x829CCCD0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901048);
}

GH_EXTERN(__imp__sub_829017A0);
GH_HOOK(sub_829017A0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829211B0u>, Absolute<0x82086778u>, Absolute<0x829CCE18u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829017A0);
}

GH_EXTERN(__imp__sub_82901900);
GH_HOOK(sub_82901900) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8291088Cu>, Absolute<0x82018650u>, Absolute<0x829CCE4Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901900);
}

GH_EXTERN(__imp__sub_82901C68);
GH_HOOK(sub_82901C68) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829CCEC4u>, Absolute<0x820146D0u>, Absolute<0x829CCECCu>, ScratchSet<Scratch<&GuestContext::r11, -2103640064ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901C68);
}

GH_EXTERN(__imp__sub_82902318);
GH_HOOK(sub_82902318) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292155Cu>, Absolute<0x82089200u>, Absolute<0x829CD0DCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902318);
}

GH_EXTERN(__imp__sub_82902820);
GH_HOOK(sub_82902820) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829219CCu>, Absolute<0x82329884u>, Absolute<0x829CD134u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902820);
}

GH_EXTERN(__imp__sub_82902B40);
GH_HOOK(sub_82902B40) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922110u>, Absolute<0x82330DACu>, Absolute<0x829CD198u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902B40);
}

GH_EXTERN(__imp__sub_82902EA0);
GH_HOOK(sub_82902EA0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829227ECu>, Absolute<0x82337254u>, Absolute<0x829CD210u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902EA0);
}

GH_EXTERN(__imp__sub_82903298);
GH_HOOK(sub_82903298) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922F9Cu>, Absolute<0x8233D1F4u>, Absolute<0x829CD284u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903298);
}

GH_EXTERN(__imp__sub_829036D0);
GH_HOOK(sub_829036D0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829235B4u>, Absolute<0x8233FDF8u>, Absolute<0x829CD300u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829036D0);
}

GH_EXTERN(__imp__sub_82418DF8);
GH_HOOK(sub_82418DF8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000B48u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>, Relative<&GuestContext::r3, 0>, Relative<&GuestContext::r3, 4>, Relative<&GuestContext::r3, 8>, Relative<&GuestContext::r3, 12>>(ctx, base, __imp__sub_82418DF8);
}

GH_EXTERN(__imp__sub_829031D8);
GH_HOOK(sub_829031D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922E64u>, Absolute<0x8233CA9Cu>, Absolute<0x829CD26Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829031D8);
}

GH_EXTERN(__imp__sub_82903610);
GH_HOOK(sub_82903610) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829234B0u>, Absolute<0x8233F828u>, Absolute<0x829CD2E8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903610);
}

GH_EXTERN(__imp__sub_82900B70);
GH_HOOK(sub_82900B70) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82913EA8u>, Absolute<0x82000E5Cu>, Absolute<0x829CC9B0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900B70);
}

GH_EXTERN(__imp__sub_82900D70);
GH_HOOK(sub_82900D70) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914090u>, Absolute<0x82018E3Cu>, Absolute<0x829CCC58u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900D70);
}

GH_EXTERN(__imp__sub_82900F68);
GH_HOOK(sub_82900F68) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914504u>, Absolute<0x8201A308u>, Absolute<0x829CCCB4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900F68);
}

GH_EXTERN(__imp__sub_829013F8);
GH_HOOK(sub_829013F8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82920A44u>, Absolute<0x820856C4u>, Absolute<0x829CCDFCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829013F8);
}

GH_EXTERN(__imp__sub_82901830);
GH_HOOK(sub_82901830) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829211C8u>, Absolute<0x82086D84u>, Absolute<0x829CCE2Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901830);
}

GH_EXTERN(__imp__sub_82901B20);
GH_HOOK(sub_82901B20) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921430u>, Absolute<0x82088034u>, Absolute<0x829CCE98u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901B20);
}

GH_EXTERN(__imp__sub_82901DF0);
GH_HOOK(sub_82901DF0) {
  GH_PROLOGUE();
  MultiplyAdd<Absolute<0x82921470u>, Absolute<0x82016F24u>, Absolute<0x829CCEFCu>, Absolute<0x829CCF04u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>, Scratch<&GuestContext::r8, -2103640064ll>>>(ctx, base, __imp__sub_82901DF0);
}

GH_EXTERN(__imp__sub_82902840);
GH_HOOK(sub_82902840) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829219CCu>, Absolute<0x82329884u>, Absolute<0x829CD138u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902840);
}

GH_EXTERN(__imp__sub_82902B60);
GH_HOOK(sub_82902B60) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292219Cu>, Absolute<0x823313BCu>, Absolute<0x829CD19Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902B60);
}

GH_EXTERN(__imp__sub_82902EC0);
GH_HOOK(sub_82902EC0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829227ECu>, Absolute<0x82337254u>, Absolute<0x829CD214u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902EC0);
}

GH_EXTERN(__imp__sub_829032B8);
GH_HOOK(sub_829032B8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922FDCu>, Absolute<0x8233D2BCu>, Absolute<0x829CD288u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829032B8);
}

GH_EXTERN(__imp__sub_829036F0);
GH_HOOK(sub_829036F0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829235B4u>, Absolute<0x8233FDF8u>, Absolute<0x829CD304u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829036F0);
}

GH_EXTERN(__imp__sub_824A69B0);
GH_HOOK(sub_824A69B0) {
  GH_PROLOGUE();
  Store<Output<&GuestContext::f1, Relative<&GuestContext::r3, 120>>, Output<&GuestContext::f2, Relative<&GuestContext::r3, 124>>>(ctx, base, __imp__sub_824A69B0);
}

GH_EXTERN(__imp__sub_828ED7E8);
GH_HOOK(sub_828ED7E8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8291203Cu>, Absolute<0x8200DE20u>, Absolute<0x829AAEE0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED7E8);
}

GH_EXTERN(__imp__sub_828ED9E0);
GH_HOOK(sub_828ED9E0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829126B0u>, Absolute<0x82000E5Cu>, Absolute<0x829AB3E8u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED9E0);
}

GH_EXTERN(__imp__sub_829019B0);
GH_HOOK(sub_829019B0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292137Cu>, Absolute<0x82087CF4u>, Absolute<0x829CCE64u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829019B0);
}

GH_EXTERN(__imp__sub_82901CE8);
GH_HOOK(sub_82901CE8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829CCEC4u>, Absolute<0x8200DE00u>, Absolute<0x829CCEDCu>, ScratchSet<Scratch<&GuestContext::r11, -2103640064ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901CE8);
}

GH_EXTERN(__imp__sub_82902620);
GH_HOOK(sub_82902620) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921720u>, Absolute<0x820896B4u>, Absolute<0x829CD0F4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902620);
}

GH_EXTERN(__imp__sub_82902920);
GH_HOOK(sub_82902920) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921C70u>, Absolute<0x8232CB7Cu>, Absolute<0x829CD154u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902920);
}

GH_EXTERN(__imp__sub_82902C60);
GH_HOOK(sub_82902C60) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829223ACu>, Absolute<0x82333E2Cu>, Absolute<0x829CD1C8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902C60);
}

GH_EXTERN(__imp__sub_82903018);
GH_HOOK(sub_82903018) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922AC0u>, Absolute<0x82338A04u>, Absolute<0x829CD234u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903018);
}

GH_EXTERN(__imp__sub_829033B8);
GH_HOOK(sub_829033B8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829231CCu>, Absolute<0x8233E2ACu>, Absolute<0x829CD2A8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829033B8);
}

GH_EXTERN(__imp__sub_82903848);
GH_HOOK(sub_82903848) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923788u>, Absolute<0x82340ED4u>, Absolute<0x829CD324u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903848);
}

GH_EXTERN(__imp__sub_8262A900);
GH_HOOK(sub_8262A900) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000B44u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262A900);
}

GH_EXTERN(__imp__sub_8262AEB8);
GH_HOOK(sub_8262AEB8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x8234EB44u>, ScratchSet<Scratch<&GuestContext::r11, -2110455808ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AEB8);
}

GH_EXTERN(__imp__sub_82703638);
GH_HOOK(sub_82703638) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Relative<&GuestContext::r3, 72>, ScratchSet<>>(ctx, base, __imp__sub_82703638);
}

GH_EXTERN(__imp__sub_82418DA8);
GH_HOOK(sub_82418DA8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000B48u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>, Relative<&GuestContext::r3, 12>, Relative<&GuestContext::r3, 8>, Relative<&GuestContext::r3, 4>, Relative<&GuestContext::r3, 0>>(ctx, base, __imp__sub_82418DA8);
}

GH_EXTERN(__imp__sub_829004D8);
GH_HOOK(sub_829004D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829B2134u>, Absolute<0x820024FCu>, Absolute<0x829B2144u>, ScratchSet<Scratch<&GuestContext::r11, -2103771136ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_829004D8);
}

GH_EXTERN(__imp__sub_82900C50);
GH_HOOK(sub_82900C50) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914020u>, Absolute<0x82018DA4u>, Absolute<0x829CCC34u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900C50);
}

GH_EXTERN(__imp__sub_82900E50);
GH_HOOK(sub_82900E50) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, true, Absolute<0x8291416Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCC80u>>(ctx, base, __imp__sub_82900E50);
}

GH_EXTERN(__imp__sub_82900F88);
GH_HOOK(sub_82900F88) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82000AE4u>, Absolute<0x82012ED0u>, Absolute<0x829CCCB8u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900F88);
}

GH_EXTERN(__imp__sub_82901418);
GH_HOOK(sub_82901418) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, true, Absolute<0x82920AB4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE00u>>(ctx, base, __imp__sub_82901418);
}

GH_EXTERN(__imp__sub_829017D8);
GH_HOOK(sub_829017D8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000AE8u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE20u>>(ctx, base, __imp__sub_829017D8);
}

GH_EXTERN(__imp__sub_82901990);
GH_HOOK(sub_82901990) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292137Cu>, Absolute<0x82087CF4u>, Absolute<0x829CCE60u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901990);
}

GH_EXTERN(__imp__sub_82901CC8);
GH_HOOK(sub_82901CC8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829CCEC0u>, Absolute<0x820024FCu>, Absolute<0x829CCED8u>, ScratchSet<Scratch<&GuestContext::r11, -2103640064ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901CC8);
}

GH_EXTERN(__imp__sub_82902600);
GH_HOOK(sub_82902600) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921704u>, Absolute<0x820894BCu>, Absolute<0x829CD0F0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902600);
}

GH_EXTERN(__imp__sub_82902900);
GH_HOOK(sub_82902900) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921BD4u>, Absolute<0x8232C524u>, Absolute<0x829CD150u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902900);
}

GH_EXTERN(__imp__sub_82902C40);
GH_HOOK(sub_82902C40) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922330u>, Absolute<0x823338DCu>, Absolute<0x829CD1B8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902C40);
}

GH_EXTERN(__imp__sub_82902FF8);
GH_HOOK(sub_82902FF8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922AC0u>, Absolute<0x82338A04u>, Absolute<0x829CD230u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902FF8);
}

GH_EXTERN(__imp__sub_82903398);
GH_HOOK(sub_82903398) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923144u>, Absolute<0x8233DF24u>, Absolute<0x829CD2A4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903398);
}

GH_EXTERN(__imp__sub_82903808);
GH_HOOK(sub_82903808) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923748u>, Absolute<0x82340E04u>, Absolute<0x829CD31Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903808);
}

GH_EXTERN(__imp__sub_8262A600);
GH_HOOK(sub_8262A600) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x823764A4u>, ScratchSet<Scratch<&GuestContext::r11, -2110324736ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262A600);
}

GH_EXTERN(__imp__sub_82901C08);
GH_HOOK(sub_82901C08) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292147Cu>, Absolute<0x82012ED0u>, Absolute<0x829CCEC0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901C08);
}

GH_EXTERN(__imp__sub_829022B8);
GH_HOOK(sub_829022B8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829214DCu>, Absolute<0x82088FDCu>, Absolute<0x829CD0A4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829022B8);
}

GH_EXTERN(__imp__sub_829027C0);
GH_HOOK(sub_829027C0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829218D4u>, Absolute<0x82328804u>, Absolute<0x829CD128u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829027C0);
}

GH_EXTERN(__imp__sub_82902AE0);
GH_HOOK(sub_82902AE0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922074u>, Absolute<0x82330B04u>, Absolute<0x829CD18Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902AE0);
}

GH_EXTERN(__imp__sub_82902E40);
GH_HOOK(sub_82902E40) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829226B4u>, Absolute<0x82336764u>, Absolute<0x829CD204u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902E40);
}

GH_EXTERN(__imp__sub_82903238);
GH_HOOK(sub_82903238) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922F14u>, Absolute<0x8233CE6Cu>, Absolute<0x829CD278u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903238);
}

GH_EXTERN(__imp__sub_82903670);
GH_HOOK(sub_82903670) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829234F0u>, Absolute<0x8233F900u>, Absolute<0x829CD2F4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903670);
}

GH_EXTERN(__imp__sub_825422B0);
GH_HOOK(sub_825422B0) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Absolute<0x82016F24u>, ScratchSet<Scratch<&GuestContext::r11, -2113863680ll>>>(ctx, base, __imp__sub_825422B0);
}

GH_EXTERN(__imp__sub_82455A28);
GH_HOOK(sub_82455A28) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Relative<&GuestContext::r3, 1000>, ScratchSet<>>(ctx, base, __imp__sub_82455A28);
}

GH_EXTERN(__imp__sub_82540070);
GH_HOOK(sub_82540070) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Absolute<0x8201D9D8u>, ScratchSet<Scratch<&GuestContext::r11, -2113798144ll>>>(ctx, base, __imp__sub_82540070);
}

GH_EXTERN(__imp__sub_829005B8);
GH_HOOK(sub_829005B8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913C7Cu>, Absolute<0x82000B44u>, Absolute<0x829B2164u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_829005B8);
}

GH_EXTERN(__imp__sub_82900A98);
GH_HOOK(sub_82900A98) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82913E18u>, Absolute<0x82018378u>, Absolute<0x829CC940u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900A98);
}

GH_EXTERN(__imp__sub_82900C90);
GH_HOOK(sub_82900C90) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8291403Cu>, Absolute<0x82018DC8u>, Absolute<0x829CCC3Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900C90);
}

GH_EXTERN(__imp__sub_82900E88);
GH_HOOK(sub_82900E88) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82914260u>, Absolute<0x82000E5Cu>, Absolute<0x829CCC88u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900E88);
}

GH_EXTERN(__imp__sub_82901970);
GH_HOOK(sub_82901970) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8291088Cu>, Absolute<0x82018650u>, Absolute<0x829CCE5Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901970);
}

GH_EXTERN(__imp__sub_82901CA8);
GH_HOOK(sub_82901CA8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829CCEC4u>, Absolute<0x8201238Cu>, Absolute<0x829CCED4u>, ScratchSet<Scratch<&GuestContext::r11, -2103640064ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901CA8);
}

GH_EXTERN(__imp__sub_829025E0);
GH_HOOK(sub_829025E0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921704u>, Absolute<0x820894BCu>, Absolute<0x829CD0ECu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829025E0);
}

GH_EXTERN(__imp__sub_829028E0);
GH_HOOK(sub_829028E0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921BD4u>, Absolute<0x8232C524u>, Absolute<0x829CD14Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829028E0);
}

GH_EXTERN(__imp__sub_82902C20);
GH_HOOK(sub_82902C20) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922330u>, Absolute<0x823338DCu>, Absolute<0x829CD1B4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902C20);
}

GH_EXTERN(__imp__sub_82902FD8);
GH_HOOK(sub_82902FD8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922A14u>, Absolute<0x82338454u>, Absolute<0x829CD22Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902FD8);
}

GH_EXTERN(__imp__sub_82903378);
GH_HOOK(sub_82903378) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923144u>, Absolute<0x8233DF24u>, Absolute<0x829CD2A0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903378);
}

GH_EXTERN(__imp__sub_829037E8);
GH_HOOK(sub_829037E8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923748u>, Absolute<0x82340E04u>, Absolute<0x829CD318u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829037E8);
}

GH_EXTERN(__imp__sub_8262A708);
GH_HOOK(sub_8262A708) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x823764ACu>, ScratchSet<Scratch<&GuestContext::r11, -2110324736ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262A708);
}

GH_EXTERN(__imp__sub_8263F1D0);
GH_HOOK(sub_8263F1D0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Relative<&GuestContext::r3, 68>, ScratchSet<>, Relative<&GuestContext::r4, 0>>(ctx, base, __imp__sub_8263F1D0);
}

GH_EXTERN(__imp__sub_82495160);
GH_HOOK(sub_82495160) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Absolute<0x82000DACu>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>>(ctx, base, __imp__sub_82495160);
}

GH_EXTERN(__imp__sub_8262AE88);
GH_HOOK(sub_8262AE88) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x8234EB38u>, ScratchSet<Scratch<&GuestContext::r11, -2110455808ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AE88);
}

GH_EXTERN(__imp__sub_828ED9C0);
GH_HOOK(sub_828ED9C0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829125C0u>, Absolute<0x82000E5Cu>, Absolute<0x829AB3E4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED9C0);
}

GH_EXTERN(__imp__sub_82901A20);
GH_HOOK(sub_82901A20) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8291088Cu>, Absolute<0x82013260u>, Absolute<0x829CCE74u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901A20);
}

GH_EXTERN(__imp__sub_82901D20);
GH_HOOK(sub_82901D20) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82921498u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCEE4u>>(ctx, base, __imp__sub_82901D20);
}

GH_EXTERN(__imp__sub_829025C0);
GH_HOOK(sub_829025C0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829216E8u>, Absolute<0x82089434u>, Absolute<0x829CD0E8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829025C0);
}

GH_EXTERN(__imp__sub_829028C0);
GH_HOOK(sub_829028C0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921B38u>, Absolute<0x8232B184u>, Absolute<0x829CD148u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829028C0);
}

GH_EXTERN(__imp__sub_82902C00);
GH_HOOK(sub_82902C00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829222B4u>, Absolute<0x8233305Cu>, Absolute<0x829CD1B0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902C00);
}

GH_EXTERN(__imp__sub_82902FB8);
GH_HOOK(sub_82902FB8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922A14u>, Absolute<0x82338454u>, Absolute<0x829CD228u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902FB8);
}

GH_EXTERN(__imp__sub_82903358);
GH_HOOK(sub_82903358) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829230C8u>, Absolute<0x8233DAE4u>, Absolute<0x829CD29Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903358);
}

GH_EXTERN(__imp__sub_828ED808);
GH_HOOK(sub_828ED808) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8291203Cu>, Absolute<0x8200DE20u>, Absolute<0x829AAEE4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED808);
}

GH_EXTERN(__imp__sub_828EDA40);
GH_HOOK(sub_828EDA40) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829126FCu>, Absolute<0x820121A0u>, Absolute<0x829AB3F4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828EDA40);
}

GH_EXTERN(__imp__sub_82901890);
GH_HOOK(sub_82901890) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921364u>, Absolute<0x82087040u>, Absolute<0x829CCE3Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901890);
}

GH_EXTERN(__imp__sub_82901C28);
GH_HOOK(sub_82901C28) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921480u>, Absolute<0x82016F20u>, Absolute<0x829CCEC4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901C28);
}

GH_EXTERN(__imp__sub_829022D8);
GH_HOOK(sub_829022D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921538u>, Absolute<0x820890CCu>, Absolute<0x829CD0A8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829022D8);
}

GH_EXTERN(__imp__sub_829027E0);
GH_HOOK(sub_829027E0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921950u>, Absolute<0x82328CF4u>, Absolute<0x829CD12Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829027E0);
}

GH_EXTERN(__imp__sub_82902B00);
GH_HOOK(sub_82902B00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922074u>, Absolute<0x82330B04u>, Absolute<0x829CD190u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902B00);
}

GH_EXTERN(__imp__sub_82902E60);
GH_HOOK(sub_82902E60) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922760u>, Absolute<0x823370BCu>, Absolute<0x829CD208u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902E60);
}

GH_EXTERN(__imp__sub_82903258);
GH_HOOK(sub_82903258) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922F14u>, Absolute<0x8233CE6Cu>, Absolute<0x829CD27Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903258);
}

GH_EXTERN(__imp__sub_82903690);
GH_HOOK(sub_82903690) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923574u>, Absolute<0x8233FD2Cu>, Absolute<0x829CD2F8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903690);
}

GH_EXTERN(__imp__sub_8242A2F0);
GH_HOOK(sub_8242A2F0) {
  GH_PROLOGUE();
  Store<Output<&GuestContext::f1, Relative<&GuestContext::r3, 0>>, Output<&GuestContext::f2, Relative<&GuestContext::r3, 4>>, Output<&GuestContext::f3, Relative<&GuestContext::r3, 8>>, Output<&GuestContext::f4, Relative<&GuestContext::r3, 12>>>(ctx, base, __imp__sub_8242A2F0);
}

GH_EXTERN(__imp__sub_8244AA80);
GH_HOOK(sub_8244AA80) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Relative<&GuestContext::r3, 980>, ScratchSet<>>(ctx, base, __imp__sub_8244AA80);
}

GH_EXTERN(__imp__sub_82901950);
GH_HOOK(sub_82901950) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82000AECu>, Absolute<0x82018650u>, Absolute<0x829CCE58u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901950);
}

GH_EXTERN(__imp__sub_82901C88);
GH_HOOK(sub_82901C88) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829CCEC0u>, Absolute<0x820024FCu>, Absolute<0x829CCED0u>, ScratchSet<Scratch<&GuestContext::r11, -2103640064ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901C88);
}

GH_EXTERN(__imp__sub_829025A0);
GH_HOOK(sub_829025A0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829216E8u>, Absolute<0x82089434u>, Absolute<0x829CD0E4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829025A0);
}

GH_EXTERN(__imp__sub_829028A0);
GH_HOOK(sub_829028A0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921B38u>, Absolute<0x8232B184u>, Absolute<0x829CD144u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829028A0);
}

GH_EXTERN(__imp__sub_82902BE0);
GH_HOOK(sub_82902BE0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829222B4u>, Absolute<0x8233305Cu>, Absolute<0x829CD1ACu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902BE0);
}

GH_EXTERN(__imp__sub_82902F98);
GH_HOOK(sub_82902F98) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922958u>, Absolute<0x82337C38u>, Absolute<0x829CD224u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902F98);
}

GH_EXTERN(__imp__sub_82903338);
GH_HOOK(sub_82903338) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829230C8u>, Absolute<0x8233DAE4u>, Absolute<0x829CD298u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903338);
}

GH_EXTERN(__imp__sub_82903770);
GH_HOOK(sub_82903770) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923664u>, Absolute<0x823401E4u>, Absolute<0x829CD314u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903770);
}

GH_EXTERN(__imp__sub_828ED948);
GH_HOOK(sub_828ED948) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829124A4u>, Absolute<0x82000E5Cu>, Absolute<0x829AB3D4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED948);
}

GH_EXTERN(__imp__sub_82900458);
GH_HOOK(sub_82900458) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82913C6Cu>, Absolute<0x82012ED0u>, Absolute<0x829B2134u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900458);
}

GH_EXTERN(__imp__sub_829005F8);
GH_HOOK(sub_829005F8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82913C94u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829B216Cu>>(ctx, base, __imp__sub_829005F8);
}

GH_EXTERN(__imp__sub_82901AC0);
GH_HOOK(sub_82901AC0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829213FCu>, Absolute<0x82088000u>, Absolute<0x829CCE88u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901AC0);
}

GH_EXTERN(__imp__sub_82901DA8);
GH_HOOK(sub_82901DA8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x829214A4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCEF8u>>(ctx, base, __imp__sub_82901DA8);
}

GH_EXTERN(__imp__sub_829026A0);
GH_HOOK(sub_829026A0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921754u>, Absolute<0x82089850u>, Absolute<0x829CD104u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829026A0);
}

GH_EXTERN(__imp__sub_829029A0);
GH_HOOK(sub_829029A0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921DA8u>, Absolute<0x8232DF9Cu>, Absolute<0x829CD164u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829029A0);
}

GH_EXTERN(__imp__sub_82902CE0);
GH_HOOK(sub_82902CE0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922408u>, Absolute<0x82333FE4u>, Absolute<0x829CD1D8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902CE0);
}

GH_EXTERN(__imp__sub_82903098);
GH_HOOK(sub_82903098) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922BE8u>, Absolute<0x82339F7Cu>, Absolute<0x829CD244u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903098);
}

GH_EXTERN(__imp__sub_828ED968);
GH_HOOK(sub_828ED968) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829124F0u>, Absolute<0x82011668u>, Absolute<0x829AB3D8u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED968);
}

GH_EXTERN(__imp__sub_82495170);
GH_HOOK(sub_82495170) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Absolute<0x82000B48u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>>(ctx, base, __imp__sub_82495170);
}

GH_EXTERN(__imp__sub_824A69C0);
GH_HOOK(sub_824A69C0) {
  GH_PROLOGUE();
  Store<Output<&GuestContext::f1, Relative<&GuestContext::r3, 132>>>(ctx, base, __imp__sub_824A69C0);
}

GH_EXTERN(__imp__sub_828ED878);
GH_HOOK(sub_828ED878) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, true, Absolute<0x829120D4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829AB3B8u>>(ctx, base, __imp__sub_828ED878);
}

GH_EXTERN(__imp__sub_828EDA00);
GH_HOOK(sub_828EDA00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829126B4u>, Absolute<0x82000E5Cu>, Absolute<0x829AB3ECu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828EDA00);
}

GH_EXTERN(__imp__sub_82900478);
GH_HOOK(sub_82900478) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82913C70u>, Absolute<0x82016F20u>, Absolute<0x829B2138u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900478);
}

GH_EXTERN(__imp__sub_82900610);
GH_HOOK(sub_82900610) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82913C98u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829B2170u>>(ctx, base, __imp__sub_82900610);
}

GH_EXTERN(__imp__sub_82900CF0);
GH_HOOK(sub_82900CF0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914058u>, Absolute<0x82018E18u>, Absolute<0x829CCC48u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900CF0);
}

GH_EXTERN(__imp__sub_82900EE8);
GH_HOOK(sub_82900EE8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829142B0u>, Absolute<0x820196F8u>, Absolute<0x829CCC94u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900EE8);
}

GH_EXTERN(__imp__sub_82901A80);
GH_HOOK(sub_82901A80) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82000AECu>, Absolute<0x82013260u>, Absolute<0x829CCE80u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901A80);
}

GH_EXTERN(__imp__sub_82901D68);
GH_HOOK(sub_82901D68) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292148Cu>, Absolute<0x82000B44u>, Absolute<0x829CCEF0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901D68);
}

GH_EXTERN(__imp__sub_82902720);
GH_HOOK(sub_82902720) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292178Cu>, Absolute<0x82327CF0u>, Absolute<0x829CD114u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110652416ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902720);
}

GH_EXTERN(__imp__sub_82902A40);
GH_HOOK(sub_82902A40) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921ED0u>, Absolute<0x8232F24Cu>, Absolute<0x829CD178u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902A40);
}

GH_EXTERN(__imp__sub_82902DA0);
GH_HOOK(sub_82902DA0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829225BCu>, Absolute<0x82334E2Cu>, Absolute<0x829CD1F0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902DA0);
}

GH_EXTERN(__imp__sub_82903178);
GH_HOOK(sub_82903178) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922DE0u>, Absolute<0x8233C714u>, Absolute<0x829CD260u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903178);
}

GH_EXTERN(__imp__sub_829035B0);
GH_HOOK(sub_829035B0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923400u>, Absolute<0x8233F424u>, Absolute<0x829CD2DCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829035B0);
}

GH_EXTERN(__imp__sub_82461748);
GH_HOOK(sub_82461748) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Relative<&GuestContext::r3, 1012>, ScratchSet<>>(ctx, base, __imp__sub_82461748);
}

GH_EXTERN(__imp__sub_8253CC18);
GH_HOOK(sub_8253CC18) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Absolute<0x82000DA4u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>>(ctx, base, __imp__sub_8253CC18);
}

GH_EXTERN(__imp__sub_82900538);
GH_HOOK(sub_82900538) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829B2138u>, Absolute<0x8200DE00u>, Absolute<0x829B2150u>, ScratchSet<Scratch<&GuestContext::r11, -2103771136ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900538);
}

GH_EXTERN(__imp__sub_82900778);
GH_HOOK(sub_82900778) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82913CFCu>, Absolute<0x820179FCu>, Absolute<0x829CBFE4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900778);
}

GH_EXTERN(__imp__sub_82900B38);
GH_HOOK(sub_82900B38) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82913E38u>, Absolute<0x82018BACu>, Absolute<0x829CC9A8u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900B38);
}

GH_EXTERN(__imp__sub_82900D30);
GH_HOOK(sub_82900D30) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914074u>, Absolute<0x82018E24u>, Absolute<0x829CCC50u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900D30);
}

GH_EXTERN(__imp__sub_82900F28);
GH_HOOK(sub_82900F28) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914314u>, Absolute<0x82019700u>, Absolute<0x829CCC9Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900F28);
}

GH_EXTERN(__imp__sub_829018E0);
GH_HOOK(sub_829018E0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82000AECu>, Absolute<0x82018650u>, Absolute<0x829CCE48u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829018E0);
}

GH_EXTERN(__imp__sub_82901C48);
GH_HOOK(sub_82901C48) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829CCEC0u>, Absolute<0x820024FCu>, Absolute<0x829CCEC8u>, ScratchSet<Scratch<&GuestContext::r11, -2103640064ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901C48);
}

GH_EXTERN(__imp__sub_829022F8);
GH_HOOK(sub_829022F8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921538u>, Absolute<0x820890CCu>, Absolute<0x829CD0ACu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829022F8);
}

GH_EXTERN(__imp__sub_82902800);
GH_HOOK(sub_82902800) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921950u>, Absolute<0x82328CF4u>, Absolute<0x829CD130u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902800);
}

GH_EXTERN(__imp__sub_82902B20);
GH_HOOK(sub_82902B20) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922110u>, Absolute<0x82330DACu>, Absolute<0x829CD194u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902B20);
}

GH_EXTERN(__imp__sub_82902E80);
GH_HOOK(sub_82902E80) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922760u>, Absolute<0x823370BCu>, Absolute<0x829CD20Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902E80);
}

GH_EXTERN(__imp__sub_82903278);
GH_HOOK(sub_82903278) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922F9Cu>, Absolute<0x8233D1F4u>, Absolute<0x829CD280u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903278);
}

GH_EXTERN(__imp__sub_829036B0);
GH_HOOK(sub_829036B0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923574u>, Absolute<0x8233FD2Cu>, Absolute<0x829CD2FCu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829036B0);
}

GH_EXTERN(__imp__sub_82703620);
GH_HOOK(sub_82703620) {
  GH_PROLOGUE();
  Store<Output<&GuestContext::f1, Relative<&GuestContext::r3, 72>>>(ctx, base, __imp__sub_82703620);
}

GH_EXTERN(__imp__sub_829003F8);
GH_HOOK(sub_829003F8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82913C44u>, Absolute<0x82016EB0u>, Absolute<0x829B2128u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_829003F8);
}

GH_EXTERN(__imp__sub_82900518);
GH_HOOK(sub_82900518) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829B2134u>, Absolute<0x820024FCu>, Absolute<0x829B214Cu>, ScratchSet<Scratch<&GuestContext::r11, -2103771136ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900518);
}

GH_EXTERN(__imp__sub_82900758);
GH_HOOK(sub_82900758) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913CFCu>, Absolute<0x820179FCu>, Absolute<0x829CBFE0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900758);
}

GH_EXTERN(__imp__sub_82900C10);
GH_HOOK(sub_82900C10) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914004u>, Absolute<0x82018D7Cu>, Absolute<0x829CCC2Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900C10);
}

GH_EXTERN(__imp__sub_82900E10);
GH_HOOK(sub_82900E10) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914100u>, Absolute<0x82018F30u>, Absolute<0x829CCC78u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900E10);
}

GH_EXTERN(__imp__sub_82901028);
GH_HOOK(sub_82901028) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914B14u>, Absolute<0x8201DFFCu>, Absolute<0x829CCCCCu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901028);
}

GH_EXTERN(__imp__sub_82901780);
GH_HOOK(sub_82901780) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829211B0u>, Absolute<0x82086778u>, Absolute<0x829CCE14u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901780);
}

GH_EXTERN(__imp__sub_829018C8);
GH_HOOK(sub_829018C8) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000AE8u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE44u>>(ctx, base, __imp__sub_829018C8);
}

GH_EXTERN(__imp__sub_82901A60);
GH_HOOK(sub_82901A60) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921394u>, Absolute<0x82087F60u>, Absolute<0x829CCE7Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901A60);
}

GH_EXTERN(__imp__sub_82901D50);
GH_HOOK(sub_82901D50) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x829214A0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCEECu>>(ctx, base, __imp__sub_82901D50);
}

GH_EXTERN(__imp__sub_82902680);
GH_HOOK(sub_82902680) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292173Cu>, Absolute<0x820896CCu>, Absolute<0x829CD100u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902680);
}

GH_EXTERN(__imp__sub_82902980);
GH_HOOK(sub_82902980) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921D2Cu>, Absolute<0x8232DA44u>, Absolute<0x829CD160u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902980);
}

GH_EXTERN(__imp__sub_82902CC0);
GH_HOOK(sub_82902CC0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829223C8u>, Absolute<0x82333F18u>, Absolute<0x829CD1D4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902CC0);
}

GH_EXTERN(__imp__sub_82903078);
GH_HOOK(sub_82903078) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922BE8u>, Absolute<0x82339F7Cu>, Absolute<0x829CD240u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903078);
}

GH_EXTERN(__imp__sub_82903418);
GH_HOOK(sub_82903418) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292320Cu>, Absolute<0x8233E384u>, Absolute<0x829CD2B4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903418);
}

GH_EXTERN(__imp__sub_829038E0);
GH_HOOK(sub_829038E0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292384Cu>, Absolute<0x82341084u>, Absolute<0x829CD32Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829038E0);
}

GH_EXTERN(__imp__sub_82902F00);
GH_HOOK(sub_82902F00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922898u>, Absolute<0x823379E4u>, Absolute<0x829CD21Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902F00);
}

GH_EXTERN(__imp__sub_829032F8);
GH_HOOK(sub_829032F8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x8292301Cu>, Absolute<0x8233D37Cu>, Absolute<0x829CD290u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829032F8);
}

GH_EXTERN(__imp__sub_82903730);
GH_HOOK(sub_82903730) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923624u>, Absolute<0x8234011Cu>, Absolute<0x829CD30Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903730);
}

GH_EXTERN(__imp__sub_8262AE70);
GH_HOOK(sub_8262AE70) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82374994u>, ScratchSet<Scratch<&GuestContext::r11, -2110324736ll>, Scratch<&GuestContext::r3, 1ll>>, Relative<&GuestContext::r5, 0>>(ctx, base, __imp__sub_8262AE70);
}

GH_EXTERN(__imp__sub_828ED8D0);
GH_HOOK(sub_828ED8D0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829121CCu>, Absolute<0x82000E5Cu>, Absolute<0x829AB3C4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED8D0);
}

GH_EXTERN(__imp__sub_829005A0);
GH_HOOK(sub_829005A0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82913C90u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829B2160u>>(ctx, base, __imp__sub_829005A0);
}

GH_EXTERN(__imp__sub_82900718);
GH_HOOK(sub_82900718) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913CC4u>, Absolute<0x82016F30u>, Absolute<0x829B245Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900718);
}

GH_EXTERN(__imp__sub_82900BB0);
GH_HOOK(sub_82900BB0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82913F9Cu>, Absolute<0x82000E5Cu>, Absolute<0x829CC9B8u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900BB0);
}

GH_EXTERN(__imp__sub_82900DB0);
GH_HOOK(sub_82900DB0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829140B0u>, Absolute<0x82018EC0u>, Absolute<0x829CCC60u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900DB0);
}

GH_EXTERN(__imp__sub_82900FC8);
GH_HOOK(sub_82900FC8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914520u>, Absolute<0x8201D998u>, Absolute<0x829CCCC0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900FC8);
}

GH_EXTERN(__imp__sub_82901450);
GH_HOOK(sub_82901450) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82920BA8u>, Absolute<0x82000E5Cu>, Absolute<0x829CCE08u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901450);
}

GH_EXTERN(__imp__sub_82901870);
GH_HOOK(sub_82901870) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921364u>, Absolute<0x82087040u>, Absolute<0x829CCE38u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901870);
}

GH_EXTERN(__imp__sub_82901BA8);
GH_HOOK(sub_82901BA8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921458u>, Absolute<0x820882B8u>, Absolute<0x829CCEB4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901BA8);
}

GH_EXTERN(__imp__sub_829004B8);
GH_HOOK(sub_829004B8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829B2138u>, Absolute<0x820146D0u>, Absolute<0x829B2140u>, ScratchSet<Scratch<&GuestContext::r11, -2103771136ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_829004B8);
}

GH_EXTERN(__imp__sub_82900680);
GH_HOOK(sub_82900680) {
  GH_PROLOGUE();
  MultiplyAdd<Absolute<0x82913C60u>, Absolute<0x82016F24u>, Absolute<0x829B2178u>, Absolute<0x829B2180u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>, Scratch<&GuestContext::r8, -2103771136ll>>>(ctx, base, __imp__sub_82900680);
}

GH_EXTERN(__imp__sub_82900B58);
GH_HOOK(sub_82900B58) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, true, Absolute<0x82913EA4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CC9ACu>>(ctx, base, __imp__sub_82900B58);
}

GH_EXTERN(__imp__sub_82900CD0);
GH_HOOK(sub_82900CD0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914058u>, Absolute<0x82018E18u>, Absolute<0x829CCC44u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900CD0);
}

GH_EXTERN(__imp__sub_82900EC8);
GH_HOOK(sub_82900EC8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829142B0u>, Absolute<0x820196F8u>, Absolute<0x829CCC90u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900EC8);
}

GH_EXTERN(__imp__sub_82901A00);
GH_HOOK(sub_82901A00) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82000AECu>, Absolute<0x82018650u>, Absolute<0x829CCE70u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901A00);
}

GH_EXTERN(__imp__sub_82901D08);
GH_HOOK(sub_82901D08) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82921494u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCEE0u>>(ctx, base, __imp__sub_82901D08);
}

GH_EXTERN(__imp__sub_82902338);
GH_HOOK(sub_82902338) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292155Cu>, Absolute<0x82089200u>, Absolute<0x829CD0E0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902338);
}

GH_EXTERN(__imp__sub_82902880);
GH_HOOK(sub_82902880) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921AACu>, Absolute<0x8232AFECu>, Absolute<0x829CD140u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902880);
}

GH_EXTERN(__imp__sub_82902BC0);
GH_HOOK(sub_82902BC0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922238u>, Absolute<0x823321E4u>, Absolute<0x829CD1A8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902BC0);
}

GH_EXTERN(__imp__sub_82902F78);
GH_HOOK(sub_82902F78) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922958u>, Absolute<0x82337C38u>, Absolute<0x829CD220u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902F78);
}

GH_EXTERN(__imp__sub_82903318);
GH_HOOK(sub_82903318) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292301Cu>, Absolute<0x8233D37Cu>, Absolute<0x829CD294u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903318);
}

GH_EXTERN(__imp__sub_82903750);
GH_HOOK(sub_82903750) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923664u>, Absolute<0x823401E4u>, Absolute<0x829CD310u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903750);
}

GH_EXTERN(__imp__sub_82902BA0);
GH_HOOK(sub_82902BA0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922238u>, Absolute<0x823321E4u>, Absolute<0x829CD1A4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902BA0);
}

GH_EXTERN(__imp__sub_82903920);
GH_HOOK(sub_82903920) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292388Cu>, Absolute<0x82341148u>, Absolute<0x829CD334u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903920);
}

GH_EXTERN(__imp__sub_824633D8);
GH_HOOK(sub_824633D8) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Relative<&GuestContext::r3, 1140>, ScratchSet<>>(ctx, base, __imp__sub_824633D8);
}

GH_EXTERN(__imp__sub_828ED890);
GH_HOOK(sub_828ED890) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x829120D8u>, Absolute<0x82000E5Cu>, Absolute<0x829AB3BCu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828ED890);
}

GH_EXTERN(__imp__sub_82903590);
GH_HOOK(sub_82903590) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923400u>, Absolute<0x8233F424u>, Absolute<0x829CD2D8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903590);
}

GH_EXTERN(__imp__sub_82418D50);
GH_HOOK(sub_82418D50) {
  GH_PROLOGUE();
  Store<Output<&GuestContext::f1, Relative<&GuestContext::r3, 0>>, Output<&GuestContext::f2, Relative<&GuestContext::r3, 4>>>(ctx, base, __imp__sub_82418D50);
}

GH_EXTERN(__imp__sub_82900438);
GH_HOOK(sub_82900438) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913C68u>, Absolute<0x820183ACu>, Absolute<0x829B2130u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900438);
}

GH_EXTERN(__imp__sub_82900570);
GH_HOOK(sub_82900570) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82913C88u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829B2158u>>(ctx, base, __imp__sub_82900570);
}

GH_EXTERN(__imp__sub_82900668);
GH_HOOK(sub_82900668) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82913C94u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829B217Cu>>(ctx, base, __imp__sub_82900668);
}

GH_EXTERN(__imp__sub_82900A78);
GH_HOOK(sub_82900A78) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913E18u>, Absolute<0x82018378u>, Absolute<0x829CC93Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900A78);
}

GH_EXTERN(__imp__sub_82900C70);
GH_HOOK(sub_82900C70) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82914020u>, Absolute<0x82018DA4u>, Absolute<0x829CCC38u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900C70);
}

GH_EXTERN(__imp__sub_82900E68);
GH_HOOK(sub_82900E68) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82914170u>, Absolute<0x82000E5Cu>, Absolute<0x829CCC84u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900E68);
}

GH_EXTERN(__imp__sub_82901920);
GH_HOOK(sub_82901920) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82000AE4u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2103640064ll>>, Absolute<0x829CCE50u>>(ctx, base, __imp__sub_82901920);
}

GH_EXTERN(__imp__sub_82901B40);
GH_HOOK(sub_82901B40) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921430u>, Absolute<0x82088034u>, Absolute<0x829CCE9Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901B40);
}

GH_EXTERN(__imp__sub_82901E30);
GH_HOOK(sub_82901E30) {
  GH_PROLOGUE();
  MultiplyAdd<Absolute<0x82921470u>, Absolute<0x82016F24u>, Absolute<0x829CCF04u>, Absolute<0x829CCF0Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>, Scratch<&GuestContext::r8, -2103640064ll>>>(ctx, base, __imp__sub_82901E30);
}

GH_EXTERN(__imp__sub_82902860);
GH_HOOK(sub_82902860) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921AACu>, Absolute<0x8232AFECu>, Absolute<0x829CD13Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902860);
}

GH_EXTERN(__imp__sub_82902B80);
GH_HOOK(sub_82902B80) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292219Cu>, Absolute<0x823313BCu>, Absolute<0x829CD1A0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902B80);
}

GH_EXTERN(__imp__sub_82902EE0);
GH_HOOK(sub_82902EE0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922898u>, Absolute<0x823379E4u>, Absolute<0x829CD218u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902EE0);
}

GH_EXTERN(__imp__sub_829032D8);
GH_HOOK(sub_829032D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922FDCu>, Absolute<0x8233D2BCu>, Absolute<0x829CD28Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829032D8);
}

GH_EXTERN(__imp__sub_82903710);
GH_HOOK(sub_82903710) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923624u>, Absolute<0x8234011Cu>, Absolute<0x829CD308u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903710);
}

GH_EXTERN(__imp__sub_828ED8F0);
GH_HOOK(sub_828ED8F0) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, true, Absolute<0x829123ACu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829AB3C8u>>(ctx, base, __imp__sub_828ED8F0);
}

GH_EXTERN(__imp__sub_828EDA20);
GH_HOOK(sub_828EDA20) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829126FCu>, Absolute<0x820121A0u>, Absolute<0x829AB3F0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_828EDA20);
}

GH_EXTERN(__imp__sub_82468DF8);
GH_HOOK(sub_82468DF8) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Absolute<0x82000B44u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>>>(ctx, base, __imp__sub_82468DF8);
}

GH_EXTERN(__imp__sub_82547720);
GH_HOOK(sub_82547720) {
  GH_PROLOGUE();
  Load<&GuestContext::f1, false, Absolute<0x82017458u>, ScratchSet<Scratch<&GuestContext::r11, -2113863680ll>>>(ctx, base, __imp__sub_82547720);
}

GH_EXTERN(__imp__sub_82900588);
GH_HOOK(sub_82900588) {
  GH_PROLOGUE();
  Load<&GuestContext::f0, false, Absolute<0x82913C8Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2103771136ll>>, Absolute<0x829B215Cu>>(ctx, base, __imp__sub_82900588);
}

GH_EXTERN(__imp__sub_82900D50);
GH_HOOK(sub_82900D50) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914090u>, Absolute<0x82018E3Cu>, Absolute<0x829CCC54u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900D50);
}

GH_EXTERN(__imp__sub_82900F48);
GH_HOOK(sub_82900F48) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914504u>, Absolute<0x8201A308u>, Absolute<0x829CCCB0u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900F48);
}

GH_EXTERN(__imp__sub_829013D8);
GH_HOOK(sub_829013D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82920A44u>, Absolute<0x820856C4u>, Absolute<0x829CCDF8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829013D8);
}

GH_EXTERN(__imp__sub_829017F0);
GH_HOOK(sub_829017F0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82000AECu>, Absolute<0x82018650u>, Absolute<0x829CCE24u>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829017F0);
}

GH_EXTERN(__imp__sub_82901AA0);
GH_HOOK(sub_82901AA0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829213FCu>, Absolute<0x82088000u>, Absolute<0x829CCE84u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901AA0);
}

GH_EXTERN(__imp__sub_82901D88);
GH_HOOK(sub_82901D88) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921490u>, Absolute<0x82000B44u>, Absolute<0x829CCEF4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901D88);
}

GH_EXTERN(__imp__sub_82902740);
GH_HOOK(sub_82902740) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x8292178Cu>, Absolute<0x82327CF0u>, Absolute<0x829CD118u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110652416ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902740);
}

GH_EXTERN(__imp__sub_82902A60);
GH_HOOK(sub_82902A60) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921F4Cu>, Absolute<0x8232F55Cu>, Absolute<0x829CD17Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902A60);
}

GH_EXTERN(__imp__sub_82902DC0);
GH_HOOK(sub_82902DC0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829225BCu>, Absolute<0x82334E2Cu>, Absolute<0x829CD1F4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902DC0);
}

GH_EXTERN(__imp__sub_82903198);
GH_HOOK(sub_82903198) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82922DE0u>, Absolute<0x8233C714u>, Absolute<0x829CD264u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82903198);
}

GH_EXTERN(__imp__sub_829035D0);
GH_HOOK(sub_829035D0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82923440u>, Absolute<0x8233F508u>, Absolute<0x829CD2E0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829035D0);
}

GH_EXTERN(__imp__sub_829005D8);
GH_HOOK(sub_829005D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913C80u>, Absolute<0x82000B44u>, Absolute<0x829B2168u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_829005D8);
}

GH_EXTERN(__imp__sub_82900B90);
GH_HOOK(sub_82900B90) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82913F98u>, Absolute<0x82000E5Cu>, Absolute<0x829CC9B4u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900B90);
}

GH_EXTERN(__imp__sub_82900D90);
GH_HOOK(sub_82900D90) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829140B0u>, Absolute<0x82018EC0u>, Absolute<0x829CCC5Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900D90);
}

GH_EXTERN(__imp__sub_82900FA8);
GH_HOOK(sub_82900FA8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82000AECu>, Absolute<0x82012ED0u>, Absolute<0x829CCCBCu>, ScratchSet<Scratch<&GuestContext::r11, -2113929216ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900FA8);
}

GH_EXTERN(__imp__sub_82901430);
GH_HOOK(sub_82901430) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Multiply, Absolute<0x82920AB8u>, Absolute<0x82000E5Cu>, Absolute<0x829CCE04u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113929216ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901430);
}

GH_EXTERN(__imp__sub_82901850);
GH_HOOK(sub_82901850) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829211C8u>, Absolute<0x82086D84u>, Absolute<0x829CCE30u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113404928ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901850);
}

GH_EXTERN(__imp__sub_82901B88);
GH_HOOK(sub_82901B88) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921458u>, Absolute<0x820882B8u>, Absolute<0x829CCEB0u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113339392ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901B88);
}

GH_EXTERN(__imp__sub_82901E58);
GH_HOOK(sub_82901E58) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x829214C0u>, Absolute<0x82018BACu>, Absolute<0x829CCF10u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901E58);
}

GH_EXTERN(__imp__sub_82902760);
GH_HOOK(sub_82902760) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82921818u>, Absolute<0x823280D4u>, Absolute<0x829CD11Cu>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902760);
}

GH_EXTERN(__imp__sub_82902A80);
GH_HOOK(sub_82902A80) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82921F4Cu>, Absolute<0x8232F55Cu>, Absolute<0x829CD180u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902A80);
}

GH_EXTERN(__imp__sub_82902DE0);
GH_HOOK(sub_82902DE0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922638u>, Absolute<0x8233609Cu>, Absolute<0x829CD1F8u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110586880ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82902DE0);
}

GH_EXTERN(__imp__sub_829031B8);
GH_HOOK(sub_829031B8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82922E64u>, Absolute<0x8233CA9Cu>, Absolute<0x829CD268u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829031B8);
}

GH_EXTERN(__imp__sub_829035F0);
GH_HOOK(sub_829035F0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82923440u>, Absolute<0x8233F508u>, Absolute<0x829CD2E4u>, ScratchSet<Scratch<&GuestContext::r11, -2104360960ll>, Scratch<&GuestContext::r10, -2110521344ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_829035F0);
}

GH_EXTERN(__imp__sub_829003D8);
GH_HOOK(sub_829003D8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82913C44u>, Absolute<0x82016EB0u>, Absolute<0x829B2124u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_829003D8);
}

GH_EXTERN(__imp__sub_829004F8);
GH_HOOK(sub_829004F8) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829B2138u>, Absolute<0x8201238Cu>, Absolute<0x829B2148u>, ScratchSet<Scratch<&GuestContext::r11, -2103771136ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_829004F8);
}

GH_EXTERN(__imp__sub_82900738);
GH_HOOK(sub_82900738) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82913CC4u>, Absolute<0x82016F30u>, Absolute<0x829B2460u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113863680ll>, Scratch<&GuestContext::r9, -2103771136ll>>>(ctx, base, __imp__sub_82900738);
}

GH_EXTERN(__imp__sub_82900BF0);
GH_HOOK(sub_82900BF0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x82913FE8u>, Absolute<0x82018D60u>, Absolute<0x829CCC28u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900BF0);
}

GH_EXTERN(__imp__sub_82900DF0);
GH_HOOK(sub_82900DF0) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Add, Absolute<0x829140CCu>, Absolute<0x82018ED0u>, Absolute<0x829CCC68u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82900DF0);
}

GH_EXTERN(__imp__sub_82901008);
GH_HOOK(sub_82901008) {
  GH_PROLOGUE();
  Binary<BinaryOperation::Subtract, Absolute<0x82914B14u>, Absolute<0x8201DFFCu>, Absolute<0x829CCCC8u>, ScratchSet<Scratch<&GuestContext::r11, -2104426496ll>, Scratch<&GuestContext::r10, -2113798144ll>, Scratch<&GuestContext::r9, -2103640064ll>>>(ctx, base, __imp__sub_82901008);
}

