
#define GPIO_DS18B20_0      4
#define MAX_DEVICES         8
#define DS18B20_RESOLUTION  DS18B20_RESOLUTION_12_BIT
#define SAMPLE_PERIOD       1000   // milliseconds
#define GPIO_PHOTORESISTOR  2

// ADC vars and consts
#define DEFAULT_VREF    1100        // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          // Multisampling
#define R               10000       // Resistor in Ohms
#define VIN             3300        // Input voltage in mV

int raw_to_lumens(int raw);
static void check_efuse(void);
static void print_char_val_type(esp_adc_cal_value_t val_type);
void light_intensity_task(void *pvParameter);
void onewire_task(void *pvParameter);