#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/binary_info.h"
#include<math.h>

#ifndef I2C_CUSTOM_H
#define I2C_CUSTOM_H

#define MPU_I2C i2c0 
#define MPU_I2C_SCL_GPIO 9
#define MPU_I2C_SCL_PIN 12
#define MPU_I2C_SDA_GPIO 8
#define MPU_I2C_SDA_PIN 11

#define AHT10 0x38
#define AHT_INIT 0xE1
#define AHT_MEASURE 0xAC
#define AHT_RESET 0xBA

#define BMP180 0x77
#define BMP_EEPROM_ADDR 0xAA
#define MSB_ADDR 0xF6
#define LSB_ADDR 0xF7
#define XLSB_ADDR 0xF8
#define BMP180_UP_REG  0xF4
#define BMP180_OSS 0
#define BMP180_UP_DATA (0x34+(BMP180_OSS<<6))
#define BMP180_UT_REG  0xF4
#define BMP180_UT_DATA 0x2E

#define BMP180_CALIB_EEPROM_SIZEW 11
#define BMP180_CALIB_EEPROM_SIZEB 22

#define word uint16_t
#define byte uint8_t


volatile double temperature =0;
volatile double pressure=0;
volatile double humidity=0;

struct AHT_ret{
    double temp;
    double hum;
};

enum calib_reg_t{CALIB_AC1=0,CALIB_AC2,CALIB_AC3,CALIB_AC4,CALIB_AC5,CALIB_AC6,CALIB_B1,CALIB_B2,CALIB_MB,CALIB_MC,CALIB_MD};

struct C_reg{
  int16_t AC1,AC2,AC3;
  uint16_t AC4,AC5,AC6;
  int16_t C_B1,C_B2,MB,MC,MD;
};

union eeprom_t{
  word calibW[BMP180_CALIB_EEPROM_SIZEW];
  byte calibB[BMP180_CALIB_EEPROM_SIZEB];
  struct C_reg calibReg;
};

union eeprom_t calibParam;
double B5; 

void BEtoLEConvert(word* wBE){
    *wBE=((*wBE&0xFF)<<8)|(*wBE>>8);
}

int i2c_setup(){
    printf("i2c setup\n");

    i2c_init(MPU_I2C, 100 * 1000);
    gpio_set_function(MPU_I2C_SCL_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(MPU_I2C_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(MPU_I2C_SCL_GPIO);
    gpio_pull_up(MPU_I2C_SDA_GPIO);
    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(MPU_I2C_SDA_PIN, MPU_I2C_SCL_PIN, GPIO_FUNC_I2C));
    return 0;
}

int AHT10_setup(){
    uint8_t ret[8];
    uint8_t i;
    for(i=0x00;i<0x7F;i++){
        if(i2c_read_timeout_us(MPU_I2C, i, ret, 1, false,1000)>0){
            printf("%02x - ",i);
            printf("Slave detected\n");
        }
    }
    if(i2c_read_timeout_us(MPU_I2C, AHT10, ret, 5, false,1000)>0){
        printf("Slave detected\n");
        if(i2c_write_blocking(MPU_I2C,AHT10,ret,1,false)){
            printf("Initialized AHT10\n");
            return 0;
        }
    }
    
    ret[0]=AHT_RESET;
    i2c_write_blocking(MPU_I2C,AHT10,ret,1,false);
    sleep_ms(100);
    return 0;
}

void BMP180_setup(){
    uint8_t retVal[8];
    retVal[0]=BMP_EEPROM_ADDR;
    i2c_write_blocking(MPU_I2C,BMP180,retVal,1,false);
    i2c_read_blocking(MPU_I2C,BMP180,calibParam.calibB,22,false);

    for(int i=0;i<11;i++){
        //printf("\nBE: %04X",calibParam.calibW[i]);
        BEtoLEConvert(&calibParam.calibW[i]);
        // printf("\tLE: %04X",calibParam.calibW[i]);
        //printf("\tData: %d",(int16_t)calibParam.calibW[i]);
    }
    // printf("\nAC1 : %hd: ",calibParam.calibReg.AC1);
    // printf("\nAC2 : %hd: ",calibParam.calibReg.AC2);
    // printf("\nAC3 : %hd: ",calibParam.calibReg.AC3);
    // printf("\nAC4 : %hu: ",calibParam.calibReg.AC4);
    // printf("\nAC5 : %hu: ",calibParam.calibReg.AC5);
    // printf("\nAC6 : %hu: ",calibParam.calibReg.AC6);
    // printf("\nB1 : %hd: ",calibParam.calibReg.C_B1);
    // printf("\nB2 : %hd: ",calibParam.calibReg.C_B2);
    // printf("\nMB : %hd: ",calibParam.calibReg.MB);
    // printf("\nMC : %hd: ",calibParam.calibReg.MC);
    // printf("\nMD : %hd: ",calibParam.calibReg.MD);
}

void display_setup(){

}

double BMP180_Temperature(){
    uint8_t retVal[8];
    uint8_t tempMSB,tempLSB;
    retVal[0]=BMP180_UT_REG;
    retVal[1]=BMP180_UT_DATA;
    i2c_write_blocking(MPU_I2C,BMP180,retVal,2,false);
    sleep_ms(10);
    retVal[0]=MSB_ADDR;
    i2c_write_blocking(MPU_I2C,BMP180,retVal,1,false);
    i2c_read_blocking(MPU_I2C,BMP180,&tempMSB,1,false);
    retVal[0]=LSB_ADDR;
    i2c_write_blocking(MPU_I2C,BMP180,retVal,1,false);
    i2c_read_blocking(MPU_I2C,BMP180,&tempLSB,1,false);

    //Calculation
    uint16_t UTval=((uint16_t)tempMSB<<8)|tempLSB;
    //printf("\nUT : %04X",UTval);
    uint32_t X1_temp=(((uint32_t)UTval-(uint32_t)calibParam.calibReg.AC6)*(uint32_t)calibParam.calibReg.AC5);
    double X1=(double)X1_temp/pow(2,15);
    double X2=((double)calibParam.calibReg.MC*pow(2,11))/(X1+(double)calibParam.calibReg.MD);
    //printf("\n X1:%lf X2:%lf",X1,X2);
    B5 = X1+ X2;
    // printf("\nX1 %lf",X1);
    // printf("\nX2 %lf",X2);
    // printf("\nB5 %lf",B5);
    double trueTemp=(B5+8.0)/pow(2,4);
    return trueTemp;
}

double BMP180_Pressure(){
    uint8_t retVal[8];
    uint8_t MSB,LSB,XLSB;

    retVal[0]=BMP180_UP_REG;
    retVal[1]=BMP180_UP_DATA;
    i2c_write_blocking(MPU_I2C,BMP180,retVal,2,false);
    sleep_ms(10);
    retVal[0]=MSB_ADDR;
    i2c_write_blocking(MPU_I2C,BMP180,retVal,1,false);
    i2c_read_blocking(MPU_I2C,BMP180,&MSB,1,false);
    retVal[0]=LSB_ADDR;
    i2c_write_blocking(MPU_I2C,BMP180,retVal,1,false);
    i2c_read_blocking(MPU_I2C,BMP180,&LSB,1,false);
    retVal[0]=XLSB_ADDR;
    i2c_write_blocking(MPU_I2C,BMP180,retVal,1,false);
    i2c_read_blocking(MPU_I2C,BMP180,&XLSB,1,false);

    //Calculation
    uint32_t UPval=0;
    UPval=(((uint32_t)MSB<<16)|((uint32_t)LSB<<8)|((uint32_t)XLSB))>>(8-BMP180_OSS);
    printf("\nUPVal: %u\n", UPval);
    double B6=B5-4000.0;
    double X1=((double)calibParam.calibReg.C_B2*(B6*B6/pow(2,12)))/pow(2,11);
    double X2=(double)calibParam.calibReg.AC2*B6/pow(2,11);
    double X3=X1+X2;
    // printf("\nX1 %lf",X1);
    // printf("\nX2 %lf",X2);
    // printf("\nX3 %lf",X3);
    double B3=(((((double)calibParam.calibReg.AC1*4.0)+X3)*pow(2,BMP180_OSS))+2.0)/4.0;
    X1 = (double)calibParam.calibReg.AC3*B6/pow(2,13);
    X2 = ((double)calibParam.calibReg.C_B1*(B6*B6/pow(2,12)))/pow(2,16);
    X3 = ((X1+X2)+(double)2)/(double)4;
    double B4= (double)calibParam.calibReg.AC4*(unsigned long)(X3+32768.0)/pow(2,15);
    double B7=((unsigned long)((double)UPval-B3))*(50000>>BMP180_OSS);
    double p;
    // printf("\nB6 %lf",B6);
    // printf("\nX1 %lf",X1);
    // printf("\nX2 %lf",X2);
    // printf("\nX3 %lf",X3);
    // printf("\nB3 %lf",B3);
    // printf("\nB4 %lf",B4);
    // printf("\nB7 %lf",B7);
    if(B7<0x80000000){
        p= (B7*2.0)/B4;
    }
    else{
        p=(B7/B4)*2;
    }
    // printf("\np: %lf",p);
    X1=(p/pow(2,8))*(p/pow(2,8));
    X1=(X1*3038.0)/pow(2,16);
    X2=(-7357.0*p)/pow(2,16);
    p=p+(X1+X2+3791.0)/pow(2,4);
    // printf("\nX1 %lf",X1);
    // printf("\nX2 %lf",X2);
    // printf("\np: %lf",p);
    return p;
}

struct AHT_ret read_temp_humidity(){
    uint8_t ret[8];
    ret[0] =AHT_MEASURE;
    ret[1] =0x33;
    ret[2] =0x00;
    if(i2c_write_blocking(MPU_I2C,AHT10,ret,3,false)>0)
        printf("Request Sent\n");
    if(i2c_read_blocking(MPU_I2C, AHT10, ret, 6, false)>0){
        printf("Data Gathered\n");
    }
    unsigned long temperature,humidity;
    temperature=0;
    humidity=0;
    double celcius,rhum;
    humidity|=ret[1];
    humidity=humidity<<8;
    humidity|=ret[2];
    humidity=humidity<<4;
    humidity|=((ret[3]&0xf0)>>4);

    temperature|=ret[3]&0x0f;
    temperature=temperature<<8;
    temperature|=ret[4];
    temperature=temperature<<8;
    temperature|=ret[5];

    celcius=(((double)temperature/1048576)*200)-50;
    rhum=((double)humidity/1048576)*100;
    
    printf("%02X %02X %02X %02X %02X ",ret[1],ret[2],ret[3],ret[4],ret[5]);
    printf("Temperature= %lf Humidity= %lf \n",celcius,rhum);
    struct AHT_ret out;
    out.temp=celcius;
    out.hum=rhum;
    return out;
}

double altitude_calc(double pressure){
    return (44330*(1-pow((pressure/101325),1/5.255)));
}
#endif