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

/* HTML4 color names */
static const char name_aqua[] PROGMEM = "aqua";
static const char name_black[] PROGMEM = "black";
static const char name_blue[] PROGMEM = "blue";
static const char name_fuchsia[] PROGMEM = "fuchsia";
static const char name_gray[] PROGMEM = "gray";
static const char name_green[] PROGMEM = "green";
static const char name_lime[] PROGMEM = "lime";
static const char name_maroon[] PROGMEM = "maroon";
static const char name_navy[] PROGMEM = "navy";
static const char name_olive[] PROGMEM = "olive";
static const char name_orange[] PROGMEM = "orange";
static const char name_purple[] PROGMEM = "purple";
static const char name_red[] PROGMEM = "red";
static const char name_silver[] PROGMEM = "silver";
static const char name_teal[] PROGMEM = "teal";
static const char name_white[] PROGMEM = "white";
static const char name_yellow[] PROGMEM = "yellow";

#if defined(USE_CSS3_COLORS)
/* Additional CSS3 color names */
static const char name_aliceblue[] PROGMEM = "aliceblue";
static const char name_antiquewhite[] PROGMEM = "antiquewhite";
static const char name_aquamarine[] PROGMEM = "aquamarine";
static const char name_azure[] PROGMEM = "azure";
static const char name_beige[] PROGMEM = "beige";
static const char name_bisque[] PROGMEM = "bisque";
static const char name_blanchedalmond[] PROGMEM = "blanchedalmond";
static const char name_blueviolet[] PROGMEM = "blueviolet";
static const char name_brown[] PROGMEM = "brown";
static const char name_burlywood[] PROGMEM = "burlywood";
static const char name_cadetblue[] PROGMEM = "cadetblue";
static const char name_chartreuse[] PROGMEM = "chartreuse";
static const char name_chocolate[] PROGMEM = "chocolate";
static const char name_coral[] PROGMEM = "coral";
static const char name_cornflowerblue[] PROGMEM = "cornflowerblue";
static const char name_cornsilk[] PROGMEM = "cornsilk";
static const char name_crimson[] PROGMEM = "crimson";
static const char name_cyan[] PROGMEM = "cyan";
static const char name_darkblue[] PROGMEM = "darkblue";
static const char name_darkcyan[] PROGMEM = "darkcyan";
static const char name_darkgoldenrod[] PROGMEM = "darkgoldenrod";
static const char name_darkgray[] PROGMEM = "darkgray";
static const char name_darkgreen[] PROGMEM = "darkgreen";
static const char name_darkgrey[] PROGMEM = "darkgrey";
static const char name_darkkhaki[] PROGMEM = "darkkhaki";
static const char name_darkmagenta[] PROGMEM = "darkmagenta";
static const char name_darkolivegreen[] PROGMEM = "darkolivegreen";
static const char name_darkorange[] PROGMEM = "darkorange";
static const char name_darkorchid[] PROGMEM = "darkorchid";
static const char name_darkred[] PROGMEM = "darkred";
static const char name_darksalmon[] PROGMEM = "darksalmon";
static const char name_darkseagreen[] PROGMEM = "darkseagreen";
static const char name_darkslateblue[] PROGMEM = "darkslateblue";
static const char name_darkslategray[] PROGMEM = "darkslategray";
static const char name_darkslategrey[] PROGMEM = "darkslategrey";
static const char name_darkturquoise[] PROGMEM = "darkturquoise";
static const char name_darkviolet[] PROGMEM = "darkviolet";
static const char name_deeppink[] PROGMEM = "deeppink";
static const char name_deepskyblue[] PROGMEM = "deepskyblue";
static const char name_dimgray[] PROGMEM = "dimgray";
static const char name_dimgrey[] PROGMEM = "dimgrey";
static const char name_dodgerblue[] PROGMEM = "dodgerblue";
static const char name_firebrick[] PROGMEM = "firebrick";
static const char name_floralwhite[] PROGMEM = "floralwhite";
static const char name_forestgreen[] PROGMEM = "forestgreen";
static const char name_gainsboro[] PROGMEM = "gainsboro";
static const char name_ghostwhite[] PROGMEM = "ghostwhite";
static const char name_gold[] PROGMEM = "gold";
static const char name_goldenrod[] PROGMEM = "goldenrod";
static const char name_greenyellow[] PROGMEM = "greenyellow";
static const char name_grey[] PROGMEM = "grey";
static const char name_honeydew[] PROGMEM = "honeydew";
static const char name_hotpink[] PROGMEM = "hotpink";
static const char name_indianred[] PROGMEM = "indianred";
static const char name_indigo[] PROGMEM = "indigo";
static const char name_ivory[] PROGMEM = "ivory";
static const char name_khaki[] PROGMEM = "khaki";
static const char name_lavender[] PROGMEM = "lavender";
static const char name_lavenderblush[] PROGMEM = "lavenderblush";
static const char name_lawngreen[] PROGMEM = "lawngreen";
static const char name_lemonchiffon[] PROGMEM = "lemonchiffon";
static const char name_lightblue[] PROGMEM = "lightblue";
static const char name_lightcoral[] PROGMEM = "lightcoral";
static const char name_lightcyan[] PROGMEM = "lightcyan";
static const char name_lightgoldenrodyellow[] PROGMEM = "lightgoldenrodyellow";
static const char name_lightgray[] PROGMEM = "lightgray";
static const char name_lightgreen[] PROGMEM = "lightgreen";
static const char name_lightgrey[] PROGMEM = "lightgrey";
static const char name_lightpink[] PROGMEM = "lightpink";
static const char name_lightsalmon[] PROGMEM = "lightsalmon";
static const char name_lightseagreen[] PROGMEM = "lightseagreen";
static const char name_lightskyblue[] PROGMEM = "lightskyblue";
static const char name_lightslategray[] PROGMEM = "lightslategray";
static const char name_lightslategrey[] PROGMEM = "lightslategrey";
static const char name_lightsteelblue[] PROGMEM = "lightsteelblue";
static const char name_lightyellow[] PROGMEM = "lightyellow";
static const char name_limegreen[] PROGMEM = "limegreen";
static const char name_linen[] PROGMEM = "linen";
static const char name_magenta[] PROGMEM = "magenta";
static const char name_mediumaquamarine[] PROGMEM = "mediumaquamarine";
static const char name_mediumblue[] PROGMEM = "mediumblue";
static const char name_mediumorchid[] PROGMEM = "mediumorchid";
static const char name_mediumpurple[] PROGMEM = "mediumpurple";
static const char name_mediumseagreen[] PROGMEM = "mediumseagreen";
static const char name_mediumslateblue[] PROGMEM = "mediumslateblue";
static const char name_mediumspringgreen[] PROGMEM = "mediumspringgreen";
static const char name_mediumturquoise[] PROGMEM = "mediumturquoise";
static const char name_mediumvioletred[] PROGMEM = "mediumvioletred";
static const char name_midnightblue[] PROGMEM = "midnightblue";
static const char name_mintcream[] PROGMEM = "mintcream";
static const char name_mistyrose[] PROGMEM = "mistyrose";
static const char name_moccasin[] PROGMEM = "moccasin";
static const char name_navajowhite[] PROGMEM = "navajowhite";
static const char name_oldlace[] PROGMEM = "oldlace";
static const char name_olivedrab[] PROGMEM = "olivedrab";
static const char name_orangered[] PROGMEM = "orangered";
static const char name_orchid[] PROGMEM = "orchid";
static const char name_palegoldenrod[] PROGMEM = "palegoldenrod";
static const char name_palegreen[] PROGMEM = "palegreen";
static const char name_paleturquoise[] PROGMEM = "paleturquoise";
static const char name_palevioletred[] PROGMEM = "palevioletred";
static const char name_papayawhip[] PROGMEM = "papayawhip";
static const char name_peachpuff[] PROGMEM = "peachpuff";
static const char name_peru[] PROGMEM = "peru";
static const char name_pink[] PROGMEM = "pink";
static const char name_plum[] PROGMEM = "plum";
static const char name_powderblue[] PROGMEM = "powderblue";
static const char name_rosybrown[] PROGMEM = "rosybrown";
static const char name_royalblue[] PROGMEM = "royalblue";
static const char name_saddlebrown[] PROGMEM = "saddlebrown";
static const char name_salmon[] PROGMEM = "salmon";
static const char name_sandybrown[] PROGMEM = "sandybrown";
static const char name_seagreen[] PROGMEM = "seagreen";
static const char name_seashell[] PROGMEM = "seashell";
static const char name_sienna[] PROGMEM = "sienna";
static const char name_skyblue[] PROGMEM = "skyblue";
static const char name_slateblue[] PROGMEM = "slateblue";
static const char name_slategray[] PROGMEM = "slategray";
static const char name_slategrey[] PROGMEM = "slategrey";
static const char name_snow[] PROGMEM = "snow";
static const char name_springgreen[] PROGMEM = "springgreen";
static const char name_steelblue[] PROGMEM = "steelblue";
static const char name_tan[] PROGMEM = "tan";
static const char name_thistle[] PROGMEM = "thistle";
static const char name_tomato[] PROGMEM = "tomato";
static const char name_turquoise[] PROGMEM = "turquoise";
static const char name_violet[] PROGMEM = "violet";
static const char name_wheat[] PROGMEM = "wheat";
static const char name_whitesmoke[] PROGMEM = "whitesmoke";
static const char name_yellowgreen[] PROGMEM = "yellowgreen";
#endif

const HtmlColorName HtmlColor::ColorNames[] PROGMEM = {
#if defined(USE_CSS3_COLORS)
  { name_aliceblue, 0xf0f8ff},
  { name_antiquewhite, 0xfaebd7},
  { name_aqua, 0xffff},
  { name_aquamarine, 0x7fffd4},
  { name_azure, 0xf0ffff},
  { name_beige, 0xf5f5dc},
  { name_bisque, 0xffe4c4},
  { name_black, 0x0},
  { name_blanchedalmond, 0xffebcd},
  { name_blue, 0xff},
  { name_blueviolet, 0x8a2be2},
  { name_brown, 0xa52a2a},
  { name_burlywood, 0xdeb887},
  { name_cadetblue, 0x5f9ea0},
  { name_chartreuse, 0x7fff00},
  { name_chocolate, 0xd2691e},
  { name_coral, 0xff7f50},
  { name_cornflowerblue, 0x6495ed},
  { name_cornsilk, 0xfff8dc},
  { name_crimson, 0xdc143c},
  { name_cyan, 0xffff},
  { name_darkblue, 0x8b},
  { name_darkcyan, 0x8b8b},
  { name_darkgoldenrod, 0xb8860b},
  { name_darkgray, 0xa9a9a9},
  { name_darkgreen, 0x6400},
  { name_darkgrey, 0xa9a9a9},
  { name_darkkhaki, 0xbdb76b},
  { name_darkmagenta, 0x8b008b},
  { name_darkolivegreen, 0x556b2f},
  { name_darkorange, 0xff8c00},
  { name_darkorchid, 0x9932cc},
  { name_darkred, 0x8b0000},
  { name_darksalmon, 0xe9967a},
  { name_darkseagreen, 0x8fbc8f},
  { name_darkslateblue, 0x483d8b},
  { name_darkslategray, 0x2f4f4f},
  { name_darkslategrey, 0x2f4f4f},
  { name_darkturquoise, 0xced1},
  { name_darkviolet, 0x9400d3},
  { name_deeppink, 0xff1493},
  { name_deepskyblue, 0xbfff},
  { name_dimgray, 0x696969},
  { name_dimgrey, 0x696969},
  { name_dodgerblue, 0x1e90ff},
  { name_firebrick, 0xb22222},
  { name_floralwhite, 0xfffaf0},
  { name_forestgreen, 0x228b22},
  { name_fuchsia, 0xff00ff},
  { name_gainsboro, 0xdcdcdc},
  { name_ghostwhite, 0xf8f8ff},
  { name_gold, 0xffd700},
  { name_goldenrod, 0xdaa520},
  { name_gray, 0x808080},
  { name_green, 0x8000},
  { name_greenyellow, 0xadff2f},
  { name_grey, 0x808080},
  { name_honeydew, 0xf0fff0},
  { name_hotpink, 0xff69b4},
  { name_indianred, 0xcd5c5c},
  { name_indigo, 0x4b0082},
  { name_ivory, 0xfffff0},
  { name_khaki, 0xf0e68c},
  { name_lavender, 0xe6e6fa},
  { name_lavenderblush, 0xfff0f5},
  { name_lawngreen, 0x7cfc00},
  { name_lemonchiffon, 0xfffacd},
  { name_lightblue, 0xadd8e6},
  { name_lightcoral, 0xf08080},
  { name_lightcyan, 0xe0ffff},
  { name_lightgoldenrodyellow, 0xfafad2},
  { name_lightgray, 0xd3d3d3},
  { name_lightgreen, 0x90ee90},
  { name_lightgrey, 0xd3d3d3},
  { name_lightpink, 0xffb6c1},
  { name_lightsalmon, 0xffa07a},
  { name_lightseagreen, 0x20b2aa},
  { name_lightskyblue, 0x87cefa},
  { name_lightslategray, 0x778899},
  { name_lightslategrey, 0x778899},
  { name_lightsteelblue, 0xb0c4de},
  { name_lightyellow, 0xffffe0},
  { name_lime, 0xff00},
  { name_limegreen, 0x32cd32},
  { name_linen, 0xfaf0e6},
  { name_magenta, 0xff00ff},
  { name_maroon, 0x800000},
  { name_mediumaquamarine, 0x66cdaa},
  { name_mediumblue, 0xcd},
  { name_mediumorchid, 0xba55d3},
  { name_mediumpurple, 0x9370d8},
  { name_mediumseagreen, 0x3cb371},
  { name_mediumslateblue, 0x7b68ee},
  { name_mediumspringgreen, 0xfa9a},
  { name_mediumturquoise, 0x48d1cc},
  { name_mediumvioletred, 0xc71585},
  { name_midnightblue, 0x191970},
  { name_mintcream, 0xf5fffa},
  { name_mistyrose, 0xffe4e1},
  { name_moccasin, 0xffe4b5},
  { name_navajowhite, 0xffdead},
  { name_navy, 0x80},
  { name_oldlace, 0xfdf5e6},
  { name_olive, 0x808000},
  { name_olivedrab, 0x6b8e23},
  { name_orange, 0xffa500},
  { name_orangered, 0xff4500},
  { name_orchid, 0xda70d6},
  { name_palegoldenrod, 0xeee8aa},
  { name_palegreen, 0x98fb98},
  { name_paleturquoise, 0xafeeee},
  { name_palevioletred, 0xd87093},
  { name_papayawhip, 0xffefd5},
  { name_peachpuff, 0xffdab9},
  { name_peru, 0xcd853f},
  { name_pink, 0xffc0cb},
  { name_plum, 0xdda0dd},
  { name_powderblue, 0xb0e0e6},
  { name_purple, 0x800080},
  { name_red, 0xff0000},
  { name_rosybrown, 0xbc8f8f},
  { name_royalblue, 0x4169e1},
  { name_saddlebrown, 0x8b4513},
  { name_salmon, 0xfa8072},
  { name_sandybrown, 0xf4a460},
  { name_seagreen, 0x2e8b57},
  { name_seashell, 0xfff5ee},
  { name_sienna, 0xa0522d},
  { name_silver, 0xc0c0c0},
  { name_skyblue, 0x87ceeb},
  { name_slateblue, 0x6a5acd},
  { name_slategray, 0x708090},
  { name_slategrey, 0x708090},
  { name_snow, 0xfffafa},
  { name_springgreen, 0xff7f},
  { name_steelblue, 0x4682b4},
  { name_tan, 0xd2b48c},
  { name_teal, 0x8080},
  { name_thistle, 0xd8bfd8},
  { name_tomato, 0xff6347},
  { name_turquoise, 0x40e0d0},
  { name_violet, 0xee82ee},
  { name_wheat, 0xf5deb3},
  { name_white, 0xffffff},
  { name_whitesmoke, 0xf5f5f5},
  { name_yellow, 0xffff00},
  { name_yellowgreen, 0x9acd32},
#else
  { name_aqua, 0xffff},
  { name_black, 0x0},
  { name_blue, 0xff},
  { name_fuchsia, 0xff00ff},
  { name_gray, 0x808080},
  { name_green, 0x8000},
  { name_lime, 0xff00},
  { name_maroon, 0x800000},
  { name_navy, 0x80},
  { name_olive, 0x808000},
  { name_orange, 0xffa500},
  { name_purple, 0x800080},
  { name_red, 0xff0000},
  { name_silver, 0xc0c0c0},
  { name_teal, 0x8080},
  { name_white, 0xffffff},
  { name_yellow, 0xffff00},
#endif
};

#ifndef pgm_read_ptr
// ESP8266 doesn't define this macro, but it doesn't need it either.
#define pgm_read_ptr(addr) (*reinterpret_cast<const void* const *>(addr))
#endif

bool HtmlColor::Parse(const char* name, size_t namelen)
{
    if (name[0] == '#')
    {
        // Parse an hexadecimal notation "#rrbbgg" or "#rgb"
        if (namelen != 4 && namelen != 7)
        {
            return false;
        }

        uint32_t color = 0;
        for (uint8_t i = 1; i < namelen; ++i)
        {
            char c = name[i];
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
                    return false;
                }
            }
            color = color * 16 + c;
            if (namelen == 4)
            {
                // 3 digit hexadecimal notation can be supported easily
                // duplicating digits.
                color = color * 16 + c;
            }
        }
        Color = color;
        return true;
    }

    for (uint8_t i = 0; i < sizeof(ColorNames)/sizeof(ColorNames[0]); ++i)
    {
        const HtmlColorName* cptr = &ColorNames[i];
        PGM_P cname = (PGM_P)pgm_read_ptr(&cptr->Name);
        if (strncasecmp_P(name, cname, namelen) == 0 && strlen_P(cname) == namelen)
        {
            Color = pgm_read_dword(&cptr->Color);
            return true;
        }
    }
    return false;
}

static inline char hexdigit(uint8_t v)
{
    return v + (v < 10 ? '0' : 'a' - 10);
}

size_t HtmlColor::ToString(char* buf, size_t buflen) const
{
    for (uint8_t i = 0; i < sizeof(ColorNames) / sizeof(ColorNames[0]); ++i)
    {
        const HtmlColorName* cptr = &ColorNames[i];
        if (pgm_read_dword(&cptr->Color) == Color)
        {
            PGM_P name = (PGM_P)pgm_read_ptr(&cptr->Name);
            size_t namelen = strlen_P(name);
            if (buflen-- > 0)
            {
                buflen = min(buflen, namelen);
                memcpy_P(buf, name, buflen);
                buf[buflen] = 0;
            }
            return namelen;
        }
    }

    if (buflen-- > 0)
    {
        if (buflen > 0)
        {
            buf[0] = '#';
        }
        uint32_t color = Color;
        for (uint8_t i = 6; i > 0; i--)
        {
            if (buflen > i)
            {
                buf[i] = hexdigit(color & 0x0000000f);
            }
            color >>= 4;
        }
        buf[min(buflen, 7)] = 0;
    }
    return 7;
}
