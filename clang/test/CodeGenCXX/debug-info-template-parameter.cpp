// Test for DebugInfo for Defaulted parameters for C++ templates
// Supported: -O0, standalone DI

// RUN: %clang_cc1 -emit-llvm %std_cxx11-14 -dwarf-version=5 -triple x86_64 %s -O0 -disable-llvm-passes -debug-info-kind=standalone -o - | FileCheck %s --check-prefixes=CHECK,PRE17
// RUN: %clang_cc1 -emit-llvm %std_cxx17- -dwarf-version=5 -triple x86_64 %s -O0 -disable-llvm-passes -debug-info-kind=standalone -o - | FileCheck %s --check-prefixes=CHECK,CXX17
// RUN: %clang_cc1 -emit-llvm %std_cxx17- -dwarf-version=4 -triple x86_64 %s -O0 -disable-llvm-passes -debug-info-kind=standalone -o - | FileCheck %s --check-prefixes=CHECK,CXX17
// RUN: %clang_cc1 -emit-llvm %std_cxx20- -dwarf-version=5 -DCXX20=1 -triple x86_64 %s -O0 -disable-llvm-passes -debug-info-kind=standalone -o - | FileCheck %s --check-prefix=CHECK-CXX20

// CHECK: DILocalVariable(name: "f1", {{.*}}, type: ![[TEMPLATE_TYPE:[0-9]+]]
// CHECK: [[TEMPLATE_TYPE]] = {{.*}}!DICompositeType({{.*}}, templateParams: ![[F1_TYPE:[0-9]+]]
// CHECK: [[F1_TYPE]] = !{![[FIRST:[0-9]+]], ![[SECOND:[0-9]+]], ![[THIRD:[0-9]+]], ![[FORTH:[0-9]+]], ![[FIFTH:[0-9]+]]}
// CHECK: [[FIRST]] = !DITemplateTypeParameter(name: "T", type: !{{[0-9]*}})
// CHECK: [[SECOND]] = !DITemplateValueParameter(name: "i", type: !{{[0-9]*}}, value: i32 6)
// PRE17: [[THIRD]] = !DITemplateValueParameter(name: "b", type: !{{[0-9]*}}, value: i8 0)
// CXX17: [[THIRD]] = !DITemplateValueParameter(name: "b", type: !{{[0-9]*}}, value: i1 false)
// CHECK: [[FIFTH]] = !DITemplateTypeParameter(name: "d", type: !{{[0-9]*}})

// CHECK: DILocalVariable(name: "f2", {{.*}}, type: ![[TEMPLATE_TYPE:[0-9]+]]
// CHECK: [[TEMPLATE_TYPE]] = {{.*}}!DICompositeType({{.*}}, templateParams: ![[F2_TYPE:[0-9]+]]
// CHECK: [[F2_TYPE]] = !{![[FIRST:[0-9]+]], ![[SECOND:[0-9]+]], ![[THIRD:[0-9]+]], ![[FORTH:[0-9]+]], ![[FIFTH:[0-9]+]]}
// CHECK: [[FIRST]] = !DITemplateTypeParameter(name: "T", type: !{{[0-9]*}}, defaulted: true)
// CHECK: [[SECOND]] = !DITemplateValueParameter(name: "i", type: !{{[0-9]*}}, defaulted: true, value: i32 3)
// PRE17: [[THIRD]] = !DITemplateValueParameter(name: "b", type: !{{[0-9]*}}, defaulted: true, value: i8 1)
// CXX17: [[THIRD]] = !DITemplateValueParameter(name: "b", type: !{{[0-9]*}}, defaulted: true, value: i1 true)
// CHECK: [[FIFTH]] = !DITemplateTypeParameter(name: "d", type: !{{[0-9]*}}, defaulted: true)

// CHECK: DILocalVariable(name: "b1", {{.*}}, type: ![[TEMPLATE_TYPE:[0-9]+]]
// CHECK: [[TEMPLATE_TYPE]] = {{.*}}!DICompositeType({{.*}}, templateParams: ![[B1_TYPE:[0-9]+]]
// CHECK: [[B1_TYPE]] = !{![[FIRST:[0-9]+]]}
// CHECK: [[FIRST]] = !DITemplateValueParameter(tag: DW_TAG_GNU_template_template_param, name: "CT", value: !"qux")

// CHECK: DILocalVariable(name: "b2", {{.*}}, type: ![[TEMPLATE_TYPE:[0-9]+]]
// CHECK: [[TEMPLATE_TYPE]] = {{.*}}!DICompositeType({{.*}}, templateParams: ![[B2_TYPE:[0-9]+]]
// CHECK: [[B2_TYPE]] = !{![[FIRST:[0-9]+]]}
// CHECK: [[FIRST]] = !DITemplateValueParameter(tag: DW_TAG_GNU_template_template_param, name: "CT", defaulted: true, value: !"bar")

template <typename T>
class bar {
};

template <typename T>
class qux {
};

template <typename T = char, int i = 3, bool b = true, int x = sizeof(T),
          typename d = bar<T>>
class foo {
};

template <template <typename T> class CT = bar>
class baz {
};

#ifdef CXX20
struct non_empty { int mem; int mem2; } ne;

template<float f = -1.5f, double d = 5.2, int * p = &ne.mem2>
class nttp {};
nttp<> n1;

// CHECK-CXX20:      DIGlobalVariable(name: "n1", {{.*}}, type: ![[NTTP_TYPE:[0-9]+]]
// CHECK-CXX20:      [[NTTP_TYPE]] = {{.*}}!DICompositeType({{.*}}name: "nttp
// CHECK-CXX20-SAME:                                        templateParams: ![[NTTP_TEMPLATES:[0-9]+]]
// CHECK-CXX20:      [[NTTP_TEMPLATES]] = !{![[FIRST:[0-9]+]], ![[SECOND:[0-9]+]], ![[THIRD:[0-9]+]]}
// CHECK-CXX20:      [[FIRST]] = !DITemplateValueParameter(name: "f"
// CHECK-CXX20-SAME:                                       defaulted: true
// CHECK-CXX20-SAME:                                       value: float -1.500000e+00
// CHECK-CXX20:      [[SECOND]] = !DITemplateValueParameter(name: "d"
// CHECK-CXX20-SAME:                                        defaulted: true
// CHECK-CXX20-SAME:                                        value: double 5.200000e+00
// CHECK-CXX20:      [[THIRD]] = !DITemplateValueParameter(name: "p"
// CHECK-CXX20-SAME:                                       defaulted: true
// CHECK-CXX20-SAME:                                       value: ptr getelementptr (i8, ptr @ne, i64 4)

#endif // CXX20

int main() {
  foo<int, 6, false, 3, double> f1;
  foo<> f2;
  baz<qux> b1;
  baz<> b2;
  return 0;
}
