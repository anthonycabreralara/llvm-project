//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef _LIBCPP___CXX03___ALGORITHM_MAKE_PROJECTED_H
#define _LIBCPP___CXX03___ALGORITHM_MAKE_PROJECTED_H

#include <__cxx03/__config>
#include <__cxx03/__functional/identity.h>
#include <__cxx03/__type_traits/decay.h>
#include <__cxx03/__type_traits/enable_if.h>
#include <__cxx03/__type_traits/integral_constant.h>
#include <__cxx03/__type_traits/invoke.h>
#include <__cxx03/__type_traits/is_member_pointer.h>
#include <__cxx03/__type_traits/is_same.h>
#include <__cxx03/__utility/declval.h>
#include <__cxx03/__utility/forward.h>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

_LIBCPP_BEGIN_NAMESPACE_STD

template <class _Pred, class _Proj>
struct _ProjectedPred {
  _Pred& __pred; // Can be a unary or a binary predicate.
  _Proj& __proj;

  _LIBCPP_HIDE_FROM_ABI _ProjectedPred(_Pred& __pred_arg, _Proj& __proj_arg) : __pred(__pred_arg), __proj(__proj_arg) {}

  template <class _Tp>
  typename __invoke_of<_Pred&, decltype(std::__invoke(std::declval<_Proj&>(), std::declval<_Tp>()))>::type
      _LIBCPP_HIDE_FROM_ABI
      operator()(_Tp&& __v) const {
    return std::__invoke(__pred, std::__invoke(__proj, std::forward<_Tp>(__v)));
  }

  template <class _T1, class _T2>
  typename __invoke_of<_Pred&,
                       decltype(std::__invoke(std::declval<_Proj&>(), std::declval<_T1>())),
                       decltype(std::__invoke(std::declval<_Proj&>(), std::declval<_T2>()))>::type _LIBCPP_HIDE_FROM_ABI
  operator()(_T1&& __lhs, _T2&& __rhs) const {
    return std::__invoke(
        __pred, std::__invoke(__proj, std::forward<_T1>(__lhs)), std::__invoke(__proj, std::forward<_T2>(__rhs)));
  }
};

template <
    class _Pred,
    class _Proj,
    __enable_if_t<!(!is_member_pointer<__decay_t<_Pred> >::value && __is_identity<__decay_t<_Proj> >::value), int> = 0>
_LIBCPP_HIDE_FROM_ABI _ProjectedPred<_Pred, _Proj> __make_projected(_Pred& __pred, _Proj& __proj) {
  return _ProjectedPred<_Pred, _Proj>(__pred, __proj);
}

// Avoid creating the functor and just use the pristine comparator -- for certain algorithms, this would enable
// optimizations that rely on the type of the comparator. Additionally, this results in less layers of indirection in
// the call stack when the comparator is invoked, even in an unoptimized build.
template <
    class _Pred,
    class _Proj,
    __enable_if_t<!is_member_pointer<__decay_t<_Pred> >::value && __is_identity<__decay_t<_Proj> >::value, int> = 0>
_LIBCPP_HIDE_FROM_ABI _Pred& __make_projected(_Pred& __pred, _Proj&) {
  return __pred;
}

_LIBCPP_END_NAMESPACE_STD

#endif // _LIBCPP___CXX03___ALGORITHM_MAKE_PROJECTED_H
