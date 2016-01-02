#include <iostream>

#include "value_ptr.hpp"

using namespace std;

class A {
  public:
    virtual A *clone() const;
};

class B : public A {};

class C {};

int main(int argc, char *argv[]) {
  cerr << "Arguments:" << endl;
  for (int i = 0; i < argc; i++) {
    cerr << "  " << i << ": " << argv[i] << endl;
  }
  cerr << endl;

  value_ptr<long> ll(new long(123));
  if (ll) {
    cout << "LL TRUE" << endl;
  } else {
    cout << "LL FALSE" << endl;
  }

  cout << is_cloneable<A>::value << endl;
  cout << is_cloneable<B>::value << endl;

  return 0;
}

