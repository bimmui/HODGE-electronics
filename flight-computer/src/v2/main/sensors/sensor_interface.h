
#include <inttypes.h>
#include "sensor_types.h"

struct sensor_reading
{
    sensor_value value;
    uint32_t timestamp;
    sensor_status status;
};

class ApoSensor
{
public:
    virtual ~ApoSensor() = default;
    virtual sensor_status initialize() = 0;
    virtual sensor_reading read() = 0;
    virtual sensor_type getType() const = 0;
    virtual uint8_t getDevID() = 0;

private:
    virtual void configure() = 0;
};