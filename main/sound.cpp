#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "sdkconfig.h"
#include "esp_event.h"
#include "driver/mcpwm.h"
#include "math.h"
#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#define GPIO_PWM0A_OUT 15 //Set GPIO 15 as PWM0A
#define GPIO_PWM0B_OUT 16 //Set GPIO 16 as PWM0B

static void pwm_thread(void *arg);

static const char *TAG = "example";

static double tone_map[] = {
    261.6255653,
    277.182631,
    293.6647679,
    311.1269837,
    329.6275569,
    349.2282314,
    369.9944227,
    391.995436,
    415.3046976,
    440,
    466.1637615,
    493.8833013,

    523.2511306,
    554.365262,
    587.3295358,
    622.2539674,
    659.2551138,
    698.4564629,
    739.9888454,
    783.990872,
    830.6093952,
    880,
    932.327523,
    987.7666025,

    1046.502261,
    1108.730524,
    1174.659072,
    1244.507935,
    1318.510228,
    1396.912926,
    1479.977691,
    1567.981744,
    1661.21879,
    1760,
    1864.655046,
    1975.533205,

    0};

#define M_1 12
#define M_2 14
#define M_3 16
#define M_4 17
#define M_5 19
#define M_6 21
#define M_7 23

#define L_1 0
#define L_2 2
#define L_3 4
#define L_4 5
#define L_5 7
#define L_6 9
#define L_7 11

#define H_1 24
#define H_2 26
#define H_3 28
#define H_4 29
#define H_5 31
#define H_6 33
#define H_7 35

#define EMPTY 36

struct yinjie
{
        int tone;
        int duration;

        yinjie(int tone) : tone(tone), duration(350000) {}
        yinjie(int tone, int duration) : tone(tone), duration(duration) {}
};

static float freq = 2;
static int64_t last_time_since_tone_change = 0;

void update_tone(void *)
{
        static const yinjie yuepu[] = {
            {
                M_1,
            },
            {M_2},
            {M_3},
            {M_1},
            {EMPTY, 50000},
            {M_1},
            {M_2},
            {M_3},
            {M_1},
            {EMPTY, 50000},
            {M_3},
            {M_4},
            {M_5},
            {EMPTY},
            {M_3},
            {M_4},
            {M_5},
            {EMPTY},

            { M_5, 350000/2},
            { M_6, 350000/2},
            { M_5, 350000/2},
            { M_4, 350000/2},
            { M_3},
            { M_1},
            {EMPTY, 50000},

            { M_5, 350000/2},
            { M_6, 350000/2},
            { M_5, 350000/2},
            { M_4, 350000/2},
            { M_3},
            { M_1},
            {EMPTY, 50000},

            { M_1},
            { L_5},
            { M_1},
            {EMPTY},

            { M_1},
            { L_5},
            { M_1},
            {EMPTY},
            {EMPTY},
            {EMPTY},
        };

        int i = 0;

        for (;;)
        {
                freq = tone_map[yuepu[i].tone];

                ESP_LOGI(TAG, "play %d", yuepu[i].tone);

                usleep(yuepu[i].duration);
                last_time_since_tone_change = esp_timer_get_time();
                i++;
                if (i >= (sizeof(yuepu) / sizeof(yinjie)) )
                        i = 0;
        }
}

extern "C" void app_main(void)
{
        ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, GPIO_PWM0A_OUT));
        ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, GPIO_PWM0B_OUT));

        mcpwm_config_t pwm_config;
        pwm_config.frequency = 96000;               //frequency = 192kHz,
        pwm_config.cmpr_a = 50;                     //initial duty cycle of PWMxA = 0
        pwm_config.cmpr_b = 50;                     //initial duty cycle of PWMxb = 0
        pwm_config.counter_mode = MCPWM_UP_DOWN_COUNTER; //up counting mode
        pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
        mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config); //Configure PWM0A & PWM0B with above settings
        mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
        mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);

        ESP_ERROR_CHECK(esp_event_loop_create_default());

        xTaskCreatePinnedToCore(update_tone, "update_tone", 2048, NULL, uxTaskPriorityGet(NULL), NULL, 1);
        last_time_since_tone_change = esp_timer_get_time();
        xTaskCreatePinnedToCore(pwm_thread, "pwm_thread", 2048, NULL, uxTaskPriorityGet(NULL), NULL, 0);
        ESP_LOGI(TAG, "task created");
}

static void pwm_thread(void *arg)
{
        for (;;)
        {
                auto time_point = esp_timer_get_time() - last_time_since_tone_change;

                if (freq == 0)
                {
                        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 0);
                        mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, 0);
                }
                else
                {
                        for (;;time_point = esp_timer_get_time() - last_time_since_tone_change)
                        {
                                TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
                                TIMERG0.wdt_feed = 1;
                                TIMERG0.wdt_wprotect = 0;

                                double wave_T = 1000000.0 / freq;

                                float duty_cycle = sin((time_point / wave_T) * 3.1415926535897932384626) / 2;

                                duty_cycle /= 3;

                               // ESP_LOGI(TAG, "duty %f, freq %f", duty_cycle, freq);
                                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty_cycle * 50 + 50 );
                                mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, 50 - duty_cycle * 50 );

                                if ( time_point >= wave_T)
                                {
                                        if ( abs(duty_cycle) < 0.03)
                                                break;                                        
                                }
                        }
                }

                TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
                TIMERG0.wdt_feed = 1;
                TIMERG0.wdt_wprotect = 0;
        }
}
