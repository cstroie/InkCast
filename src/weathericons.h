/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 *
 * WMO weather code → Weather Icons glyph mapping
 * Font codepoints from https://erikflowers.github.io/weather-icons/
 */

#ifndef WEATHERICONS_H
#define WEATHERICONS_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "WeatherIcons44pt7b.h"
#include "WeatherIcons10pt7b.h"

#define WI_FONT        (&weathericons44pt8b)
#define WI_SMALL_FONT  (&weathericons10pt8b)

// Day icons
#define WI_DAY_CLOUDY_WINDY        ((uint16_t)0xF001)  // wi-day-cloudy-windy
#define WI_DAY_CLOUDY              ((uint16_t)0xF002)  // wi-day-cloudy
#define WI_DAY_FOG                 ((uint16_t)0xF003)  // wi-day-fog
#define WI_DAY_HAIL                ((uint16_t)0xF004)  // wi-day-hail
#define WI_DAY_LIGHTNING           ((uint16_t)0xF005)  // wi-day-lightning
#define WI_DAY_RAIN_MIX            ((uint16_t)0xF006)  // wi-day-rain-mix
#define WI_DAY_RAIN_WIND           ((uint16_t)0xF007)  // wi-day-rain-wind
#define WI_DAY_RAIN                ((uint16_t)0xF008)  // wi-day-rain
#define WI_DAY_SHOWERS             ((uint16_t)0xF009)  // wi-day-showers
#define WI_DAY_SNOW                ((uint16_t)0xF00A)  // wi-day-snow
#define WI_DAY_SPRINKLE            ((uint16_t)0xF00B)  // wi-day-sprinkle
#define WI_DAY_SUNNY_OVERCAST      ((uint16_t)0xF00C)  // wi-day-sunny-overcast
#define WI_DAY_SUNNY               ((uint16_t)0xF00D)  // wi-day-sunny
#define WI_DAY_STORM_SHOWERS       ((uint16_t)0xF00E)  // wi-day-storm-showers
#define WI_DAY_THUNDERSTORM        ((uint16_t)0xF010)  // wi-day-thunderstorm
#define WI_DAY_SNOW_WIND           ((uint16_t)0xF065)  // wi-day-snow-wind
#define WI_DAY_SLEET_STORM         ((uint16_t)0xF068)  // wi-day-sleet-storm
#define WI_DAY_SNOW_THUNDERSTORM   ((uint16_t)0xF06B)  // wi-day-snow-thunderstorm
#define WI_DAY_CLOUDY_HIGH         ((uint16_t)0xF07D)  // wi-day-cloudy-high
#define WI_DAY_WINDY               ((uint16_t)0xF085)  // wi-day-windy
#define WI_DAY_SLEET               ((uint16_t)0xF0B2)  // wi-day-sleet
#define WI_DAY_HAZE                ((uint16_t)0xF0B6)  // wi-day-haze

// Night icons (alt = moon variants)
#define WI_NIGHT_ALT_CLOUDY_GUSTS      ((uint16_t)0xF022)  // wi-night-alt-cloudy-gusts
#define WI_NIGHT_ALT_CLOUDY_WINDY      ((uint16_t)0xF023)  // wi-night-alt-cloudy-windy
#define WI_NIGHT_ALT_HAIL              ((uint16_t)0xF024)  // wi-night-alt-hail
#define WI_NIGHT_ALT_LIGHTNING         ((uint16_t)0xF025)  // wi-night-alt-lightning
#define WI_NIGHT_ALT_RAIN_MIX          ((uint16_t)0xF026)  // wi-night-alt-rain-mix
#define WI_NIGHT_ALT_RAIN_WIND         ((uint16_t)0xF027)  // wi-night-alt-rain-wind
#define WI_NIGHT_ALT_RAIN              ((uint16_t)0xF028)  // wi-night-alt-rain
#define WI_NIGHT_ALT_SHOWERS           ((uint16_t)0xF029)  // wi-night-alt-showers
#define WI_NIGHT_ALT_SNOW              ((uint16_t)0xF02A)  // wi-night-alt-snow
#define WI_NIGHT_ALT_SPRINKLE          ((uint16_t)0xF02B)  // wi-night-alt-sprinkle
#define WI_NIGHT_ALT_STORM_SHOWERS     ((uint16_t)0xF02C)  // wi-night-alt-storm-showers
#define WI_NIGHT_ALT_THUNDERSTORM      ((uint16_t)0xF02D)  // wi-night-alt-thunderstorm
#define WI_NIGHT_CLEAR                 ((uint16_t)0xF02E)  // wi-night-clear
#define WI_NIGHT_CLOUDY_GUSTS          ((uint16_t)0xF02F)  // wi-night-cloudy-gusts
#define WI_NIGHT_CLOUDY_WINDY          ((uint16_t)0xF030)  // wi-night-cloudy-windy
#define WI_NIGHT_CLOUDY                ((uint16_t)0xF031)  // wi-night-cloudy
#define WI_NIGHT_HAIL                  ((uint16_t)0xF032)  // wi-night-hail
#define WI_NIGHT_LIGHTNING             ((uint16_t)0xF033)  // wi-night-lightning
#define WI_NIGHT_RAIN_MIX              ((uint16_t)0xF034)  // wi-night-rain-mix
#define WI_NIGHT_RAIN_WIND             ((uint16_t)0xF035)  // wi-night-rain-wind
#define WI_NIGHT_RAIN                  ((uint16_t)0xF036)  // wi-night-rain
#define WI_NIGHT_SHOWERS               ((uint16_t)0xF037)  // wi-night-showers
#define WI_NIGHT_SNOW                  ((uint16_t)0xF038)  // wi-night-snow
#define WI_NIGHT_SPRINKLE              ((uint16_t)0xF039)  // wi-night-sprinkle
#define WI_NIGHT_STORM_SHOWERS         ((uint16_t)0xF03A)  // wi-night-storm-showers
#define WI_NIGHT_THUNDERSTORM          ((uint16_t)0xF03B)  // wi-night-thunderstorm
#define WI_NIGHT_SNOW_WIND             ((uint16_t)0xF066)  // wi-night-snow-wind
#define WI_NIGHT_ALT_SNOW_WIND         ((uint16_t)0xF067)  // wi-night-alt-snow-wind
#define WI_NIGHT_SLEET_STORM           ((uint16_t)0xF069)  // wi-night-sleet-storm
#define WI_NIGHT_ALT_SLEET_STORM       ((uint16_t)0xF06A)  // wi-night-alt-sleet-storm
#define WI_NIGHT_SNOW_THUNDERSTORM     ((uint16_t)0xF06C)  // wi-night-snow-thunderstorm
#define WI_NIGHT_ALT_SNOW_THUNDERSTORM ((uint16_t)0xF06D)  // wi-night-alt-snow-thunderstorm
#define WI_NIGHT_ALT_CLOUDY_HIGH       ((uint16_t)0xF07E)  // wi-night-alt-cloudy-high
#define WI_NIGHT_CLOUDY_HIGH           ((uint16_t)0xF080)  // wi-night-cloudy-high
#define WI_NIGHT_ALT_PARTLY_CLOUDY     ((uint16_t)0xF081)  // wi-night-alt-partly-cloudy
#define WI_NIGHT_PARTLY_CLOUDY         ((uint16_t)0xF083)  // wi-night-partly-cloudy
#define WI_NIGHT_ALT_CLOUDY            ((uint16_t)0xF086)  // wi-night-alt-cloudy
#define WI_NIGHT_ALT_SLEET             ((uint16_t)0xF0B4)  // wi-night-alt-sleet
#define WI_NIGHT_SLEET                 ((uint16_t)0xF0B3)  // wi-night-sleet


// Generic / neutral weather
#define WI_CLOUDY_GUSTS    ((uint16_t)0xF011)  // wi-cloudy-gusts
#define WI_CLOUDY_WINDY    ((uint16_t)0xF012)  // wi-cloudy-windy
#define WI_CLOUDY          ((uint16_t)0xF013)  // wi-cloudy
#define WI_FOG             ((uint16_t)0xF014)  // wi-fog
#define WI_HAIL            ((uint16_t)0xF015)  // wi-hail
#define WI_LIGHTNING       ((uint16_t)0xF016)  // wi-lightning
#define WI_RAIN_MIX        ((uint16_t)0xF017)  // wi-rain-mix
#define WI_RAIN_WIND       ((uint16_t)0xF018)  // wi-rain-wind
#define WI_RAIN            ((uint16_t)0xF019)  // wi-rain
#define WI_SHOWERS         ((uint16_t)0xF01A)  // wi-showers
#define WI_SNOW            ((uint16_t)0xF01B)  // wi-snow
#define WI_SPRINKLE        ((uint16_t)0xF01C)  // wi-sprinkle
#define WI_STORM_SHOWERS   ((uint16_t)0xF01D)  // wi-storm-showers
#define WI_THUNDERSTORM    ((uint16_t)0xF01E)  // wi-thunderstorm
#define WI_WINDY           ((uint16_t)0xF021)  // wi-windy
#define WI_SNOW_WIND       ((uint16_t)0xF064)  // wi-snow-wind
#define WI_SMOKE           ((uint16_t)0xF062)  // wi-smoke
#define WI_DUST            ((uint16_t)0xF063)  // wi-dust
#define WI_SANDSTORM       ((uint16_t)0xF082)  // wi-sandstorm
#define WI_SLEET           ((uint16_t)0xF0B5)  // wi-sleet

// Astronomy / sky
#define WI_SOLAR_ECLIPSE   ((uint16_t)0xF06E)  // wi-solar-eclipse
#define WI_LUNAR_ECLIPSE   ((uint16_t)0xF070)  // wi-lunar-eclipse
#define WI_METEOR          ((uint16_t)0xF071)  // wi-meteor
#define WI_HURRICANE       ((uint16_t)0xF073)  // wi-hurricane
#define WI_TORNADO         ((uint16_t)0xF056)  // wi-tornado
#define WI_HOT             ((uint16_t)0xF072)  // wi-hot
#define WI_SNOWFLAKE_COLD  ((uint16_t)0xF076)  // wi-snowflake-cold
#define WI_SMOG            ((uint16_t)0xF074)  // wi-smog
#define WI_STARS           ((uint16_t)0xF077)  // wi-stars
#define WI_FLOOD           ((uint16_t)0xF07C)  // wi-flood

// Moon phases
#define WI_MOON_NEW               ((uint16_t)0xF095)  // wi-moon-new
#define WI_MOON_WAXING_CRESCENT_1 ((uint16_t)0xF096)  // wi-moon-waxing-crescent-1
#define WI_MOON_WAXING_CRESCENT_2 ((uint16_t)0xF097)  // wi-moon-waxing-crescent-2
#define WI_MOON_WAXING_CRESCENT_3 ((uint16_t)0xF098)  // wi-moon-waxing-crescent-3
#define WI_MOON_WAXING_CRESCENT_4 ((uint16_t)0xF099)  // wi-moon-waxing-crescent-4
#define WI_MOON_WAXING_CRESCENT_5 ((uint16_t)0xF09A)  // wi-moon-waxing-crescent-5
#define WI_MOON_WAXING_CRESCENT_6 ((uint16_t)0xF09B)  // wi-moon-waxing-crescent-6
#define WI_MOON_FIRST_QUARTER     ((uint16_t)0xF09C)  // wi-moon-first-quarter
#define WI_MOON_WAXING_GIBBOUS_1  ((uint16_t)0xF09D)  // wi-moon-waxing-gibbous-1
#define WI_MOON_WAXING_GIBBOUS_2  ((uint16_t)0xF09E)  // wi-moon-waxing-gibbous-2
#define WI_MOON_WAXING_GIBBOUS_3  ((uint16_t)0xF09F)  // wi-moon-waxing-gibbous-3
#define WI_MOON_WAXING_GIBBOUS_4  ((uint16_t)0xF0A0)  // wi-moon-waxing-gibbous-4
#define WI_MOON_WAXING_GIBBOUS_5  ((uint16_t)0xF0A1)  // wi-moon-waxing-gibbous-5
#define WI_MOON_WAXING_GIBBOUS_6  ((uint16_t)0xF0A2)  // wi-moon-waxing-gibbous-6
#define WI_MOON_FULL              ((uint16_t)0xF0A3)  // wi-moon-full
#define WI_MOON_WANING_GIBBOUS_1  ((uint16_t)0xF0A4)  // wi-moon-waning-gibbous-1
#define WI_MOON_WANING_GIBBOUS_2  ((uint16_t)0xF0A5)  // wi-moon-waning-gibbous-2
#define WI_MOON_WANING_GIBBOUS_3  ((uint16_t)0xF0A6)  // wi-moon-waning-gibbous-3
#define WI_MOON_WANING_GIBBOUS_4  ((uint16_t)0xF0A7)  // wi-moon-waning-gibbous-4
#define WI_MOON_WANING_GIBBOUS_5  ((uint16_t)0xF0A8)  // wi-moon-waning-gibbous-5
#define WI_MOON_WANING_GIBBOUS_6  ((uint16_t)0xF0A9)  // wi-moon-waning-gibbous-6
#define WI_MOON_THIRD_QUARTER     ((uint16_t)0xF0AA)  // wi-moon-third-quarter
#define WI_MOON_WANING_CRESCENT_1 ((uint16_t)0xF0AB)  // wi-moon-waning-crescent-1
#define WI_MOON_WANING_CRESCENT_2 ((uint16_t)0xF0AC)  // wi-moon-waning-crescent-2
#define WI_MOON_WANING_CRESCENT_3 ((uint16_t)0xF0AD)  // wi-moon-waning-crescent-3
#define WI_MOON_WANING_CRESCENT_4 ((uint16_t)0xF0AE)  // wi-moon-waning-crescent-4
#define WI_MOON_WANING_CRESCENT_5 ((uint16_t)0xF0AF)  // wi-moon-waning-crescent-5
#define WI_MOON_WANING_CRESCENT_6 ((uint16_t)0xF0B0)  // wi-moon-waning-crescent-6

// Clock / time
#define WI_TIME_1   ((uint16_t)0xF08A)  // wi-time-1
#define WI_TIME_2   ((uint16_t)0xF08B)  // wi-time-2
#define WI_TIME_3   ((uint16_t)0xF08C)  // wi-time-3
#define WI_TIME_4   ((uint16_t)0xF08D)  // wi-time-4
#define WI_TIME_5   ((uint16_t)0xF08E)  // wi-time-5
#define WI_TIME_6   ((uint16_t)0xF08F)  // wi-time-6
#define WI_TIME_7   ((uint16_t)0xF090)  // wi-time-7
#define WI_TIME_8   ((uint16_t)0xF091)  // wi-time-8
#define WI_TIME_9   ((uint16_t)0xF092)  // wi-time-9
#define WI_TIME_10  ((uint16_t)0xF093)  // wi-time-10
#define WI_TIME_11  ((uint16_t)0xF094)  // wi-time-11
#define WI_TIME_12  ((uint16_t)0xF089)  // wi-time-12

// Direction arrows
#define WI_DIRECTION_UP          ((uint16_t)0xF058)  // wi-direction-up
#define WI_DIRECTION_UP_RIGHT    ((uint16_t)0xF057)  // wi-direction-up-right
#define WI_DIRECTION_RIGHT       ((uint16_t)0xF04D)  // wi-direction-right
#define WI_DIRECTION_DOWN_RIGHT  ((uint16_t)0xF088)  // wi-direction-down-right
#define WI_DIRECTION_DOWN        ((uint16_t)0xF044)  // wi-direction-down
#define WI_DIRECTION_DOWN_LEFT   ((uint16_t)0xF043)  // wi-direction-down-left
#define WI_DIRECTION_LEFT        ((uint16_t)0xF048)  // wi-direction-left
#define WI_DIRECTION_UP_LEFT     ((uint16_t)0xF087)  // wi-direction-up-left
#define WI_WIND_DIRECTION        ((uint16_t)0xF0B1)  // wi-wind-direction

// Observation / instruments
#define WI_THERMOMETER          ((uint16_t)0xF055)  // wi-thermometer
#define WI_THERMOMETER_EXTERIOR ((uint16_t)0xF053)  // wi-thermometer-exterior
#define WI_THERMOMETER_INTERNAL ((uint16_t)0xF054)  // wi-thermometer-internal
#define WI_BAROMETER            ((uint16_t)0xF079)  // wi-barometer
#define WI_HUMIDITY             ((uint16_t)0xF07A)  // wi-humidity
#define WI_RAINDROP             ((uint16_t)0xF078)  // wi-raindrop
#define WI_RAINDROPS            ((uint16_t)0xF04E)  // wi-raindrops
#define WI_STRONG_WIND          ((uint16_t)0xF050)  // wi-strong-wind
#define WI_SUNRISE              ((uint16_t)0xF051)  // wi-sunrise
#define WI_SUNSET               ((uint16_t)0xF052)  // wi-sunset
#define WI_HORIZON              ((uint16_t)0xF047)  // wi-horizon
#define WI_HORIZON_ALT          ((uint16_t)0xF046)  // wi-horizon-alt

// Misc / UI
#define WI_CLOUD              ((uint16_t)0xF041)  // wi-cloud
#define WI_CLOUD_DOWN         ((uint16_t)0xF03D)  // wi-cloud-down
#define WI_CLOUD_UP           ((uint16_t)0xF040)  // wi-cloud-up
#define WI_CLOUD_REFRESH      ((uint16_t)0xF03E)  // wi-cloud-refresh
#define WI_REFRESH            ((uint16_t)0xF04C)  // wi-refresh
#define WI_REFRESH_ALT        ((uint16_t)0xF04B)  // wi-refresh-alt
#define WI_UMBRELLA           ((uint16_t)0xF084)  // wi-umbrella
#define WI_CELSIUS            ((uint16_t)0xF03C)  // wi-celsius
#define WI_FAHRENHEIT         ((uint16_t)0xF045)  // wi-fahrenheit
#define WI_DEGREES            ((uint16_t)0xF042)  // wi-degrees
#define WI_ALIEN              ((uint16_t)0xF075)  // wi-alien
#define WI_NA                 ((uint16_t)0xF07B)  // wi-na

// Returns the Weather Icons codepoint for a WMO weather code.
// isDay=true selects day icons, false selects night icons where available.
// Returns WI_NA if the code is unknown.
uint16_t getIconCode(int wmoCode, bool isDay = true);

#endif // WEATHERICONS_H
