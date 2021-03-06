#include <iostream>

#include "value_ptr.h"

// =========================================================================================================================================
// == TESTS ================================================================================================================================
// =========================================================================================================================================

std::size_t indent_size = 2;
std::size_t indent_depth = 0;

static void log(char const line[]) {
  for (std::size_t i = 0; i < indent_depth * indent_size; i++) {
    std::cout << " ";
  }
  std::cout << line << std::endl;
}

static void log_up(char const line[]) {
  log(line);
  indent_depth++;
}

static void log_down() {
  indent_depth--;
}

// =========================================================================================================================================

static bool test_fundamental() {
  value_ptr<int> vi1 = new int(1);
  value_ptr<int> vi2 = new int(2);

  value_ptr<int> vi3 = vi1;
  value_ptr<int> vi4 = vi2;

  vi4 = vi1;
  vi3 = vi2;

  auto h = vi1.get_handler(); h = h;

  vi3.get_handler() = vi4.get_handler();

  return true;
}

// =========================================================================================================================================

class Base {
  public:
    Base() { log("Base::Base()"); }
    Base(Base const &) { log("Base::Base(Base const &)"); }
    Base(Base &&) { log("Base::Base(Base &&)"); }
    Base &operator=(Base other) { log_up("Base::operator=(Base)"); swap(other); log_down(); return *this; }
    Base &operator=(Base &&other) { log_up("Base::operator=(Base &&)"); swap(std::move(other)); log_down(); return  *this; }
    virtual ~Base() { log("Base::~Base()"); }

    void swap(Base &) { log("Base::swap(Base &)"); }
    void swap(Base &&) { log("Base::swap(Base &&)"); }

    virtual Base *clone(void *p = nullptr) const {
      Base *ret;
      if (nullptr == p) {
        log_up("Base::clone()"); ret = new Base(*this); log_down();
      } else {
        log_up("Base::clone(void *)"); ret = new(p) Base(*this); log_down();
      }
      return ret;
    }
};

void swap(Base &, Base &);
void swap(Base &, Base &&);
void swap(Base &&, Base &);

void swap(Base &left, Base &right) { left.swap(right); }
void swap(Base &left, Base &&right) { left.swap(std::move(right)); }
void swap(Base &&left, Base &right) { right.swap(std::move(left)); }



class Derived : public Base {
  public:
    Derived() : Base() { log("Derived::Derived()"); }
    Derived(Derived const &other) : Base(other) { log("Derived::Derived(Derived const &)"); }
    Derived(Derived &&other) : Base(std::move(other)) { log("Derived::Derived(Derived &&)"); }
    Derived &operator=(Derived other) { log_up("Derived::operator=(Derived)"); swap(other); log_down(); return *this; }
    Derived &operator=(Derived &&other) { log_up("Derived::operator=(Derived &&)"); swap(std::move(other)); log_down(); return  *this; }
    virtual ~Derived() { log("Derived::~Derived()"); }

    void swap(Derived &) { log("Derived::swap(Derived &)"); }
    void swap(Derived &&) { log("Derived::swap(Derived &&)"); }

    virtual Derived *clone(void *p = nullptr) const {
      Derived *ret;
      if (nullptr == p) {
        log_up("Derived::clone()"); ret = new Derived(*this); log_down();
      } else {
        log_up("Derived::clone(void *)"); ret = new(p) Derived(*this); log_down();
      }
      return ret;
    }
};

void swap(Derived &, Derived &);
void swap(Derived &, Derived &&);
void swap(Derived &&, Derived &);

void swap(Derived &left, Derived &right) { left.swap(right); }
void swap(Derived &left, Derived &&right) { left.swap(std::move(right)); }
void swap(Derived &&left, Derived &right) { right.swap(std::move(left)); }

static bool test_base() {
  log_up("value_ptr<Base> vb1 = new Derived()"); value_ptr<Base> vb1 = new Derived(); log_down();
  log_up("value_ptr<Base> vb2 = new Derived()"); value_ptr<Base> vb2 = new Derived(); log_down();

  log_up("value_ptr<Base> vb3 = vb1"); value_ptr<Base> vb3 = vb1; log_down();
  log_up("value_ptr<Base> vb4 = vb2"); value_ptr<Base> vb4 = vb2; log_down();

  log_up("vb4 = vb1"); vb4 = vb1; log_down();
  log_up("vb3 = vb2"); vb3 = vb2; log_down();

  log_up("vb1.get_handler()"); auto h = vb1.get_handler(); h = h; log_down();

  log_up("vb3.get_handler() = vb4.get_handler()"); vb3.get_handler() = vb4.get_handler(); log_down();

  return true;
}

static bool test_base_array() {
  log_up("value_ptr<Base[]> vb1 = new Derived[5]()"); value_ptr<Base[]> vb1 = new Derived[5](); log_down();
  log_up("value_ptr<Base[]> vb2 = new Derived[5]()"); value_ptr<Base[]> vb2 = new Derived[5](); log_down();

  log_up("value_ptr<Base[]> vb3 = vb1"); value_ptr<Base[]> vb3 = vb1; log_down();
  log_up("value_ptr<Base[]> vb4 = vb2"); value_ptr<Base[]> vb4 = vb2; log_down();

  log_up("vb4 = vb1"); vb4 = vb1; log_down();
  log_up("vb3 = vb2"); vb3 = vb2; log_down();

  log_up("vb1.get_handler()"); auto h = vb1.get_handler(); h = h; log_down();

  log_up("vb3.get_handler() = vb4.get_handler()"); vb3.get_handler() = vb4.get_handler(); log_down();

  return true;
}

// =========================================================================================================================================
// =========================================================================================================================================


using namespace std;


int main(int argc, char *argv[]) {
  cerr << "Arguments:" << endl;
  for (int i = 0; i < argc; i++) {
    cerr << "  " << i << ": " << argv[i] << endl;
  }
  cerr << endl;

  // ---------------------------------------------------------------------------

  cout << "FUNDAMENTAL" << endl; test_fundamental(); cout << endl << endl;
  cout << "BASE"        << endl; test_base()       ; cout << endl << endl;
  cout << "ARRAY"       << endl; test_base_array() ; cout << endl << endl;

  value_ptr<Base[2]> vb = new Base[2]();
  cout << "RESET BEGIN" << endl;
  vb.reset();
  cout << "RESET END" << endl;

  // ---------------------------------------------------------------------------

  return 0;
}

