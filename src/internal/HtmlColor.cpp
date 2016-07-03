/*-------------------------------------------------------------------------
This file contains the HtmlColor implementation

Written by Unai Uribarri

I invest time and resources providing this open source code,
please support me by dontating (see https://github.com/Makuna/NeoPixelBus)

-------------------------------------------------------------------------
This file is part of the Makuna/NeoPixelBus library.

NeoPixelBus is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3 of
the License, or (at your option) any later version.

NeoPixelBus is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with NeoPixel.  If not, see
<http://www.gnu.org/licenses/>.
-------------------------------------------------------------------------*/
#include "HtmlColor.h"

// ------------------------------------------------------------------------
// HtmlColorPair represents an association between a name and a HTML color code
// ------------------------------------------------------------------------
struct HtmlColorPair
{
    PGM_P Name;
    uint32_t Color;
};

/* HTML4 color names */
static const char c_HtmlNameAqua[] PROGMEM = "aqua";
static const char c_HtmlNameBlack[] PROGMEM = "black";
static const char c_HtmlNameBlue[] PROGMEM = "blue";
static const char c_HtmlNameFuchsia[] PROGMEM = "fuchsia";
static const char c_HtmlNameGray[] PROGMEM = "gray";
static const char c_HtmlNameGreen[] PROGMEM = "green";
static const char c_HtmlNameLime[] PROGMEM = "lime";
static const char c_HtmlNameMaroon[] PROGMEM = "maroon";
static const char c_HtmlNameNavy[] PROGMEM = "navy";
static const char c_HtmlNameOlive[] PROGMEM = "olive";
static const char c_HtmlNameOrange[] PROGMEM = "orange";
static const char c_HtmlNamePurple[] PROGMEM = "purple";
static const char c_HtmlNameRed[] PROGMEM = "red";
static const char c_HtmlNameSilver[] PROGMEM = "silver";
static const char c_HtmlNameTeal[] PROGMEM = "teal";
static const char c_HtmlNameWhite[] PROGMEM = "white";
static const char c_HtmlNameYellow[] PROGMEM = "yellow";

#if defined(USE_CSS3_COLORS)
/* Additional CSS3 color names */
static const char c_HtmlNameAliceBlue[] PROGMEM = "aliceblue";
static const char c_HtmlNameAntiqueWhite[] PROGMEM = "antiquewhite";
static const char c_HtmlNameAquamarine[] PROGMEM = "aquamarine";
static const char c_HtmlNameAzure[] PROGMEM = "azure";
static const char c_HtmlNameBeige[] PROGMEM = "beige";
static const char c_HtmlNameBisque[] PROGMEM = "bisque";
static const char c_HtmlNameBlanchedAlmond[] PROGMEM = "blanchedalmond";
static const char c_HtmlNameBlueViolet[] PROGMEM = "blueviolet";
static const char c_HtmlNameBrown[] PROGMEM = "brown";
static const char c_HtmlNameBurlyWood[] PROGMEM = "burlywood";
static const char c_HtmlNameCadetBlue[] PROGMEM = "cadetblue";
static const char c_HtmlNameChartreuse[] PROGMEM = "chartreuse";
static const char c_HtmlNameChocolate[] PROGMEM = "chocolate";
static const char c_HtmlNameCoral[] PROGMEM = "coral";
static const char c_HtmlNameCornflowerBlue[] PROGMEM = "cornflowerblue";
static const char c_HtmlNameCornsilk[] PROGMEM = "cornsilk";
static const char c_HtmlNameCrimson[] PROGMEM = "crimson";
static const char c_HtmlNameCyan[] PROGMEM = "cyan";
static const char c_HtmlNameDarkBlue[] PROGMEM = "darkblue";
static const char c_HtmlNameDarkCyan[] PROGMEM = "darkcyan";
static const char c_HtmlNameDarkGoldenrod[] PROGMEM = "darkgoldenrod";
static const char c_HtmlNameDarkGray[] PROGMEM = "darkgray";
static const char c_HtmlNameDarkGreen[] PROGMEM = "darkgreen";
static const char c_HtmlNameDarkGrey[] PROGMEM = "darkgrey";
static const char c_HtmlNameDarkKhaki[] PROGMEM = "darkkhaki";
static const char c_HtmlNameDarkMagenta[] PROGMEM = "darkmagenta";
static const char c_HtmlNameDarkOliveGreen[] PROGMEM = "darkolivegreen";
static const char c_HtmlNameDarkOrange[] PROGMEM = "darkorange";
static const char c_HtmlNameDarkOrchid[] PROGMEM = "darkorchid";
static const char c_HtmlNameDarkRed[] PROGMEM = "darkred";
static const char c_HtmlNameDarkSalmon[] PROGMEM = "darksalmon";
static const char c_HtmlNameDarkSeaGreen[] PROGMEM = "darkseagreen";
static const char c_HtmlNameDarkSlateBlue[] PROGMEM = "darkslateblue";
static const char c_HtmlNameDarkSlateGray[] PROGMEM = "darkslategray";
static const char c_HtmlNameDarkSlateGrey[] PROGMEM = "darkslategrey";
static const char c_HtmlNameDarkTurquoise[] PROGMEM = "darkturquoise";
static const char c_HtmlNameDarkViolet[] PROGMEM = "darkviolet";
static const char c_HtmlNameDeepPink[] PROGMEM = "deeppink";
static const char c_HtmlNameDeepSkyBlue[] PROGMEM = "deepskyblue";
static const char c_HtmlNameDimGray[] PROGMEM = "dimgray";
static const char c_HtmlNameDimGrey[] PROGMEM = "dimgrey";
static const char c_HtmlNameDodgerBlue[] PROGMEM = "dodgerblue";
static const char c_HtmlNameFirebrick[] PROGMEM = "firebrick";
static const char c_HtmlNameFloralWhite[] PROGMEM = "floralwhite";
static const char c_HtmlNameForestGreen[] PROGMEM = "forestgreen";
static const char c_HtmlNameGainsboro[] PROGMEM = "gainsboro";
static const char c_HtmlNameGhostWhite[] PROGMEM = "ghostwhite";
static const char c_HtmlNameGold[] PROGMEM = "gold";
static const char c_HtmlNameGoldenrod[] PROGMEM = "goldenrod";
static const char c_HtmlNameGreenYellow[] PROGMEM = "greenyellow";
static const char c_HtmlNameGrey[] PROGMEM = "grey";
static const char c_HtmlNameHoneydew[] PROGMEM = "honeydew";
static const char c_HtmlNameHotPink[] PROGMEM = "hotpink";
static const char c_HtmlNameIndianRed[] PROGMEM = "indianred";
static const char c_HtmlNameIndigo[] PROGMEM = "indigo";
static const char c_HtmlNameIvory[] PROGMEM = "ivory";
static const char c_HtmlNameKhaki[] PROGMEM = "khaki";
static const char c_HtmlNameLavender[] PROGMEM = "lavender";
static const char c_HtmlNameLavenderBlush[] PROGMEM = "lavenderblush";
static const char c_HtmlNameLawnGreen[] PROGMEM = "lawngreen";
static const char c_HtmlNameLemonChiffon[] PROGMEM = "lemonchiffon";
static const char c_HtmlNameLightBlue[] PROGMEM = "lightblue";
static const char c_HtmlNameLightCoral[] PROGMEM = "lightcoral";
static const char c_HtmlNameLightCyan[] PROGMEM = "lightcyan";
static const char c_HtmlNameLightGoldenrodYellow[] PROGMEM = "lightgoldenrodyellow";
static const char c_HtmlNameLightGray[] PROGMEM = "lightgray";
static const char c_HtmlNameLightGreen[] PROGMEM = "lightgreen";
static const char c_HtmlNameLightGrey[] PROGMEM = "lightgrey";
static const char c_HtmlNameLightPink[] PROGMEM = "lightpink";
static const char c_HtmlNameLightSalmon[] PROGMEM = "lightsalmon";
static const char c_HtmlNameLightSeaGreen[] PROGMEM = "lightseagreen";
static const char c_HtmlNameLightSkyBlue[] PROGMEM = "lightskyblue";
static const char c_HtmlNameLightSlateGray[] PROGMEM = "lightslategray";
static const char c_HtmlNameLightSlateGrey[] PROGMEM = "lightslategrey";
static const char c_HtmlNameLightSteelBlue[] PROGMEM = "lightsteelblue";
static const char c_HtmlNameLightYellow[] PROGMEM = "lightyellow";
static const char c_HtmlNameLimeGreen[] PROGMEM = "limegreen";
static const char c_HtmlNameLinen[] PROGMEM = "linen";
static const char c_HtmlNameMagenta[] PROGMEM = "magenta";
static const char c_HtmlNameMediumAquamarine[] PROGMEM = "mediumaquamarine";
static const char c_HtmlNameMediumBlue[] PROGMEM = "mediumblue";
static const char c_HtmlNameMediumOrchid[] PROGMEM = "mediumorchid";
static const char c_HtmlNameMediumPurple[] PROGMEM = "mediumpurple";
static const char c_HtmlNameMediumSeagreen[] PROGMEM = "mediumseagreen";
static const char c_HtmlNameMediumSlateBlue[] PROGMEM = "mediumslateblue";
static const char c_HtmlNameMediumSpringGreen[] PROGMEM = "mediumspringgreen";
static const char c_HtmlNameMediumTurquoise[] PROGMEM = "mediumturquoise";
static const char c_HtmlNameMediumVioletRed[] PROGMEM = "mediumvioletred";
static const char c_HtmlNameMidnightBlue[] PROGMEM = "midnightblue";
static const char c_HtmlNameMintCream[] PROGMEM = "mintcream";
static const char c_HtmlNameMistyRose[] PROGMEM = "mistyrose";
static const char c_HtmlNameMoccasin[] PROGMEM = "moccasin";
static const char c_HtmlNameNavajoWhite[] PROGMEM = "navajowhite";
static const char c_HtmlNameOldLace[] PROGMEM = "oldlace";
static const char c_HtmlNameOliveDrab[] PROGMEM = "olivedrab";
static const char c_HtmlNameOrangeRed[] PROGMEM = "orangered";
static const char c_HtmlNameOrchid[] PROGMEM = "orchid";
static const char c_HtmlNamePaleGoldenrod[] PROGMEM = "palegoldenrod";
static const char c_HtmlNamePaleGreen[] PROGMEM = "palegreen";
static const char c_HtmlNamePaleTurquoise[] PROGMEM = "paleturquoise";
static const char c_HtmlNamePaleVioletRed[] PROGMEM = "palevioletred";
static const char c_HtmlNamePapayaWhip[] PROGMEM = "papayawhip";
static const char c_HtmlNamePeachPuff[] PROGMEM = "peachpuff";
static const char c_HtmlNamePeru[] PROGMEM = "peru";
static const char c_HtmlNamePink[] PROGMEM = "pink";
static const char c_HtmlNamePlum[] PROGMEM = "plum";
static const char c_HtmlNamePowderBlue[] PROGMEM = "powderblue";
static const char c_HtmlNameRosyBrown[] PROGMEM = "rosybrown";
static const char c_HtmlNameRoyalBlue[] PROGMEM = "royalblue";
static const char c_HtmlNameSaddleBrown[] PROGMEM = "saddlebrown";
static const char c_HtmlNameSalmon[] PROGMEM = "salmon";
static const char c_HtmlNameSandyBrown[] PROGMEM = "sandybrown";
static const char c_HtmlNameSeaGreen[] PROGMEM = "seagreen";
static const char c_HtmlNameSeaShell[] PROGMEM = "seashell";
static const char c_HtmlNameSienna[] PROGMEM = "sienna";
static const char c_HtmlNameSkyBlue[] PROGMEM = "skyblue";
static const char c_HtmlNameSlateBlue[] PROGMEM = "slateblue";
static const char c_HtmlNameSlateGray[] PROGMEM = "slategray";
static const char c_HtmlNameSlateGrey[] PROGMEM = "slategrey";
static const char c_HtmlNameSnow[] PROGMEM = "snow";
static const char c_HtmlNameSpringGreen[] PROGMEM = "springgreen";
static const char c_HtmlNameSteelBlue[] PROGMEM = "steelblue";
static const char c_HtmlNameTan[] PROGMEM = "tan";
static const char c_HtmlNameThistle[] PROGMEM = "thistle";
static const char c_HtmlNameTomato[] PROGMEM = "tomato";
static const char c_HtmlNameTurquoise[] PROGMEM = "turquoise";
static const char c_HtmlNameViolet[] PROGMEM = "violet";
static const char c_HtmlNameWheat[] PROGMEM = "wheat";
static const char c_HtmlNameWhiteSmoke[] PROGMEM = "whitesmoke";
static const char c_HtmlNameYellowGreen[] PROGMEM = "yellowgreen";
#endif

const HtmlColorPair c_ColorNames[] PROGMEM = {
#if defined(USE_CSS3_COLORS)
  { c_HtmlNameAliceBlue, 0xf0f8ff},
  { c_HtmlNameAntiqueWhite, 0xfaebd7},
  { c_HtmlNameAqua, 0xffff},
  { c_HtmlNameAquamarine, 0x7fffd4},
  { c_HtmlNameAzure, 0xf0ffff},
  { c_HtmlNameBeige, 0xf5f5dc},
  { c_HtmlNameBisque, 0xffe4c4},
  { c_HtmlNameBlack, 0x0},
  { c_HtmlNameBlanchedAlmond, 0xffebcd},
  { c_HtmlNameBlue, 0xff},
  { c_HtmlNameBlueViolet, 0x8a2be2},
  { c_HtmlNameBrown, 0xa52a2a},
  { c_HtmlNameBurlyWood, 0xdeb887},
  { c_HtmlNameCadetBlue, 0x5f9ea0},
  { c_HtmlNameChartreuse, 0x7fff00},
  { c_HtmlNameChocolate, 0xd2691e},
  { c_HtmlNameCoral, 0xff7f50},
  { c_HtmlNameCornflowerBlue, 0x6495ed},
  { c_HtmlNameCornsilk, 0xfff8dc},
  { c_HtmlNameCrimson, 0xdc143c},
  { c_HtmlNameCyan, 0xffff},
  { c_HtmlNameDarkBlue, 0x8b},
  { c_HtmlNameDarkCyan, 0x8b8b},
  { c_HtmlNameDarkGoldenrod, 0xb8860b},
  { c_HtmlNameDarkGray, 0xa9a9a9},
  { c_HtmlNameDarkGreen, 0x6400},
  { c_HtmlNameDarkGrey, 0xa9a9a9},
  { c_HtmlNameDarkKhaki, 0xbdb76b},
  { c_HtmlNameDarkMagenta, 0x8b008b},
  { c_HtmlNameDarkOliveGreen, 0x556b2f},
  { c_HtmlNameDarkOrange, 0xff8c00},
  { c_HtmlNameDarkOrchid, 0x9932cc},
  { c_HtmlNameDarkRed, 0x8b0000},
  { c_HtmlNameDarkSalmon, 0xe9967a},
  { c_HtmlNameDarkSeaGreen, 0x8fbc8f},
  { c_HtmlNameDarkSlateBlue, 0x483d8b},
  { c_HtmlNameDarkSlateGray, 0x2f4f4f},
  { c_HtmlNameDarkSlateGrey, 0x2f4f4f},
  { c_HtmlNameDarkTurquoise, 0xced1},
  { c_HtmlNameDarkViolet, 0x9400d3},
  { c_HtmlNameDeepPink, 0xff1493},
  { c_HtmlNameDeepSkyBlue, 0xbfff},
  { c_HtmlNameDimGray, 0x696969},
  { c_HtmlNameDimGrey, 0x696969},
  { c_HtmlNameDodgerBlue, 0x1e90ff},
  { c_HtmlNameFirebrick, 0xb22222},
  { c_HtmlNameFloralWhite, 0xfffaf0},
  { c_HtmlNameForestGreen, 0x228b22},
  { c_HtmlNameFuchsia, 0xff00ff},
  { c_HtmlNameGainsboro, 0xdcdcdc},
  { c_HtmlNameGhostWhite, 0xf8f8ff},
  { c_HtmlNameGold, 0xffd700},
  { c_HtmlNameGoldenrod, 0xdaa520},
  { c_HtmlNameGray, 0x808080},
  { c_HtmlNameGreen, 0x8000},
  { c_HtmlNameGreenYellow, 0xadff2f},
  { c_HtmlNameGrey, 0x808080},
  { c_HtmlNameHoneydew, 0xf0fff0},
  { c_HtmlNameHotPink, 0xff69b4},
  { c_HtmlNameIndianRed, 0xcd5c5c},
  { c_HtmlNameIndigo, 0x4b0082},
  { c_HtmlNameIvory, 0xfffff0},
  { c_HtmlNameKhaki, 0xf0e68c},
  { c_HtmlNameLavender, 0xe6e6fa},
  { c_HtmlNameLavenderBlush, 0xfff0f5},
  { c_HtmlNameLawnGreen, 0x7cfc00},
  { c_HtmlNameLemonChiffon, 0xfffacd},
  { c_HtmlNameLightBlue, 0xadd8e6},
  { c_HtmlNameLightCoral, 0xf08080},
  { c_HtmlNameLightCyan, 0xe0ffff},
  { c_HtmlNameLightGoldenrodYellow, 0xfafad2},
  { c_HtmlNameLightGray, 0xd3d3d3},
  { c_HtmlNameLightGreen, 0x90ee90},
  { c_HtmlNameLightGrey, 0xd3d3d3},
  { c_HtmlNameLightPink, 0xffb6c1},
  { c_HtmlNameLightSalmon, 0xffa07a},
  { c_HtmlNameLightSeaGreen, 0x20b2aa},
  { c_HtmlNameLightSkyBlue, 0x87cefa},
  { c_HtmlNameLightSlateGray, 0x778899},
  { c_HtmlNameLightSlateGrey, 0x778899},
  { c_HtmlNameLightSteelBlue, 0xb0c4de},
  { c_HtmlNameLightYellow, 0xffffe0},
  { c_HtmlNameLime, 0xff00},
  { c_HtmlNameLimeGreen, 0x32cd32},
  { c_HtmlNameLinen, 0xfaf0e6},
  { c_HtmlNameMagenta, 0xff00ff},
  { c_HtmlNameMaroon, 0x800000},
  { c_HtmlNameMediumAquamarine, 0x66cdaa},
  { c_HtmlNameMediumBlue, 0xcd},
  { c_HtmlNameMediumOrchid, 0xba55d3},
  { c_HtmlNameMediumPurple, 0x9370d8},
  { c_HtmlNameMediumSeagreen, 0x3cb371},
  { c_HtmlNameMediumSlateBlue, 0x7b68ee},
  { c_HtmlNameMediumSpringGreen, 0xfa9a},
  { c_HtmlNameMediumTurquoise, 0x48d1cc},
  { c_HtmlNameMediumVioletRed, 0xc71585},
  { c_HtmlNameMidnightBlue, 0x191970},
  { c_HtmlNameMintCream, 0xf5fffa},
  { c_HtmlNameMistyRose, 0xffe4e1},
  { c_HtmlNameMoccasin, 0xffe4b5},
  { c_HtmlNameNavajoWhite, 0xffdead},
  { c_HtmlNameNavy, 0x80},
  { c_HtmlNameOldLace, 0xfdf5e6},
  { c_HtmlNameOlive, 0x808000},
  { c_HtmlNameOliveDrab, 0x6b8e23},
  { c_HtmlNameOrange, 0xffa500},
  { c_HtmlNameOrangeRed, 0xff4500},
  { c_HtmlNameOrchid, 0xda70d6},
  { c_HtmlNamePaleGoldenrod, 0xeee8aa},
  { c_HtmlNamePaleGreen, 0x98fb98},
  { c_HtmlNamePaleTurquoise, 0xafeeee},
  { c_HtmlNamePaleVioletRed, 0xd87093},
  { c_HtmlNamePapayaWhip, 0xffefd5},
  { c_HtmlNamePeachPuff, 0xffdab9},
  { c_HtmlNamePeru, 0xcd853f},
  { c_HtmlNamePink, 0xffc0cb},
  { c_HtmlNamePlum, 0xdda0dd},
  { c_HtmlNamePowderBlue, 0xb0e0e6},
  { c_HtmlNamePurple, 0x800080},
  { c_HtmlNameRed, 0xff0000},
  { c_HtmlNameRosyBrown, 0xbc8f8f},
  { c_HtmlNameRoyalBlue, 0x4169e1},
  { c_HtmlNameSaddleBrown, 0x8b4513},
  { c_HtmlNameSalmon, 0xfa8072},
  { c_HtmlNameSandyBrown, 0xf4a460},
  { c_HtmlNameSeaGreen, 0x2e8b57},
  { c_HtmlNameSeaShell, 0xfff5ee},
  { c_HtmlNameSienna, 0xa0522d},
  { c_HtmlNameSilver, 0xc0c0c0},
  { c_HtmlNameSkyBlue, 0x87ceeb},
  { c_HtmlNameSlateBlue, 0x6a5acd},
  { c_HtmlNameSlateGray, 0x708090},
  { c_HtmlNameSlateGrey, 0x708090},
  { c_HtmlNameSnow, 0xfffafa},
  { c_HtmlNameSpringGreen, 0xff7f},
  { c_HtmlNameSteelBlue, 0x4682b4},
  { c_HtmlNameTan, 0xd2b48c},
  { c_HtmlNameTeal, 0x8080},
  { c_HtmlNameThistle, 0xd8bfd8},
  { c_HtmlNameTomato, 0xff6347},
  { c_HtmlNameTurquoise, 0x40e0d0},
  { c_HtmlNameViolet, 0xee82ee},
  { c_HtmlNameWheat, 0xf5deb3},
  { c_HtmlNameWhite, 0xffffff},
  { c_HtmlNameWhiteSmoke, 0xf5f5f5},
  { c_HtmlNameYellow, 0xffff00},
  { c_HtmlNameYellowGreen, 0x9acd32},
#else
  { c_HtmlNameAqua, 0xffff},
  { c_HtmlNameBlack, 0x0},
  { c_HtmlNameBlue, 0xff},
  { c_HtmlNameFuchsia, 0xff00ff},
  { c_HtmlNameGray, 0x808080},
  { c_HtmlNameGreen, 0x8000},
  { c_HtmlNameLime, 0xff00},
  { c_HtmlNameMaroon, 0x800000},
  { c_HtmlNameNavy, 0x80},
  { c_HtmlNameOlive, 0x808000},
  { c_HtmlNameOrange, 0xffa500},
  { c_HtmlNamePurple, 0x800080},
  { c_HtmlNameRed, 0xff0000},
  { c_HtmlNameSilver, 0xc0c0c0},
  { c_HtmlNameTeal, 0x8080},
  { c_HtmlNameWhite, 0xffffff},
  { c_HtmlNameYellow, 0xffff00},
#endif
};

#ifndef pgm_read_ptr
// ESP8266 doesn't define this macro
#define pgm_read_ptr(addr) (*reinterpret_cast<const void* const *>(addr))
#endif

#ifndef countof
#define countof(array) (sizeof(array)/sizeof(array[0]))
#endif

size_t HtmlColor::Parse(const char* name, size_t nameSize)
{
    if (nameSize > 0)
    {
        if (name[0] == '#')
        {
            // Parse an hexadecimal notation "#rrbbgg" or "#rgb"
            //
            uint8_t temp[6]; // stores preconverted chars to hex values
            uint8_t tempSize = 0;

            for (uint8_t indexChar = 1; indexChar < nameSize && indexChar < 8; ++indexChar)
            {
                char c = name[indexChar];
                if (c >= '0' && c <= '9')
                {
                    c -= '0';
                }
                else
                {
                    // Convert a letter to lower case (only for ASCII letters)
                    // It's faster & smaller than tolower()
                    c |= 32;
                    if (c >= 'a' && c <= 'f')
                    {
                        c = c - 'a' + 10;
                    }
                    else
                    {
                        // we found an non hexidecimal char
                        // which could be null, deliminator, or other spacing
                        break;
                    }
                }

                temp[tempSize] = c;
                tempSize++;
            }

            if (tempSize != 3 && tempSize != 6)
            {
                // invalid count of numerical chars
                return 0;
            }
            else
            {
                uint32_t color = 0;
                for (uint8_t indexChar = 0; indexChar < tempSize; ++indexChar)
                {
                    color = color * 16 + temp[indexChar];
                    if (tempSize == 3)
                    {
                        // 3 digit hexadecimal notation can be supported easily
                        // duplicating digits.
                        color = color * 16 + temp[indexChar];
                    }
                }

                Color = color;
                return tempSize;
            }
        }
        else
        {
            // parse a standard name for the color
            //

            // the normal list is small enough a binary search isn't interesting,
            for (uint8_t indexName = 0; indexName < countof(c_ColorNames); ++indexName)
            {
                const HtmlColorPair* colorPair = &c_ColorNames[indexName];
                PGM_P searchName = (PGM_P)pgm_read_ptr(&colorPair->Name);
                size_t str1Size = nameSize;
                const char* str1 = name;
                const char* str2P = searchName;

                uint16_t result;

                while (str1Size > 0)
                {
                    char ch1 = tolower(*str1++);
                    char ch2 = tolower(pgm_read_byte(str2P++));
                    result = ch1 - ch2;
                    if (result != 0 || ch2 == '\0')
                    {
                        if (ch2 == '\0' && !isalnum(ch1))
                        {
                            // the string continues but is not part of a
                            // valid color name, 
                            // ends in white space, deliminator, etc
                            result = 0;
                        }
                        break;
                    }
                    result = -1; // have not reached the end of searchName;
                    str1Size--;
                }

                if (result == 0)
                {
                    Color = pgm_read_dword(&colorPair->Color);
                    return nameSize - str1Size;
                }
            }
        }
    }

    return 0;
}

static inline char hexdigit(uint8_t v)
{
    return v + (v < 10 ? '0' : 'a' - 10);
}

size_t HtmlColor::ToString(char* buf, size_t bufSize) const
{
    // search for a color value/name pairs first
    for (uint8_t indexName = 0; indexName < countof(c_ColorNames); ++indexName)
    {
        const HtmlColorPair* colorPair = &c_ColorNames[indexName];
        if (pgm_read_dword(&colorPair->Color) == Color)
        {
            PGM_P name = (PGM_P)pgm_read_ptr(&colorPair->Name);
            strncpy_P(buf, name, bufSize);
            return strlen_P(name);
        }
    }

    // no color name pair match, convert using numerical format
    return ToNumericalString(buf, bufSize);
}

size_t HtmlColor::ToNumericalString(char* buf, size_t bufSize) const
{
    size_t bufLen = bufSize - 1;

    if (bufLen-- > 0)
    {
        if (bufLen > 0)
        {
            buf[0] = '#';
        }

        uint32_t color = Color;
        for (uint8_t indexDigit = 6; indexDigit > 0; indexDigit--)
        {
            if (bufLen > indexDigit)
            {
                buf[indexDigit] = hexdigit(color & 0x0000000f);
            }
            color >>= 4;
        }

        buf[min(bufLen, 7)] = 0;
    }
    return 7;
}
