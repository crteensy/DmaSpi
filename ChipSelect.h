#ifndef CHIPSELECT_H
#define CHIPSELECT_H

#include <core_pins.h>

/** A abstract class that provides a chip select interface
**/

class AbstractChipSelect
{
	public:
    virtual void select() = 0;
    virtual void deselect() = 0;
		virtual ~AbstractChipSelect() {}
};

class DummyChipSelect : public AbstractChipSelect
{
  void select() override {}
  void deselect() override {}
};

class DebugChipSelect : public AbstractChipSelect
{
  void select() override {Serial.println("Dummy CS: select()");}
  void deselect() override {Serial.println("Dummy CS: deselect()");}
};

/** An active low chip select class. This also configures the pin once.
**/
class ActiveLowChipSelect : public AbstractChipSelect
{
  public:
    ActiveLowChipSelect(const unsigned int& pin, const SPISettings& settings)
      : pin_(pin),
      settings_(settings)
    {
      pinMode(pin, OUTPUT);
      digitalWriteFast(pin, 1);
    }
    void select() override
    {
      SPI.beginTransaction(settings_);
      digitalWriteFast(pin_, 0);
    }
    void deselect() override
    {
      digitalWriteFast(pin_, 1);
      SPI.endTransaction();
    }
  private:
    const unsigned int pin_;
    const SPISettings& settings_;

};

#endif // CHIPSELECT_H

