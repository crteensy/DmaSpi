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
      Serial.printf("Selecting on pin %02u\n", pin_);
      applySpiSettings(settings_);
      digitalWriteFast(pin_, 0);
    }
    void deselect() override
    {
      Serial.printf("Deselecting on pin %02u\n", pin_);
      applySpiSettings(SPISettings());
      digitalWriteFast(pin_, 1);
    }
  private:
    void applySpiSettings(const SPISettings& settings)
    {
      SPI.endTransaction();
      SPI.beginTransaction(settings);
    }
    const unsigned int pin_;
    const SPISettings& settings_;

};

#endif // CHIPSELECT_H

