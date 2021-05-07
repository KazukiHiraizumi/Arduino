#include "ICM_20948.h"
#include "IcmDriver.h"
#include "Timeout.h"
#include "Arduino.h"
#include <ICM_20948.h>

#define BUFLEN 40
#define BUFCAL 20
#define BUFWA 30

static void cb_loop(void *ref);
static int n_calib=0;
static void cb_trigger(void *ref){
	IcmDriver *tag=(IcmDriver *)ref;
  ICM_20948 *icm=tag->icm;
  double *buf=tag->data;
  icm_20948_DMP_data_t data;
  icm->readDMPdataFromFIFO(&data);
  if(( icm->status == ICM_20948_Stat_Ok ) || ( icm->status == ICM_20948_Stat_FIFOMoreDataAvail )){
    if ( (data.header & DMP_header_bitmap_Quat9) > 0 ){
      double q1 = ((double)data.Quat9.Data.Q1) / 1073741824.0; // Convert to double. Divide by 2^30
      double q2 = ((double)data.Quat9.Data.Q2) / 1073741824.0; // Convert to double. Divide by 2^30
      double q3 = ((double)data.Quat9.Data.Q3) / 1073741824.0; // Convert to double. Divide by 2^30
      double q0 = sqrt( 1.0 - ((q1 * q1) + (q2 * q2) + (q3 * q3)));
      buf[0]=q1;
      buf[1]=q2;
      buf[2]=q3;
      buf[3]=q0;
    }
    if ( (data.header & DMP_header_bitmap_Accel) > 0 ){
      double m1 = ((double)data.Raw_Accel.Data.X)/1000*9.8;
      double m2 = ((double)data.Raw_Accel.Data.Y)/1000*9.8;
      double m3 = ((double)data.Raw_Accel.Data.Z)/1000*9.8;
      buf[4]=m1-buf[BUFCAL+4];
      buf[5]=m2-buf[BUFCAL+5];
      buf[6]=m3-buf[BUFCAL+6];
    }
    if ( (data.header & DMP_header_bitmap_Geomag) > 0 ){
      double m1 = ((double)data.Geomag.Data.Q1) / 1073741824.0; // Convert to double. Divide by 2^30
      double m2 = ((double)data.Geomag.Data.Q2) / 1073741824.0; // Convert to double. Divide by 2^30
      double m3 = ((double)data.Geomag.Data.Q3) / 1073741824.0; // Convert to double. Divide by 2^30
      buf[10]=m1;
      buf[11]=m2;
      buf[12]=m3;
    }
    if(n_calib==0) (*tag->callback)(tag->data);
    else{
      double *wa=tag->data+BUFWA;
      for(int i=0;i<7;i++) wa[i]+=buf[i];
      if(n_calib<100) n_calib++;
      else{
        double *zero=tag->data+BUFCAL;
        for(int i=0;i<7;i++) zero[i]=wa[i]/n_calib;
        for(int i=0;i<BUFCAL;i++) buf[i]=0;
        n_calib=0;
      }
    }
  }
  tag->ev_trigger=0;
  tag->ev_loop=Timeout.set(tag,cb_loop,0);
}
static void cb_loop(void *ref){
  IcmDriver *tag=(IcmDriver *)ref;
  ICM_20948 *icm=tag->icm;
  if ( icm->status == ICM_20948_Stat_FIFOMoreDataAvail )
    tag->ev_trigger=Timeout.set(tag,cb_trigger,0);
  else
    tag->ev_trigger=Timeout.set(tag,cb_trigger,10);
}
IcmDriver::IcmDriver(ICM_20948_I2C *dev,Stream *w,int a,CallbackDoublePtr cb){
	callback=cb;
	icm=dev;
	wire=w;
  adds=a;
	ev_trigger=ev_loop=0;
	data=new double[BUFLEN];
  for(int i=0;i<BUFLEN;i++) data[i]=0;
	TwoWire *tw=(TwoWire *)wire;
  tw->begin();
  tw->setClock(400000);
  Serial.println("Wire begin");
  reset();
}
void IcmDriver::reset(void){
  Timeout.clear(ev_loop);
  Timeout.clear(ev_trigger);
  for(int i=0;i<BUFLEN;i++) data[i]=0;
  n_calib=1;

  bool initialized = false;
  while( !initialized ){
    icm->begin(*(TwoWire *)wire,adds);
    if( icm->status != ICM_20948_Stat_Ok ) delay(500);
    else initialized = true;
  }
  Serial.println("icm begin");
  bool success = true; // Use success to show if the configuration was successful

  // Configure clock source through PWR_MGMT_1
  success &= (icm->setClockSource(ICM_20948_Clock_Auto) == ICM_20948_Stat_Ok); // This is shorthand: success will be set to false if setClockSource fails
  
  // Enable accel and gyro sensors through PWR_MGMT_2
  // Enable Accelerometer (all axes) and Gyroscope (all axes) by writing zero to PWR_MGMT_2
  success &= (icm->setBank(0) == ICM_20948_Stat_Ok); // Select Bank 0
  uint8_t pwrMgmt2 = 0x40; // Set the reserved bit 6
  success &= (icm->write(AGB0_REG_PWR_MGMT_2, &pwrMgmt2, 1) == ICM_20948_Stat_Ok); // Write one byte to the PWR_MGMT_2 register

  // Configure I2C_Master/Gyro/Accel in Low Power Mode (cycled) with LP_CONFIG
  success &= (icm->setSampleMode( (ICM_20948_Internal_Mst | ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), ICM_20948_Sample_Mode_Cycled ) == ICM_20948_Stat_Ok);

  // Disable the FIFO
  success &= (icm->enableFIFO(false) == ICM_20948_Stat_Ok);

  // Disable the DMP
  success &= (icm->enableDMP(false) == ICM_20948_Stat_Ok);

  // Set Gyro FSR (Full scale range) to 2000dps through GYRO_CONFIG_1
  // Set Accel FSR (Full scale range) to 4g through ACCEL_CONFIG
  ICM_20948_fss_t myFSS;  // This uses a "Full Scale Settings" structure that can contain values for all configurable sensors
  myFSS.a = gpm4;         // (ICM_20948_ACCEL_CONFIG_FS_SEL_e)
  myFSS.g = dps2000;       // (ICM_20948_GYRO_CONFIG_1_FS_SEL_e)
  success &= (icm->setFullScale( (ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myFSS ) == ICM_20948_Stat_Ok);

  // Enable interrupt for FIFO overflow from FIFOs through INT_ENABLE_2
  //success &= (icm->intEnableOverflowFIFO( 0x1F ) == ICM_20948_Stat_Ok); // Enable the interrupt on all FIFOs

  // Turn off what goes into the FIFO through FIFO_EN_1, FIFO_EN_2
  // Stop the peripheral data from being written to the FIFO by writing zero to FIFO_EN_1
  success &= (icm->setBank(0) == ICM_20948_Stat_Ok); // Select Bank 0
  uint8_t zero = 0;
  success &= (icm->write(AGB0_REG_FIFO_EN_1, &zero, 1) == ICM_20948_Stat_Ok);
  // Stop the accelerometer, gyro and temperature data from being written to the FIFO by writing zero to FIFO_EN_2
  success &= (icm->write(AGB0_REG_FIFO_EN_2, &zero, 1) == ICM_20948_Stat_Ok);

  // Turn off data ready interrupt through INT_ENABLE_1
  success &= (icm->intEnableRawDataReady(false) == ICM_20948_Stat_Ok);

  // Reset FIFO through FIFO_RST
  success &= (icm->resetFIFO() == ICM_20948_Stat_Ok);

  // Set gyro sample rate divider with GYRO_SMPLRT_DIV
  // Set accel sample rate divider with ACCEL_SMPLRT_DIV_2
  ICM_20948_smplrt_t mySmplrt;
  mySmplrt.g = 19; // ODR is computed as follows: 1.1 kHz/(1+GYRO_SMPLRT_DIV[7:0]). 19 = 55Hz. InvenSense Nucleo example uses 19 (0x13).
  mySmplrt.a = 19; // ODR is computed as follows: 1.125 kHz/(1+ACCEL_SMPLRT_DIV[11:0]). 19 = 56.25Hz. InvenSense Nucleo example uses 19 (0x13).
  icm->setSampleRate( (ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), mySmplrt ); // ** Note: comment this line to leave the sample rates at the maximum **
  
  // Setup DMP start address through PRGM_STRT_ADDRH/PRGM_STRT_ADDRL
  success &= (icm->setDMPstartAddress() == ICM_20948_Stat_Ok); // Defaults to DMP_START_ADDRESS

  // Now load the DMP firmware
  success &= (icm->loadDMPFirmware() == ICM_20948_Stat_Ok);

  // Write the 2 byte Firmware Start Value to ICM PRGM_STRT_ADDRH/PRGM_STRT_ADDRL
  success &= (icm->setDMPstartAddress() == ICM_20948_Stat_Ok); // Defaults to DMP_START_ADDRESS

  // Set the Hardware Fix Disable register to 0x48
  success &= (icm->setBank(0) == ICM_20948_Stat_Ok); // Select Bank 0
  uint8_t fix = 0x48;
  success &= (icm->write(AGB0_REG_HW_FIX_DISABLE, &fix, 1) == ICM_20948_Stat_Ok);
  
  // Set the Single FIFO Priority Select register to 0xE4
  success &= (icm->setBank(0) == ICM_20948_Stat_Ok); // Select Bank 0
  uint8_t fifoPrio = 0xE4;
  success &= (icm->write(AGB0_REG_SINGLE_FIFO_PRIORITY_SEL, &fifoPrio, 1) == ICM_20948_Stat_Ok);
  
  // Configure Accel scaling to DMP
  const unsigned char accScale[4] = {0x04, 0x00, 0x00, 0x00};
  success &= (icm->writeDMPmems(ACC_SCALE, 4, &accScale[0]) == ICM_20948_Stat_Ok); // Write accScale to ACC_SCALE DMP register
  // In order to output hardware unit data as configured FSR write 0x00040000 when FSR is 4g
  const unsigned char accScale2[4] = {0x00, 0x04, 0x00, 0x00};
  success &= (icm->writeDMPmems(ACC_SCALE2, 4, &accScale2[0]) == ICM_20948_Stat_Ok); // Write accScale2 to ACC_SCALE2 DMP register

  // Configure Compass mount matrix and scale to DMP
  const unsigned char mountMultiplierZero[4] = {0x00, 0x00, 0x00, 0x00};
  const unsigned char mountMultiplierPlus[4] = {0x09, 0x99, 0x99, 0x99}; // Value taken from InvenSense Nucleo example
  const unsigned char mountMultiplierMinus[4] = {0xF6, 0x66, 0x66, 0x67}; // Value taken from InvenSense Nucleo example
  success &= (icm->writeDMPmems(CPASS_MTX_00, 4, &mountMultiplierPlus[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(CPASS_MTX_01, 4, &mountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(CPASS_MTX_02, 4, &mountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(CPASS_MTX_10, 4, &mountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(CPASS_MTX_11, 4, &mountMultiplierMinus[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(CPASS_MTX_12, 4, &mountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(CPASS_MTX_20, 4, &mountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(CPASS_MTX_21, 4, &mountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(CPASS_MTX_22, 4, &mountMultiplierMinus[0]) == ICM_20948_Stat_Ok);

  // Configure the B2S Mounting Matrix
  const unsigned char b2sMountMultiplierZero[4] = {0x00, 0x00, 0x00, 0x00};
  const unsigned char b2sMountMultiplierPlus[4] = {0x40, 0x00, 0x00, 0x00}; // Value taken from InvenSense Nucleo example
  success &= (icm->writeDMPmems(B2S_MTX_00, 4, &b2sMountMultiplierPlus[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(B2S_MTX_01, 4, &b2sMountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(B2S_MTX_02, 4, &b2sMountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(B2S_MTX_10, 4, &b2sMountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(B2S_MTX_11, 4, &b2sMountMultiplierPlus[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(B2S_MTX_12, 4, &b2sMountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(B2S_MTX_20, 4, &b2sMountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(B2S_MTX_21, 4, &b2sMountMultiplierZero[0]) == ICM_20948_Stat_Ok);
  success &= (icm->writeDMPmems(B2S_MTX_22, 4, &b2sMountMultiplierPlus[0]) == ICM_20948_Stat_Ok);

  // Configure the DMP Gyro Scaling Factor
  success &= (icm->setGyroSF(19, 3) == ICM_20948_Stat_Ok); // 19 = 55Hz (see above), 3 = 2000dps (see above)
  
  // Configure the Gyro full scale
  const unsigned char gyroFullScale[4] = {0x10, 0x00, 0x00, 0x00}; // 2000dps : 2^28
  success &= (icm->writeDMPmems(GYRO_FULLSCALE, 4, &gyroFullScale[0]) == ICM_20948_Stat_Ok);

  // Configure the Accel Only Gain: 15252014 (225Hz) 30504029 (112Hz) 61117001 (56Hz)
  const unsigned char accelOnlyGain[4] = {0x03, 0xA4, 0x92, 0x49}; // 56Hz
  //const unsigned char accelOnlyGain[4] = {0x00, 0xE8, 0xBA, 0x2E}; // InvenSense Nucleo example uses 225Hz
  success &= (icm->writeDMPmems(ACCEL_ONLY_GAIN, 4, &accelOnlyGain[0]) == ICM_20948_Stat_Ok);
  
  // Configure the Accel Alpha Var: 1026019965 (225Hz) 977872018 (112Hz) 882002213 (56Hz)
  const unsigned char accelAlphaVar[4] = {0x34, 0x92, 0x49, 0x25}; // 56Hz
  //const unsigned char accelAlphaVar[4] = {0x06, 0x66, 0x66, 0x66}; // Value taken from InvenSense Nucleo example
  success &= (icm->writeDMPmems(ACCEL_ALPHA_VAR, 4, &accelAlphaVar[0]) == ICM_20948_Stat_Ok);
  
  // Configure the Accel A Var: 47721859 (225Hz) 95869806 (112Hz) 191739611 (56Hz)
  const unsigned char accelAVar[4] = {0x0B, 0x6D, 0xB6, 0xDB}; // 56Hz
  //const unsigned char accelAVar[4] = {0x39, 0x99, 0x99, 0x9A}; // Value taken from InvenSense Nucleo example
  success &= (icm->writeDMPmems(ACCEL_A_VAR, 4, &accelAVar[0]) == ICM_20948_Stat_Ok);
  
  // Configure the Accel Cal Rate
  const unsigned char accelCalRate[4] = {0x00, 0x00}; // Value taken from InvenSense Nucleo example
  success &= (icm->writeDMPmems(ACCEL_CAL_RATE, 2, &accelCalRate[0]) == ICM_20948_Stat_Ok);

  // Configure the Compass Time Buffer. The compass (magnetometer) is set to 100Hz (AK09916_mode_cont_100hz)
  // in startupMagnetometer. We need to set CPASS_TIME_BUFFER to 100 too.
  const unsigned char compassRate[2] = {0x00, 0x64}; // 100Hz
  success &= (icm->writeDMPmems(CPASS_TIME_BUFFER, 2, &compassRate[0]) == ICM_20948_Stat_Ok);
  
  // Enable DMP interrupt
  //success &= (icm->intEnableDMP(true) == ICM_20948_Stat_Ok);

  // Enable the DMP orientation sensor
  success &= (icm->enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) == ICM_20948_Stat_Ok);
  //success &= (icm->enableDMPSensor(INV_ICM20948_SENSOR_RAW_GYROSCOPE) == ICM_20948_Stat_Ok);
  //success &= (icm->enableDMPSensor(INV_ICM20948_SENSOR_RAW_ACCELEROMETER) == ICM_20948_Stat_Ok);
  success &= (icm->enableDMPSensor(INV_ICM20948_SENSOR_ACCELEROMETER) == ICM_20948_Stat_Ok);
  //success &= (icm->enableDMPSensor(INV_ICM20948_SENSOR_MAGNETIC_FIELD_UNCALIBRATED) == ICM_20948_Stat_Ok);

  // Configuring DMP to output data at multiple ODRs:
  success &= (icm->setDMPODRrate(DMP_ODR_Reg_Quat9, 0) == ICM_20948_Stat_Ok); // Set to the maximum
  success &= (icm->setDMPODRrate(DMP_ODR_Reg_Accel, 0) == ICM_20948_Stat_Ok); // Set to the maximum
  //success &= (icm->setDMPODRrate(DMP_ODR_Reg_Gyro, 0) == ICM_20948_Stat_Ok); // Set to the maximum
  //success &= (icm->setDMPODRrate(DMP_ODR_Reg_Gyro_Calibr, 0) == ICM_20948_Stat_Ok); // Set to the maximum
  //success &= (icm->setDMPODRrate(DMP_ODR_Reg_Cpass, 0) == ICM_20948_Stat_Ok); // Set to the maximum
  //success &= (icm->setDMPODRrate(DMP_ODR_Reg_Cpass_Calibr, 0) == ICM_20948_Stat_Ok); // Set to the maximum

  // Enable the FIFO
  success &= (icm->enableFIFO() == ICM_20948_Stat_Ok);

  // Enable the DMP
  success &= (icm->enableDMP() == ICM_20948_Stat_Ok);

  // Reset DMP
  success &= (icm->resetDMP() == ICM_20948_Stat_Ok);

  // Reset FIFO
  success &= (icm->resetFIFO() == ICM_20948_Stat_Ok);

  if( success ){
    Serial.println(F("DMP enabled!"));
    ev_trigger=Timeout.set(this,cb_trigger,10);
  }
  else{
    Serial.println(F("Enable DMP failed!"));
    Serial.println(F("Please check that you have uncommented line 29 (#define ICM_20948_USE_DMP) in ICM_20948_C.h..."));
  }
}
bool IcmDriver::parse(char *){
  reset();
}
