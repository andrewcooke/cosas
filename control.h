
#ifndef COSA_CONTROL_H
#define COSA_CONTROL_H

#include <memory>


class Input {
  
public:
  
  virtual void set(float val) = 0;
  
};


class Delegate : public Input {

public:

  Delegate(std::unique_ptr<Input> del);

protected:

  std::unique_ptr<Input> delegate;
  
};


class Change : public Delegate {

public:

  Change(std::unique_ptr<Input> del);
  void set(float value) override;

private:

  float prev = -1;
  
};


class Sigmoid : public Delegate {

public:

  Sigmoid(std::unique_ptr<Input> del, float lin);
  void set(float value) override;

private:

  // 0 means completely flat at 1/2; 1 is completely linear
  float linear;

};


class Exp : public Delegate {

public:
  
  Exp(std::unique_ptr<Input> del);
  void set(float value) override;

};

#endif
