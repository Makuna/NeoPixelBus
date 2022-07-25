/******************************************************************************************************************************************************
*****************************************************************************************************************************************************
*****************************************************************************************************************************************************
******* Class to manipulate RgbcctColor **********************************************************************************************************************************************
*****************************************************************************************************************************************************
*****************************************************************************************************************************************************
*****************************************************************************************************************************************************/

#pragma once
#include "NeoPixelBus.h"

//#define ENABLE_RGBCCT_DEBUG


// CT min and max
#define CCT_MIN_DEFAULT 153          // 6500K
#define CCT_MAX_DEFAULT 500          // 2000K


class RgbcctColor_Controller{

private:

  /**
   * Set internal colours, also buffer writer memory location if set
   * */
  uint8_t* external_rgbcct_memory_writer = nullptr; //use reintepret for this, 5 byte address

  // Rgbcct replacement to allow storing in pointer place too
  // Full range values
  void set_R(uint8_t r){
    R = r;
    if(external_rgbcct_memory_writer != nullptr){ *(external_rgbcct_memory_writer+0) = R; 
    
    // Serial.printf("external_rgbcct_memory_writer = %d ", external_rgbcct_memory_writer[0]);
    
    
    } else {
      // Serial.println("external_rgbcct_memory_writer == nullptr");
      }
  }
  void set_G(uint8_t g){
    G = g;
    if(external_rgbcct_memory_writer != nullptr){ *(external_rgbcct_memory_writer+1) = G; }
  }
  void set_B(uint8_t b){
    B = b;
    if(external_rgbcct_memory_writer != nullptr){ *(external_rgbcct_memory_writer+2) = B; }
  }
  void set_WW(uint8_t ww){
    WW = ww;
    if(external_rgbcct_memory_writer != nullptr){ *(external_rgbcct_memory_writer+3) = WW; }
  }
  void set_WC(uint8_t wc){
    WC = wc;
    if(external_rgbcct_memory_writer != nullptr){ *(external_rgbcct_memory_writer+4) = WC; }
  }
  
  uint16_t _hue = 0;  // 0..359
  uint8_t  _sat = 255;  // 0..255
      
  uint8_t  _briRGB = 255;  // 0..255
  uint8_t  _briCCT = 255;

  // Enable manual control (may not be needed)
  bool     _pwm_multi_channels = false;
  bool     _base_colour_contains_brightness_applied = false;
  
  // Color or mono
  uint8_t  _color_mode = LIGHT_MODE_BOTH;
  // Using rgbcct partially, for RGB, RGBW, RGBWW, RGBCCT etc with the unused channel turned off
  uint8_t  _subtype = LIGHT_TYPE__RGBCCT__ID;
  // are RGB and CT linked, i.e. if we set CT then RGB channels are off
  bool     _cct_rgb_linked = true;
  uint16_t _cct = CCT_MIN_DEFAULT;  // 153..500, default to 153 (cold white)
  uint16_t _cct_min_range = CCT_MIN_DEFAULT;   // the minimum CT rendered range
  uint16_t _cct_max_range = CCT_MAX_DEFAULT;   // the maximum CT rendered range

      
  /***
   *  Enable Color Modes
   * */
  void addRGBMode() {
    setColorMode(_color_mode | LIGHT_MODE_RGB);
  }
  void addCCTMode() {
    setColorMode(_color_mode | LIGHT_MODE_CCT);
  }

  /**
   * UpdateInternalColour
   * Generate rgbcct object values with latest values
   * */
  void UpdateInternalColour()
  {
    
    RgbcctColor colour_full_range = GetColourFullRange();
    RgbcctColor colour_with_brightness = GetColourWithBrightnessApplied();
    RgbcctColor colour_out;

    if(_base_colour_contains_brightness_applied){
      colour_out = colour_with_brightness;
    }else{
      colour_out = colour_full_range;
    }

    uint8_t briRGB = getBrightnessRGB255();
    uint8_t briCCT = getBrightnessCCT255();
    
    // #ifdef ENABLE_RGBCCT_DEBUG
    // Serial.printf("UpdateInternalColour\n\r colour_full_range (%d %d %d %d %d)\n\r colour_with_brightness (%d %d %d %d %d)\n\r -- rgb%d ct%d\n\r", 
    //   colour_full_range.R,colour_full_range.G,colour_full_range.B,colour_full_range.WC, colour_full_range.WW,
    //   colour_with_brightness.R,colour_with_brightness.G,colour_with_brightness.B,colour_with_brightness.WC, colour_with_brightness.WW,
    //   briRGB,briCCT
    // );
    // #endif // ENABLE_RGBCCT_DEBUG
    
    // Serial.printf("getSubType %d\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r", _subtype);

    switch (_subtype) {
      default:
      // Turn all channels off
      case LIGHT_TYPE__NONE__ID:
      Serial.println("Light type is not set, or known?");
        set_R(0);
        set_G(0);
        set_B(0);
        set_WC(0);
        set_WW(0);
        break;
      // Use RGB brightness for all channels
      case LIGHT_TYPE__SINGLE__ID:
        set_R(briRGB);
        set_G(briRGB);
        set_B(briRGB);
        set_WC(briRGB);
        set_WW(briRGB);
        break;
      case LIGHT_TYPE__COLDWARM__ID:
        set_R(0);
        set_G(0);
        set_B(0);
        set_WC(colour_out.WC);
        set_WW(colour_out.WW);
        break;
      // case LIGHT_TYPE__RGBW__ID:
      // case LIGHT_TYPE__RGBCW__ID:
      case LIGHT_TYPE__RGBCCT__ID:
        // if (LIGHT_TYPE__RGBCW__ID == _subtype) {   // error when combining colours types, if the subtypes are wc/ww then another case should be used here
          set_WC(colour_out.WC);
          set_WW(colour_out.WW);
        // } else {
        //   set_WC(briCCT);
        //   set_WW(briCCT);
        // }
        
        set_R(colour_out.R);
        set_G(colour_out.G);
        set_B(colour_out.B);

        // continue

      break;
      case LIGHT_TYPE__RGBW__ID:  //white   "WHITE" as rgbW should be generic to any white
        set_WC(briCCT);
        set_WW(briCCT);
        set_R(colour_out.R);
        set_G(colour_out.G);
        set_B(colour_out.B);

      break;

/**
 * @brief 
 * CW and WW, ie signel channel white should be handled here?
 * 
 */
      case LIGHT_TYPE__RGBCW__ID: //cold white?
        set_WC(briCCT);
        set_WW(briCCT);
        set_R(colour_out.R);
        set_G(colour_out.G);
        set_B(colour_out.B);
      break;


      case LIGHT_TYPE__RGB__ID:
        set_WC(0);
        set_WW(0);
        set_R(colour_out.R);
        set_G(colour_out.G);
        set_B(colour_out.B);
        break;
    }

  }
    
  void setHS_FullBrightness(HsbColor hsb, bool flag_hue_sat_stored_as_full_brightness_range = true){

    HsbColor hsb_new = hsb;
    if(flag_hue_sat_stored_as_full_brightness_range){
      hsb_new.B = 1; // full
    }
    RgbColor rgb = hsb_new;
    set_R(rgb.R);
    set_G(rgb.G);
    set_B(rgb.B);
    _hue = hsb_new.H*360;
    _sat = hsb_new.S*255;
    addRGBMode();
  }



public:


enum LightSubType{ 
  LIGHT_TYPE__NONE__ID=0, 
  LIGHT_TYPE__SINGLE__ID, // likely never used for me, remove
  LIGHT_TYPE__COLDWARM__ID,  //CCT Only
  LIGHT_TYPE__RGB__ID,   
  LIGHT_TYPE__RGBW__ID, 
  LIGHT_TYPE__RGBCCT__ID, // CW/WW 
  
  // Previous methods that remember colour order, probably not needed or at least cct assume default of RGBWC
  LIGHT_TYPE__RGBWC__ID, 
  LIGHT_TYPE__RGBCW__ID
};

enum LightColorModes {
  LIGHT_MODE_RGB = 1, 
  LIGHT_MODE_CCT = 2, 
  LIGHT_MODE_BOTH = 3 
};

  RgbcctColor_Controller(){
    // Set defaults
    set_R(0);
    set_G(0);
    set_B(0);
    set_WW(0);
    set_WC(0);
  };

  void Sync(){
    UpdateInternalColour();
  }
  
  // Full range values
  union {
    struct {
      union {
        uint8_t R;
        #ifndef ESP32
        uint8_t r;
        uint8_t red;
        #endif// ESP32
      };
      union {
        uint8_t G;
        #ifndef ESP32
        uint8_t g;
        uint8_t green;
        #endif// ESP32
      };
      union {
        uint8_t B;
        #ifndef ESP32
        uint8_t b;
        uint8_t blue;
        #endif// ESP32
      };
      union {
        uint8_t WC;
        #ifndef ESP32
        uint8_t wc;
        uint8_t white_cold;
        #endif// ESP32
      };
      union {
        uint8_t WW;
        #ifndef ESP32
        uint8_t ww;
        uint8_t warm_white;
        #endif
      };
    };
    uint8_t raw[5];
  };

  // Array access operator to index into the color object
  inline uint8_t& operator[] (uint8_t x) __attribute__((always_inline))
  {
    return raw[x];
  }

  // Array access operator to index into the color object
  inline const uint8_t& operator[] (uint8_t x) const __attribute__((always_inline))
  {
    return raw[x];
  }
  
  void setChannelsRaw(uint8_t *channels) {
    R = channels[0];
    G = channels[1];
    B = channels[2];
    WC = channels[3];
    WW = channels[4];
  }

  void setChannelsRaw(uint8_t r, uint8_t g, uint8_t b, uint8_t wc, uint8_t ww) {
    R = r;
    G = g;
    B = b;
    WC = wc;
    WW = ww;
  }
  
  /**
   * _base_colour_contains_brightness_applied
   * */
  void setApplyBrightnessToOutput(bool state){
    _base_colour_contains_brightness_applied = state;
  }
  bool getApplyBrightnessToOutput(){
    return _base_colour_contains_brightness_applied;
  }

  /**
   * External buffer was changed, refresh internal values
   * */
  void UpdateFromExternalBuffer(){

    if(external_rgbcct_memory_writer != nullptr){ 
      R  = *(external_rgbcct_memory_writer+0);
      G  = *(external_rgbcct_memory_writer+1);
      B  = *(external_rgbcct_memory_writer+2);
      WW = *(external_rgbcct_memory_writer+3);
      WC = *(external_rgbcct_memory_writer+4);    
    }

  }

  /**
   * Assumes a pointer to a 5 byte address, which stores the colours as [r,g,b,ww,wc]
   * */
  void setRgbcctColourOutputAddress(uint8_t* addr, bool flag_optional_load_from_address = false){

    external_rgbcct_memory_writer = addr;

    if(flag_optional_load_from_address){
      R  = *(addr+0);
      G  = *(addr+1);
      B  = *(addr+2);
      WW = *(addr+3);
      WC = *(addr+4);
    }     
  
  }


  /*********************************************************************************************************************************
   * ****** Subtype **************************************************************************************************************************
   * ********************************************************************************************************************************/
  
  void setSubType(uint8_t sub_type) {
    Serial.printf("setSubType %d\n\r\n\r\n\r\n\r\n\r\n\r\n\r\n\r", sub_type);
    _subtype = sub_type;
  }

  uint8_t getSubType(void) {
    return _subtype;
  }

  
  /*********************************************************************************************************************************
   * ****** ColorMode **************************************************************************************************************************
   * ********************************************************************************************************************************/
  
  // This function is a bit hairy, it will try to match the rerquired colormode with the features of the device:
  //  LIGHT_TYPE_NONE:     LIGHT_MODE_RGB
  //  LIGHT_TYPE_SINGLE:   LIGHT_MODE_RGB
  //  LIGHT_TYPE_COLDWARM: LIGHT_MODE_CT
  //  LIGHT_TYPE_RGB:      LIGHT_MODE_RGB
  //  LIGHT_TYPE_RGBW:     LIGHT_MODE_RGB,LIGHT_MODE_CT or LIGHT_MODE_BOTH
  //  LIGHT_TYPE_RGBCW:    LIGHT_MODE_RGB,LIGHT_MODE_CT or LIGHT_MODE_BOTH

  uint8_t setColorMode(uint8_t cm) {

    uint8_t prev_cm = _color_mode;
    if (cm < LIGHT_MODE_RGB) { cm = LIGHT_MODE_RGB; }
    if (cm > LIGHT_MODE_BOTH) { cm = LIGHT_MODE_BOTH; }
    uint8_t maxbri = (_briRGB >= _briCCT) ? _briRGB : _briCCT;

    switch (_subtype) {
      case LIGHT_TYPE__COLDWARM__ID:
        _color_mode = LIGHT_MODE_CCT;
        break;
      case LIGHT_TYPE__NONE__ID:
      case LIGHT_TYPE__SINGLE__ID:
      case LIGHT_TYPE__RGB__ID:
      default:
        _color_mode = LIGHT_MODE_RGB;
        break;
      case LIGHT_TYPE__RGBW__ID:
      case LIGHT_TYPE__RGBCW__ID:
      case LIGHT_TYPE__RGBCCT__ID:
        _color_mode = cm;
        break;
    }
    if (LIGHT_MODE_RGB == _color_mode) {
      _briCCT = 0;
      if (0 == _briRGB) { _briRGB = maxbri; }
    }
    if (LIGHT_MODE_CCT == _color_mode) {
      _briRGB = 0;
      if (0 == _briCCT) { _briCCT = maxbri; }
    }
    // #ifdef ENABLE_RGBCCT_DEBUG
    // Serial.printf("prev_cm (%d) req_cm (%d) new_cm (%d)\n\r", prev_cm, cm, _color_mode);
    // #endif // ENABLE_RGBCCT_DEBUG

    return prev_cm;
  
  }
  
  uint8_t getColorMode() {
    return _color_mode;
  }

  
  /*********************************************************************************************************************************
   * ****** RGB **************************************************************************************************************************
   * ********************************************************************************************************************************/
  
  // sets RGB and returns the Brightness. Bri is updated unless keep_bri is true
  uint8_t setRGB(uint8_t r, uint8_t g, uint8_t b, bool keep_bri = false) {
    
    #ifdef ENABLE_RGBCCT_DEBUG
    Serial.printf("setRGB RGB input (%d %d %d)\n\r", r, g, b);
    #endif

    uint32_t max = (r > g && r > b) ? r : (g > b) ? g : b;   // 0..255

    if (0 == max) {
      // No active RGB channels, disable RGB mode
      r = g = b = 255;
      setColorMode(LIGHT_MODE_CCT);
    } else {
      if (255 > max) {
        // we need to normalize rgb
        r = map(r, 0, max, 0, 255);
        g = map(g, 0, max, 0, 255);
        b = map(b, 0, max, 0, 255);
      }
      // Enable RGB mode
      addRGBMode();
    }
    if (!keep_bri) {
      _briRGB = (_color_mode & LIGHT_MODE_RGB) ? max : 0;
    }

    // Update RGB Components
    set_R(r);
    set_G(g);
    set_B(b);
    
    // Update HSB Components
    HsbColor hsb = RgbColor(r,g,b);
    _hue = hsb.H*360;
    _sat = hsb.S*255;

    #ifdef ENABLE_RGBCCT_DEBUG
    // Serial.printf("setRGB RGB output (%d %d %d) HS (%d %d) bri (%d)", _r, _g, _b, _hue, _sat, _briRGB);
    #endif

    return max;
  }

  
  /*********************************************************************************************************************************
   * ****** RgbcctColors **************************************************************************************************************************
   * ********************************************************************************************************************************/
  
  /**
   * Get RgbcctColor with brightness applied
   * */
  RgbcctColor GetColourWithBrightnessApplied(){

    bool rgb_channels_on = _color_mode & LIGHT_MODE_RGB;
    bool ct_channels_on =   _color_mode & LIGHT_MODE_CCT;

    RgbcctColor adjusted_colour;
    
    adjusted_colour.R = rgb_channels_on ? map(R, 0, 255, 0, _briRGB) : 0;
    adjusted_colour.G = rgb_channels_on ? map(G, 0, 255, 0, _briRGB) : 0;
    adjusted_colour.B = rgb_channels_on ? map(B, 0, 255, 0, _briRGB) : 0;

    adjusted_colour.WC = ct_channels_on  ? map(WC, 0, 255, 0, _briCCT) : 0; 
    adjusted_colour.WW = ct_channels_on  ? map(WW, 0, 255, 0, _briCCT) : 0;

    return adjusted_colour;

  }

  /**
   * Get RgbcctColor withOUT brightness applied
   * */
  RgbcctColor GetColourFullRange(){

    bool rgb_channels_on = _color_mode & LIGHT_MODE_RGB;
    bool ct_channels_on =   _color_mode & LIGHT_MODE_CCT;

    RgbcctColor adjusted_colour;
    
    adjusted_colour.R = rgb_channels_on ? R : 0;
    adjusted_colour.G = rgb_channels_on ? G : 0;
    adjusted_colour.B = rgb_channels_on ? B : 0;

    adjusted_colour.WC = ct_channels_on  ? WC : 0; 
    adjusted_colour.WW = ct_channels_on  ? WW : 0;

    return adjusted_colour;

  }

  
  /*********************************************************************************************************************************
   * ****** HsbColors **************************************************************************************************************************
   * ********************************************************************************************************************************/

  HsbColor getHsbColor(void){
    HsbColor hsb = HsbColor(RgbColor(0));
    hsb.H = _hue/360.0f;
    hsb.S = _sat/255.0f;
    hsb.B = _briRGB/255.0f;
    return hsb;
  }
  
  void setHue360(uint16_t hue_new){
    HsbColor hsb = getHsbColor();
    hsb.H = hue_new/360.0f;
    setHsbColor(hsb);
  }

  void setSat255(uint8_t sat_new){
    HsbColor hsb = getHsbColor();
    hsb.S = sat_new/255.0f;
    setHsbColor(hsb);
  }

  void setHsbColor(HsbColor hsb) {
    setHS_FullBrightness(hsb);
    setBrightnessRGB255(hsb.B*255); // Set brightness independently
    if (_cct_rgb_linked) { setColorMode(LIGHT_MODE_RGB); }   // try to force RGB
    UpdateInternalColour();
  }

  
  uint16_t getHue360(){
    return _hue;
  }
  
  uint8_t getSat255(){
    return _sat;
  }
  
  /************************************************************************************************************************************
  ******* Brightness *****************************************************************************************************************************
  ************************************************************************************************************************************/
 
  // sets both master Bri and whiteBri
  void setBrightness255(uint8_t bri) {
    setBrightnessRGB255(_color_mode & LIGHT_MODE_RGB ? bri : 0);
    setBrightnessCCT255(_color_mode & LIGHT_MODE_CCT ? bri : 0);
    UpdateInternalColour();
    #ifdef ENABLE_RGBCCT_DEBUG
    // AddLog(LOG_LEVEL_TEST, PSTR("setBri RGB raw (%d %d %d) HS (%d %d) bri (%d)"), R, G, B, _hue, _sat, _briRGB);
    #endif
  }

  uint8_t getBrightness255(void) {
    // return the max of _briCCT and _briRGB
    return (_briRGB >= _briCCT) ? _briRGB : _briCCT;
  }

  /************************************************************************************************************************************
  ******* BrightnessRGB *****************************************************************************************************************************
  ************************************************************************************************************************************/
 
  uint8_t setBrightnessRGB255(uint8_t bri_rgb) {
    uint8_t prev_bri = _briRGB;
    _briRGB = bri_rgb;
    //needs to map in to RGB? or I need to keep RGBCCT as colour only, with these brightness separate
    if (bri_rgb > 0) { addRGBMode(); }
    UpdateInternalColour();
    return prev_bri;
  }

  uint8_t getBrightnessRGB255() {
    return _briRGB;
  }

  /************************************************************************************************************************************
  ******* BrightnessCCT *****************************************************************************************************************************
  ************************************************************************************************************************************/

  // changes the white brightness alone
  uint8_t setBrightnessCCT255(uint8_t bri_cct) {
    uint8_t prev_bri = _briCCT;
    _briCCT = bri_cct;
    if (bri_cct > 0) { addCCTMode(); }
    UpdateInternalColour();
    return prev_bri;
  }

  uint8_t getBrightnessCCT255() {
    return _briCCT;
  } 

  /************************************************************************************************************************************
  ******* ColorTemperature *****************************************************************************************************************************
  ************************************************************************************************************************************/

  void setCCTRange(uint16_t cct_min_range, uint16_t cct_max_range) {
    _cct_min_range = cct_min_range;
    _cct_max_range = cct_max_range;
  }
  void getCTRange(uint16_t *cct_min_range, uint16_t *cct_max_range) {
    if (cct_min_range) { *cct_min_range = _cct_min_range; }
    if (cct_max_range) { *cct_max_range = _cct_max_range; }
  }

  // is the channel a regular PWM or ColorTemp control
  bool isChannelCCT(uint32_t channel) {
    // if ((LIGHT_TYPE_COLDWARM_ID == _subtype) && (1 == channel)) { return true; }   // PMW reserved for CT
    // if ((LIGHT_TYPE_RGBCW_ID == _subtype) && (4 == channel)) { return true; }   // PMW reserved for CT

  //    if (
  //   // (MODULE_PHILIPS_ID == pCONT_set->my_module_type) || 
  //   (pCONT_set->Settings.flag4.pwm_ct_mode)) {
  //   if ((LST_COLDWARM == subtype) && (1 == channel)) { return true; }   // PMW reserved for CT
  //   if ((LST_RGBCW == subtype) && (4 == channel)) { return true; }   // PMW reserved for CT
  // }
  // return false;
    return false;
  }


  void setColorTempAndBrightnessCCT255(uint16_t new_cct, uint8_t briCCT) {
    /* Color Temperature (https://developers.meethue.com/documentation/core-concepts)
      * ct = 153 = 6500K = Cold = CCWW = FF00
      * ct = 500 = 2000K = Warm = CCWW = 00FF
      */
    // don't set CT if not supported
    if ((LIGHT_TYPE__COLDWARM__ID != _subtype) && (LIGHT_TYPE__RGBW__ID > _subtype)) {
      return;
    }
    setCCT(new_cct);
    setBrightnessCCT255(briCCT);
    if (_cct_rgb_linked) { setColorMode(LIGHT_MODE_CCT); }   // try to force CT
    UpdateInternalColour();
  }

  void setCCT(uint16_t cct) {
    
// Serial.printf("cct=%d\n\r",cct);

    if (0 == cct) {
      // disable cct mode
      setColorMode(LIGHT_MODE_RGB);  // try deactivating CT mode, setColorMode() will check which is legal
    } else {
      cct = (cct < CCT_MIN_DEFAULT ? CCT_MIN_DEFAULT : (cct > CCT_MAX_DEFAULT ? CCT_MAX_DEFAULT : cct));

      WW = map(cct, _cct_min_range, _cct_max_range, 0, 255);
      WC = 255 - WW;
      _cct = cct;
      addCCTMode();
    }
    UpdateInternalColour();
    #ifdef ENABLE_RGBCCT_DEBUG
    // Serial.printf("setCT RGB raw (%d %d %d) HS (%d %d) briRGB (%d) briCCT (%d) CT (%d)", R, G, B, _hue, _sat, _briRGB, _briCCT, _ct);
    #endif
  }

  uint16_t getCCT(){
    return _cct; // 153..500, or CT_MIN..CT_MAX
  }

  uint16_t getCTifEnabled(){
    // don't calculate CT for unsupported devices
    if ((LIGHT_TYPE__COLDWARM__ID != _subtype) && (LIGHT_TYPE__RGBCW__ID != _subtype)) {
      return 0;
    }
    return (getColorMode() & LIGHT_MODE_CCT) ? getCCT() : 0;
  }

  // get the CT value within the range into a 10 bits 0..1023 value
  uint16_t getCCT10bits() {
    return map(_cct, _cct_min_range, _cct_max_range, 0, 1023);
  }

  bool setRGBCCTLinked(bool cct_rgb_linked) {
    
    bool prev = _cct_rgb_linked;
    if (_pwm_multi_channels) {
      _cct_rgb_linked = false;   // force to false if _pwm_multi_channels is set
    } else {
      _cct_rgb_linked = cct_rgb_linked;
    }
    return prev;
  }

  bool getRGBCCTLinked(){
    return _cct_rgb_linked;
  }


  /************************************************************************************************************************************
  ******* PWMChannels ie Direct control *****************************************************************************************************************************
  ************************************************************************************************************************************/

  bool setPWMMultiChannel(bool pwm_multi_channels) {
    bool prev = _pwm_multi_channels;
    _pwm_multi_channels = pwm_multi_channels;
    if (pwm_multi_channels)  setRGBCCTLinked(false);    // if pwm multi channel, then unlink RGB and CT
    return prev;
  }

  bool isPWMMultiChannel(void) {
    return _pwm_multi_channels;
  }

};


