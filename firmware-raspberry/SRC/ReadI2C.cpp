#include <iostream>
#include "I2C_HIH_Measurement.h"
#include "ReadI2C.h"

using namespace std ;
using namespace Raspberry_Gruppo_1 ;


void CReadI2C::operator()()
{
    CI2C_HIH_Measurement i2c_HIH_Measurement ;
    i2c_HIH_Measurement.ReadInternalHumidityAndInternalTemperature() ;
}
