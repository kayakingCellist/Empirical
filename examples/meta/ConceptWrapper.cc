//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2018.
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  Some examples code for using emp::ConceptWrapper template

#include <iostream>

#include "base/Ptr.h"
#include "meta/ConceptWrapper.h"

EMP_BUILD_CONCEPT( TestConcept, TestConcept_Base,
                   PUBLIC( int x=5; ),
                   REQUIRED_FUN(RequiredFun1, "Missing RequiredFun function 1!", void),
                   REQUIRED_FUN(RequiredFun2, "Missing RequiredFun function 2!", void),
                   OPTIONAL_FUN(OptionalFun1, { std::cout << "Default version of OptionalFun1()" << std::endl; x++; }, void),
                   REQUIRED_FUN(DoMath1, "Missing required function DoMath1", double, double, double ),
                   OPTIONAL_FUN(DoMath2, (arg1 + arg2)/2.0, double, double, double )
                 );


struct MinimalClass {
  void RequiredFun1() { std::cout << "In MinimalClass::RequiredFun1()" << std::endl; }
  void RequiredFun2() { std::cout << "In MinimalClass::RequiredFun2()" << std::endl; }
  // No OptionalFun1 function
  double DoMath1(double arg1, double arg2) { return std::min(arg1, arg2); }
  // No Math2 function (it's optional)
};

struct FullClass {
  void RequiredFun1() { std::cout << "In FullClass::RequiredFun1()" << std::endl; }
  void RequiredFun2() { std::cout << "In FullClass::RequiredFun2()" << std::endl; }
  void OptionalFun1() { std::cout << "In FullClass::OptionalFun2()" << std::endl; }
  double DoMath1(double arg1, double arg2) { return std::max(arg1, arg2); }
  double DoMath2(double arg1, double arg2) { return arg1 * arg2; }

  double DoMath3(double arg1, double arg2) { return arg1 + 5 * arg2; }
};

int main() {
  TestConcept<MinimalClass> min_class;
  min_class.RequiredFun1();
  min_class.RequiredFun2();
  min_class.OptionalFun1();

  TestConcept<FullClass> full_class;
  full_class.RequiredFun1();
  full_class.RequiredFun2();
  full_class.OptionalFun1();

  std::cout << "min_class.x = " << min_class.x << std::endl;
  std::cout << "full_class.x = " << full_class.x << std::endl;
  std::cout << "full_class.DoMath3(2, 4) = " << full_class.DoMath3(2,4) << std::endl;

  std::vector<emp::Ptr<TestConcept_Base>> tests;
  tests.push_back( new TestConcept<MinimalClass> );
  tests.push_back( new TestConcept<MinimalClass> );
  tests.push_back( new TestConcept<FullClass> );
  tests.push_back( new TestConcept<MinimalClass> );
  tests.push_back( new TestConcept<FullClass> );
  tests.push_back( new TestConcept<FullClass> );
  tests.push_back( new TestConcept<MinimalClass> );

  std::cout << "\n--- Array Tests ---\n";
  for (size_t i = 0 ; i < tests.size(); i++) {
    emp::Ptr<TestConcept_Base> tc_ptr = tests[i];
    std::cout << "=> " << i << "\n";
    tc_ptr->RequiredFun1();
    tc_ptr->RequiredFun2();
    tc_ptr->OptionalFun1();
    std::cout << "DoMath1(" << i << "," << 3 << ") = " << tc_ptr->DoMath1(i,3) << "\n";
    std::cout << "DoMath2(" << i << "," << 3 << ") = " << tc_ptr->DoMath2(i,3) << "\n";
  }

  std::cout << std::endl;
  std::cout << "Done!" << std::endl;
}