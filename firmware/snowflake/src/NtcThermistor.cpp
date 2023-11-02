#include "NtcThermistor.h"
#include "application.h"

typedef struct {
    int temperature;
    float resistanceScale;
} TemperatureToResistance;

static constexpr TemperatureToResistance temperatureToResistance[] = {
    {-40, 40.49},
    {-35, 28.81},
    {-30, 20.72},
    {-25, 15.07},
    {-20, 11.06},
    {-15, 8.198},
    {-10, 6.129},
    {-5, 4.622},
    {0, 3.515},
    {5, 2.694},
    {10, 2.080},
    {15, 1.618},
    {20, 1.267},
    {25, 1.0},
    {30, 0.7944},
    {35, 0.6350},
    {40, 0.5108},
    {45, 0.4132},
    {50, 0.3363},
    {55, 0.2752},
    {60, 0.2263},
    {65, 0.1871},
    {70, 0.1554},
    {75, 0.1297},
    {80, 0.1087},
    {85, 0.09153},
    {90, 0.07738},
    {95, 0.06567},
    {100, 0.05596},
    {105, 0.04786},
    {110, 0.04108},
    {115, 0.03539},
    {120, 0.03059},
    {125, 0.02652},
    {130, 0.02307},
    {135, 0.02013},
    {140, 0.01762},
    {145, 0.01546},
    {150, 0.01361}
};

float ntcGetTemperature() {
    static constexpr int NTC_PIN = A6;

    int adcValue = 0;
    for (int i = 0; i < 100; i++) {
        adcValue += static_cast<int>(analogRead(NTC_PIN));
    }
    float ntcRisistanceVoltage = adcValue / 100.0 / 4096.0 * 5.0;
    float ntcRisistanceScale = ntcRisistanceVoltage / (3.3 - ntcRisistanceVoltage);

    size_t size = sizeof(temperatureToResistance) / sizeof(temperatureToResistance[0]);
    if (ntcRisistanceScale >= temperatureToResistance[0].resistanceScale) {
        return temperatureToResistance[0].temperature;
    }
    if (ntcRisistanceScale <= temperatureToResistance[size - 1].resistanceScale) {
        return temperatureToResistance[size - 1].temperature;
    }

    for (size_t i = 1; i < sizeof(temperatureToResistance) / sizeof(temperatureToResistance[0]); i++) {
        if (ntcRisistanceScale < temperatureToResistance[i - 1].resistanceScale && ntcRisistanceScale >= temperatureToResistance[i].resistanceScale) {
            float deltaResistance = temperatureToResistance[i].resistanceScale - temperatureToResistance[i - 1].resistanceScale;
            float deltaTemperature = temperatureToResistance[i].temperature - temperatureToResistance[i - 1].temperature;
            float temperature = temperatureToResistance[i - 1].temperature + (ntcRisistanceScale - temperatureToResistance[i - 1].resistanceScale) * (deltaTemperature / deltaResistance);
            return temperature;
        }
    }

    return 404; // If not found, return a default value
}