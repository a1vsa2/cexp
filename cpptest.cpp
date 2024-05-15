#include <iostream>

using std::cout;
using std::endl;

class Base {

public:
    Base() {}
    ~Base() {}
    virtual void baseVFunA() const = 0;
    friend int main();
protected:
    void baseFunB();
    virtual void baseVFunB();


private:
    void baseFunC();
    virtual void baseVFunC();
};

class Child : public Base {

public:
    Child();
    void baseFunB();
    void baseVFunA() const override;
    void baseVFunB() override;
    void baseVFunC() override;
};

void Base::baseFunB() {
    cout << __PRETTY_FUNCTION__ << endl;
}
void Base::baseFunC() {
    cout << __PRETTY_FUNCTION__ << endl;
}

void Base::baseVFunB() {
    cout << __PRETTY_FUNCTION__ << endl;
}
void Base::baseVFunC() {
    cout << __PRETTY_FUNCTION__ << endl;
}
Child::Child() {
   cout << "constructor" << endl;
}
void Child::baseVFunA() const {
    cout << __PRETTY_FUNCTION__ << endl;
}
void Child::baseVFunB() {
    cout << __PRETTY_FUNCTION__ << endl;
}
void Child::baseVFunC() {
    cout << __PRETTY_FUNCTION__ << endl;
}
void Child::baseFunB() {
    //Base::baseFunB();
    // Base::baseVFunC();
    cout << __PRETTY_FUNCTION__ << endl;
}

int main() {
    // Base base;
    Child child;
    Base *base = &child;
    base->baseFunB();
    base->baseVFunA();
    // base.baseVFunA();
    // child.baseVFunA();
    return 0;
}