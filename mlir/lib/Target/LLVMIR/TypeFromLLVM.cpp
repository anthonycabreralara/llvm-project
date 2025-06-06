//===- TypeFromLLVM.cpp - type translation from LLVM to MLIR IR -===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "mlir/Target/LLVMIR/TypeFromLLVM.h"
#include "mlir/Dialect/LLVMIR/LLVMTypes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"

#include "llvm/ADT/TypeSwitch.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Type.h"

using namespace mlir;

namespace mlir {
namespace LLVM {
namespace detail {
/// Support for translating LLVM IR types to MLIR LLVM dialect types.
class TypeFromLLVMIRTranslatorImpl {
public:
  /// Constructs a class creating types in the given MLIR context.
  TypeFromLLVMIRTranslatorImpl(MLIRContext &context,
                               bool importStructsAsLiterals)
      : context(context), importStructsAsLiterals(importStructsAsLiterals) {}

  /// Translates the given type.
  Type translateType(llvm::Type *type) {
    if (knownTranslations.count(type))
      return knownTranslations.lookup(type);

    Type translated =
        llvm::TypeSwitch<llvm::Type *, Type>(type)
            .Case<llvm::ArrayType, llvm::FunctionType, llvm::IntegerType,
                  llvm::PointerType, llvm::StructType, llvm::FixedVectorType,
                  llvm::ScalableVectorType, llvm::TargetExtType>(
                [this](auto *type) { return this->translate(type); })
            .Default([this](llvm::Type *type) {
              return translatePrimitiveType(type);
            });
    knownTranslations.try_emplace(type, translated);
    return translated;
  }

private:
  /// Translates the given primitive, i.e. non-parametric in MLIR nomenclature,
  /// type.
  Type translatePrimitiveType(llvm::Type *type) {
    if (type->isVoidTy())
      return LLVM::LLVMVoidType::get(&context);
    if (type->isHalfTy())
      return Float16Type::get(&context);
    if (type->isBFloatTy())
      return BFloat16Type::get(&context);
    if (type->isFloatTy())
      return Float32Type::get(&context);
    if (type->isDoubleTy())
      return Float64Type::get(&context);
    if (type->isFP128Ty())
      return Float128Type::get(&context);
    if (type->isX86_FP80Ty())
      return Float80Type::get(&context);
    if (type->isX86_AMXTy())
      return LLVM::LLVMX86AMXType::get(&context);
    if (type->isPPC_FP128Ty())
      return LLVM::LLVMPPCFP128Type::get(&context);
    if (type->isLabelTy())
      return LLVM::LLVMLabelType::get(&context);
    if (type->isMetadataTy())
      return LLVM::LLVMMetadataType::get(&context);
    if (type->isTokenTy())
      return LLVM::LLVMTokenType::get(&context);
    llvm_unreachable("not a primitive type");
  }

  /// Translates the given array type.
  Type translate(llvm::ArrayType *type) {
    return LLVM::LLVMArrayType::get(translateType(type->getElementType()),
                                    type->getNumElements());
  }

  /// Translates the given function type.
  Type translate(llvm::FunctionType *type) {
    SmallVector<Type, 8> paramTypes;
    translateTypes(type->params(), paramTypes);
    return LLVM::LLVMFunctionType::get(translateType(type->getReturnType()),
                                       paramTypes, type->isVarArg());
  }

  /// Translates the given integer type.
  Type translate(llvm::IntegerType *type) {
    return IntegerType::get(&context, type->getBitWidth());
  }

  /// Translates the given pointer type.
  Type translate(llvm::PointerType *type) {
    return LLVM::LLVMPointerType::get(&context, type->getAddressSpace());
  }

  /// Translates the given structure type.
  Type translate(llvm::StructType *type) {
    SmallVector<Type, 8> subtypes;
    if (type->isLiteral() || importStructsAsLiterals) {
      translateTypes(type->subtypes(), subtypes);
      return LLVM::LLVMStructType::getLiteral(&context, subtypes,
                                              type->isPacked());
    }

    if (type->isOpaque())
      return LLVM::LLVMStructType::getOpaque(type->getName(), &context);

    // With opaque pointers, types in LLVM can't be recursive anymore. Note that
    // using getIdentified is not possible, as type names in LLVM are not
    // guaranteed to be unique.
    translateTypes(type->subtypes(), subtypes);
    LLVM::LLVMStructType translated = LLVM::LLVMStructType::getNewIdentified(
        &context, type->getName(), subtypes, type->isPacked());
    knownTranslations.try_emplace(type, translated);
    return translated;
  }

  /// Translates the given fixed-vector type.
  Type translate(llvm::FixedVectorType *type) {
    return VectorType::get(type->getNumElements(),
                           translateType(type->getElementType()));
  }

  /// Translates the given scalable-vector type.
  Type translate(llvm::ScalableVectorType *type) {
    return VectorType::get(type->getMinNumElements(),
                           translateType(type->getElementType()),
                           /*scalableDims=*/true);
  }

  /// Translates the given target extension type.
  Type translate(llvm::TargetExtType *type) {
    SmallVector<Type> typeParams;
    translateTypes(type->type_params(), typeParams);

    return LLVM::LLVMTargetExtType::get(&context, type->getName(), typeParams,
                                        type->int_params());
  }

  /// Translates a list of types.
  void translateTypes(ArrayRef<llvm::Type *> types,
                      SmallVectorImpl<Type> &result) {
    result.reserve(result.size() + types.size());
    for (llvm::Type *type : types)
      result.push_back(translateType(type));
  }

  /// Map of known translations. Serves as a cache and as recursion stopper for
  /// translating recursive structs.
  llvm::DenseMap<llvm::Type *, Type> knownTranslations;

  /// The context in which MLIR types are created.
  MLIRContext &context;

  /// Controls if structs should be imported as literal structs, i.e., nameless
  /// structs.
  bool importStructsAsLiterals;
};

} // namespace detail
} // namespace LLVM
} // namespace mlir

LLVM::TypeFromLLVMIRTranslator::TypeFromLLVMIRTranslator(
    MLIRContext &context, bool importStructsAsLiterals)
    : impl(std::make_unique<detail::TypeFromLLVMIRTranslatorImpl>(
          context, importStructsAsLiterals)) {}

LLVM::TypeFromLLVMIRTranslator::~TypeFromLLVMIRTranslator() = default;

Type LLVM::TypeFromLLVMIRTranslator::translateType(llvm::Type *type) {
  return impl->translateType(type);
}
