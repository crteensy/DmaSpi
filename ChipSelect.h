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
template<unsigned int pin>
class ActiveLowChipSelect : public AbstractChipSelect
{
  public:
    ActiveLowChipSelect()
    {
      static Init init; // configure the pin
    }
    void select() override
    {
      digitalWriteFast(pin, 0);
    }
    void deselect() override
    {
      digitalWriteFast(pin, 1);
    }
  private:
    /** Configures a pin as output, high **/
    class Init
    {
      public:
        Init()
        {
          pinMode(pin, OUTPUT);
          digitalWriteFast(pin, 1);
        }
    };

//    static Init m_init;
};

//template<unsigned int pin>
//typename ActiveLowChipSelect<pin>::Init ActiveLowChipSelect<pin>::m_init;


#endif // CHIPSELECT_H

