#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include "display.h"
#include "i2c_custom.h"
#include <math.h>



int main(){
    stdio_init_all();
    // sleep_ms(10000);
    printf("Code starts\n\n");
    i2c_setup();
    printf("Port Enabled\n");
    AHT10_setup();
    BMP180_setup();
    double temperature=BMP180_Temperature();
    printf("\n True temp: %lf",temperature/10);
    double pressure=BMP180_Pressure();
    printf("\nPressure: %lf",pressure);    
    struct AHT_ret out=read_temp_humidity();
    double altitude=altitude_calc(pressure);

    //run through the complete initialization process
    SSD1306_init();

    //Initialize render area for entire frame (SSD1306_WIDTH pixels by SSD1306_NUM_PAGES pages)
    struct render_area frame_area = {
        start_col: 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
        };

    calc_render_area_buflen(&frame_area);

    // zero the entire display
    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

    // intro sequence: flash the screen 3 times
    
    SSD1306_send_cmd(SSD1306_SET_ALL_ON);    // Set all pixels on
    sleep_ms(500);
    SSD1306_send_cmd(SSD1306_SET_ENTIRE_ON); // go back to following RAM for pixel state
    sleep_ms(500);

    char temp_AHT[10];
    char temp_BMP[10];
    char hum[10];
    char BMP_press[10];
    char alt[10];

    
    sprintf(temp_BMP,"\t%5.5lf",temperature/10);
    sprintf(hum,"\t%5.5lf ",out.hum);
    sprintf(BMP_press,"\t%5.5lf",pressure);
    sprintf(alt,"\t%5.5lf",altitude);

    printf(">>>>%s %s %s %s<<<<",temp_AHT,temp_BMP,hum,BMP_press);

    char *text[] = {
        "Temperature AHT",
        "\n",
        "\n",
        temp_AHT,
        "Temperature BMP",
        "\n",
        "\n",
        temp_BMP,
        "Humidity ",
        "\n",
        "\n",
        hum,
        "Pressure ",
        "\n",
        "\n",
        BMP_press,
        "Altitude ",
        "\n",
        "\n",
        alt
    };

    int y = 0;
    for(int j=0;j<5;j++){
        memset(buf, 0, SSD1306_BUF_LEN);
        y = 0;
        for (uint i = j*4;i < 4*(j+1); i++) {
            WriteString(buf, 5, y, text[i]);
            y+=8;
        }
        render(buf, &frame_area);
        sleep_ms(3000);
    }

    return 0;
}
