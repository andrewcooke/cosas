
#ifndef COSA_CONTROL_H
#define COSA_CONTROL_H

#include <memory>


class Input {
  
public:
  
  virtual void set(float val) = 0;
  
};


class Blank : public Input {

public:

  Blank();
  void set(float value) override;

};


class Delegate : public Input {

public:

  Delegate(Input& del);

protected:

  Input& delegate;
  
};


class Change : public Delegate {

public:

  Change(Input& del);
  void set(float value) override;

private:

  float prev = -1;
  
};


class Sigmoid : public Delegate {

public:

  Sigmoid(Input& del, float lin);
  void set(float value) override;

private:

  // 0 means completely flat at 1/2; 1 is completely linear
  float linear;

};


class Exp : public Delegate {

public:
  
  Exp(Input& del);
  void set(float value) override;

};


class Range : public Delegate {

public:

  Range(Input& del, float c, float lo, float hi);
  void set(float value) override;
  virtual float update(float value) = 0;

protected:

  float current;
  
private:

  float low;
  float high;

};
  

class Additive : public Range {

public:
  
  Additive(Input& del, float c, float lo, float hi);
  float update(float value);

};

  
class Multiplicative : public Range {

public:
  
  Multiplicative(Input& del, float c, float lo, float hi);
  float update(float value);

};

  
#endif
