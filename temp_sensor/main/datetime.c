// ====================================================================
// INCLUDES 
// ====================================================================

// esp-idf
#include "esp_sntp.h"

// Third party
#include "lwip/err.h"
#include "lwip/sys.h"

#include "common.h"
#include "datetime.h"

// #include "wifi.h"

// ====================================================================
// MACROS
// ====================================================================

#define TIME_TM_TEST_YEAR  (2016-1900)
#define TIME_UPDATED_ONCE()    (tm_gmt.tm_year >= TIME_TM_TEST_YEAR)

// ====================================================================
// TIMEZONE DATA
// ====================================================================

// https://raw.githubusercontent.com/nayarsystems/posix_tz_db/master/zones.csv
// static const char* timezone_data = "Africa/Abidjan;GMT0\nAfrica/Accra;GMT0\nAfrica/Addis_Ababa;EAT-3\nAfrica/Algiers;CET-1\nAfrica/Asmara;EAT-3\nAfrica/Bamako;GMT0\nAfrica/Bangui;WAT-1\nAfrica/Banjul;GMT0\nAfrica/Bissau;GMT0\nAfrica/Blantyre;CAT-2\nAfrica/Brazzaville;WAT-1\nAfrica/Bujumbura;CAT-2\nAfrica/Cairo;EET-2\nAfrica/Casablanca;<+01>-1\nAfrica/Ceuta;CET-1CEST,M3.5.0,M10.5.0/3\nAfrica/Conakry;GMT0\nAfrica/Dakar;GMT0\nAfrica/Dar_es_Salaam;EAT-3\nAfrica/Djibouti;EAT-3\nAfrica/Douala;WAT-1\nAfrica/El_Aaiun;<+01>-1\nAfrica/Freetown;GMT0\nAfrica/Gaborone;CAT-2\nAfrica/Harare;CAT-2\nAfrica/Johannesburg;SAST-2\nAfrica/Juba;EAT-3\nAfrica/Kampala;EAT-3\nAfrica/Khartoum;CAT-2\nAfrica/Kigali;CAT-2\nAfrica/Kinshasa;WAT-1\nAfrica/Lagos;WAT-1\nAfrica/Libreville;WAT-1\nAfrica/Lome;GMT0\nAfrica/Luanda;WAT-1\nAfrica/Lubumbashi;CAT-2\nAfrica/Lusaka;CAT-2\nAfrica/Malabo;WAT-1\nAfrica/Maputo;CAT-2\nAfrica/Maseru;SAST-2\nAfrica/Mbabane;SAST-2\nAfrica/Mogadishu;EAT-3\nAfrica/Monrovia;GMT0\nAfrica/Nairobi;EAT-3\nAfrica/Ndjamena;WAT-1\nAfrica/Niamey;WAT-1\nAfrica/Nouakchott;GMT0\nAfrica/Ouagadougou;GMT0\nAfrica/Porto-Novo;WAT-1\nAfrica/Sao_Tome;GMT0\nAfrica/Tripoli;EET-2\nAfrica/Tunis;CET-1\nAfrica/Windhoek;CAT-2\nAmerica/Adak;HST10HDT,M3.2.0,M11.1.0\nAmerica/Anchorage;AKST9AKDT,M3.2.0,M11.1.0\nAmerica/Anguilla;AST4\nAmerica/Antigua;AST4\nAmerica/Araguaina;<-03>3\nAmerica/Argentina/Buenos_Aires;<-03>3\nAmerica/Argentina/Catamarca;<-03>3\nAmerica/Argentina/Cordoba;<-03>3\nAmerica/Argentina/Jujuy;<-03>3\nAmerica/Argentina/La_Rioja;<-03>3\nAmerica/Argentina/Mendoza;<-03>3\nAmerica/Argentina/Rio_Gallegos;<-03>3\nAmerica/Argentina/Salta;<-03>3\nAmerica/Argentina/San_Juan;<-03>3\nAmerica/Argentina/San_Luis;<-03>3\nAmerica/Argentina/Tucuman;<-03>3\nAmerica/Argentina/Ushuaia;<-03>3\nAmerica/Aruba;AST4\nAmerica/Asuncion;<-04>4<-03>,M10.1.0/0,M3.4.0/0\nAmerica/Atikokan;EST5\nAmerica/Bahia;<-03>3\nAmerica/Bahia_Banderas;CST6CDT,M4.1.0,M10.5.0\nAmerica/Barbados;AST4\nAmerica/Belem;<-03>3\nAmerica/Belize;CST6\nAmerica/Blanc-Sablon;AST4\nAmerica/Boa_Vista;<-04>4\nAmerica/Bogota;<-05>5\nAmerica/Boise;MST7MDT,M3.2.0,M11.1.0\nAmerica/Cambridge_Bay;MST7MDT,M3.2.0,M11.1.0\nAmerica/Campo_Grande;<-04>4\nAmerica/Cancun;EST5\nAmerica/Caracas;<-04>4\nAmerica/Cayenne;<-03>3\nAmerica/Cayman;EST5\nAmerica/Chicago;CST6CDT,M3.2.0,M11.1.0\nAmerica/Chihuahua;MST7MDT,M4.1.0,M10.5.0\nAmerica/Costa_Rica;CST6\nAmerica/Creston;MST7\nAmerica/Cuiaba;<-04>4\nAmerica/Curacao;AST4\nAmerica/Danmarkshavn;GMT0\nAmerica/Dawson;MST7\nAmerica/Dawson_Creek;MST7\nAmerica/Denver;MST7MDT,M3.2.0,M11.1.0\nAmerica/Detroit;EST5EDT,M3.2.0,M11.1.0\nAmerica/Dominica;AST4\nAmerica/Edmonton;MST7MDT,M3.2.0,M11.1.0\nAmerica/Eirunepe;<-05>5\nAmerica/El_Salvador;CST6\nAmerica/Fortaleza;<-03>3\nAmerica/Fort_Nelson;MST7\nAmerica/Glace_Bay;AST4ADT,M3.2.0,M11.1.0\nAmerica/Godthab;<-03>3<-02>,M3.5.0/-2,M10.5.0/-1\nAmerica/Goose_Bay;AST4ADT,M3.2.0,M11.1.0\nAmerica/Grand_Turk;EST5EDT,M3.2.0,M11.1.0\nAmerica/Grenada;AST4\nAmerica/Guadeloupe;AST4\nAmerica/Guatemala;CST6\nAmerica/Guayaquil;<-05>5\nAmerica/Guyana;<-04>4\nAmerica/Halifax;AST4ADT,M3.2.0,M11.1.0\nAmerica/Havana;CST5CDT,M3.2.0/0,M11.1.0/1\nAmerica/Hermosillo;MST7\nAmerica/Indiana/Indianapolis;EST5EDT,M3.2.0,M11.1.0\nAmerica/Indiana/Knox;CST6CDT,M3.2.0,M11.1.0\nAmerica/Indiana/Marengo;EST5EDT,M3.2.0,M11.1.0\nAmerica/Indiana/Petersburg;EST5EDT,M3.2.0,M11.1.0\nAmerica/Indiana/Tell_City;CST6CDT,M3.2.0,M11.1.0\nAmerica/Indiana/Vevay;EST5EDT,M3.2.0,M11.1.0\nAmerica/Indiana/Vincennes;EST5EDT,M3.2.0,M11.1.0\nAmerica/Indiana/Winamac;EST5EDT,M3.2.0,M11.1.0\nAmerica/Inuvik;MST7MDT,M3.2.0,M11.1.0\nAmerica/Iqaluit;EST5EDT,M3.2.0,M11.1.0\nAmerica/Jamaica;EST5\nAmerica/Juneau;AKST9AKDT,M3.2.0,M11.1.0\nAmerica/Kentucky/Louisville;EST5EDT,M3.2.0,M11.1.0\nAmerica/Kentucky/Monticello;EST5EDT,M3.2.0,M11.1.0\nAmerica/Kralendijk;AST4\nAmerica/La_Paz;<-04>4\nAmerica/Lima;<-05>5\nAmerica/Los_Angeles;PST8PDT,M3.2.0,M11.1.0\nAmerica/Lower_Princes;AST4\nAmerica/Maceio;<-03>3\nAmerica/Managua;CST6\nAmerica/Manaus;<-04>4\nAmerica/Marigot;AST4\nAmerica/Martinique;AST4\nAmerica/Matamoros;CST6CDT,M3.2.0,M11.1.0\nAmerica/Mazatlan;MST7MDT,M4.1.0,M10.5.0\nAmerica/Menominee;CST6CDT,M3.2.0,M11.1.0\nAmerica/Merida;CST6CDT,M4.1.0,M10.5.0\nAmerica/Metlakatla;AKST9AKDT,M3.2.0,M11.1.0\nAmerica/Mexico_City;CST6CDT,M4.1.0,M10.5.0\nAmerica/Miquelon;<-03>3<-02>,M3.2.0,M11.1.0\nAmerica/Moncton;AST4ADT,M3.2.0,M11.1.0\nAmerica/Monterrey;CST6CDT,M4.1.0,M10.5.0\nAmerica/Montevideo;<-03>3\nAmerica/Montreal;EST5EDT,M3.2.0,M11.1.0\nAmerica/Montserrat;AST4\nAmerica/Nassau;EST5EDT,M3.2.0,M11.1.0\nAmerica/New_York;EST5EDT,M3.2.0,M11.1.0\nAmerica/Nipigon;EST5EDT,M3.2.0,M11.1.0\nAmerica/Nome;AKST9AKDT,M3.2.0,M11.1.0\nAmerica/Noronha;<-02>2\nAmerica/North_Dakota/Beulah;CST6CDT,M3.2.0,M11.1.0\nAmerica/North_Dakota/Center;CST6CDT,M3.2.0,M11.1.0\nAmerica/North_Dakota/New_Salem;CST6CDT,M3.2.0,M11.1.0\nAmerica/Nuuk;<-03>3<-02>,M3.5.0/-2,M10.5.0/-1\nAmerica/Ojinaga;MST7MDT,M3.2.0,M11.1.0\nAmerica/Panama;EST5\nAmerica/Pangnirtung;EST5EDT,M3.2.0,M11.1.0\nAmerica/Paramaribo;<-03>3\nAmerica/Phoenix;MST7\nAmerica/Port-au-Prince;EST5EDT,M3.2.0,M11.1.0\nAmerica/Port_of_Spain;AST4\nAmerica/Porto_Velho;<-04>4\nAmerica/Puerto_Rico;AST4\nAmerica/Punta_Arenas;<-03>3\nAmerica/Rainy_River;CST6CDT,M3.2.0,M11.1.0\nAmerica/Rankin_Inlet;CST6CDT,M3.2.0,M11.1.0\nAmerica/Recife;<-03>3\nAmerica/Regina;CST6\nAmerica/Resolute;CST6CDT,M3.2.0,M11.1.0\nAmerica/Rio_Branco;<-05>5\nAmerica/Santarem;<-03>3\nAmerica/Santiago;<-04>4<-03>,M9.1.6/24,M4.1.6/24\nAmerica/Santo_Domingo;AST4\nAmerica/Sao_Paulo;<-03>3\nAmerica/Scoresbysund;<-01>1<+00>,M3.5.0/0,M10.5.0/1\nAmerica/Sitka;AKST9AKDT,M3.2.0,M11.1.0\nAmerica/St_Barthelemy;AST4\nAmerica/St_Johns;NST3:30NDT,M3.2.0,M11.1.0\nAmerica/St_Kitts;AST4\nAmerica/St_Lucia;AST4\nAmerica/St_Thomas;AST4\nAmerica/St_Vincent;AST4\nAmerica/Swift_Current;CST6\nAmerica/Tegucigalpa;CST6\nAmerica/Thule;AST4ADT,M3.2.0,M11.1.0\nAmerica/Thunder_Bay;EST5EDT,M3.2.0,M11.1.0\nAmerica/Tijuana;PST8PDT,M3.2.0,M11.1.0\nAmerica/Toronto;EST5EDT,M3.2.0,M11.1.0\nAmerica/Tortola;AST4\nAmerica/Vancouver;PST8PDT,M3.2.0,M11.1.0\nAmerica/Whitehorse;MST7\nAmerica/Winnipeg;CST6CDT,M3.2.0,M11.1.0\nAmerica/Yakutat;AKST9AKDT,M3.2.0,M11.1.0\nAmerica/Yellowknife;MST7MDT,M3.2.0,M11.1.0\nAntarctica/Casey;<+11>-11\nAntarctica/Davis;<+07>-7\nAntarctica/DumontDUrville;<+10>-10\nAntarctica/Macquarie;AEST-10AEDT,M10.1.0,M4.1.0/3\nAntarctica/Mawson;<+05>-5\nAntarctica/McMurdo;NZST-12NZDT,M9.5.0,M4.1.0/3\nAntarctica/Palmer;<-03>3\nAntarctica/Rothera;<-03>3\nAntarctica/Syowa;<+03>-3\nAntarctica/Troll;<+00>0<+02>-2,M3.5.0/1,M10.5.0/3\nAntarctica/Vostok;<+06>-6\nArctic/Longyearbyen;CET-1CEST,M3.5.0,M10.5.0/3\nAsia/Aden;<+03>-3\nAsia/Almaty;<+06>-6\nAsia/Amman;EET-2EEST,M3.5.4/24,M10.5.5/1\nAsia/Anadyr;<+12>-12\nAsia/Aqtau;<+05>-5\nAsia/Aqtobe;<+05>-5\nAsia/Ashgabat;<+05>-5\nAsia/Atyrau;<+05>-5\nAsia/Baghdad;<+03>-3\nAsia/Bahrain;<+03>-3\nAsia/Baku;<+04>-4\nAsia/Bangkok;<+07>-7\nAsia/Barnaul;<+07>-7\nAsia/Beirut;EET-2EEST,M3.5.0/0,M10.5.0/0\nAsia/Bishkek;<+06>-6\nAsia/Brunei;<+08>-8\nAsia/Chita;<+09>-9\nAsia/Choibalsan;<+08>-8\nAsia/Colombo;<+0530>-5:30\nAsia/Damascus;EET-2EEST,M3.5.5/0,M10.5.5/0\nAsia/Dhaka;<+06>-6\nAsia/Dili;<+09>-9\nAsia/Dubai;<+04>-4\nAsia/Dushanbe;<+05>-5\nAsia/Famagusta;EET-2EEST,M3.5.0/3,M10.5.0/4\nAsia/Gaza;EET-2EEST,M3.4.4/48,M10.4.4/49\nAsia/Hebron;EET-2EEST,M3.4.4/48,M10.4.4/49\nAsia/Ho_Chi_Minh;<+07>-7\nAsia/Hong_Kong;HKT-8\nAsia/Hovd;<+07>-7\nAsia/Irkutsk;<+08>-8\nAsia/Jakarta;WIB-7\nAsia/Jayapura;WIT-9\nAsia/Jerusalem;IST-2IDT,M3.4.4/26,M10.5.0\nAsia/Kabul;<+0430>-4:30\nAsia/Kamchatka;<+12>-12\nAsia/Karachi;PKT-5\nAsia/Kathmandu;<+0545>-5:45\nAsia/Khandyga;<+09>-9\nAsia/Kolkata;IST-5:30\nAsia/Krasnoyarsk;<+07>-7\nAsia/Kuala_Lumpur;<+08>-8\nAsia/Kuching;<+08>-8\nAsia/Kuwait;<+03>-3\nAsia/Macau;CST-8\nAsia/Magadan;<+11>-11\nAsia/Makassar;WITA-8\nAsia/Manila;PST-8\nAsia/Muscat;<+04>-4\nAsia/Nicosia;EET-2EEST,M3.5.0/3,M10.5.0/4\nAsia/Novokuznetsk;<+07>-7\nAsia/Novosibirsk;<+07>-7\nAsia/Omsk;<+06>-6\nAsia/Oral;<+05>-5\nAsia/Phnom_Penh;<+07>-7\nAsia/Pontianak;WIB-7\nAsia/Pyongyang;KST-9\nAsia/Qatar;<+03>-3\nAsia/Qyzylorda;<+05>-5\nAsia/Riyadh;<+03>-3\nAsia/Sakhalin;<+11>-11\nAsia/Samarkand;<+05>-5\nAsia/Seoul;KST-9\nAsia/Shanghai;CST-8\nAsia/Singapore;<+08>-8\nAsia/Srednekolymsk;<+11>-11\nAsia/Taipei;CST-8\nAsia/Tashkent;<+05>-5\nAsia/Tbilisi;<+04>-4\nAsia/Tehran;<+0330>-3:30<+0430>,J79/24,J263/24\nAsia/Thimphu;<+06>-6\nAsia/Tokyo;JST-9\nAsia/Tomsk;<+07>-7\nAsia/Ulaanbaatar;<+08>-8\nAsia/Urumqi;<+06>-6\nAsia/Ust-Nera;<+10>-10\nAsia/Vientiane;<+07>-7\nAsia/Vladivostok;<+10>-10\nAsia/Yakutsk;<+09>-9\nAsia/Yangon;<+0630>-6:30\nAsia/Yekaterinburg;<+05>-5\nAsia/Yerevan;<+04>-4\nAtlantic/Azores;<-01>1<+00>,M3.5.0/0,M10.5.0/1\nAtlantic/Bermuda;AST4ADT,M3.2.0,M11.1.0\nAtlantic/Canary;WET0WEST,M3.5.0/1,M10.5.0\nAtlantic/Cape_Verde;<-01>1\nAtlantic/Faroe;WET0WEST,M3.5.0/1,M10.5.0\nAtlantic/Madeira;WET0WEST,M3.5.0/1,M10.5.0\nAtlantic/Reykjavik;GMT0\nAtlantic/South_Georgia;<-02>2\nAtlantic/Stanley;<-03>3\nAtlantic/St_Helena;GMT0\nAustralia/Adelaide;ACST-9:30ACDT,M10.1.0,M4.1.0/3\nAustralia/Brisbane;AEST-10\nAustralia/Broken_Hill;ACST-9:30ACDT,M10.1.0,M4.1.0/3\nAustralia/Currie;AEST-10AEDT,M10.1.0,M4.1.0/3\nAustralia/Darwin;ACST-9:30\nAustralia/Eucla;<+0845>-8:45\nAustralia/Hobart;AEST-10AEDT,M10.1.0,M4.1.0/3\nAustralia/Lindeman;AEST-10\nAustralia/Lord_Howe;<+1030>-10:30<+11>-11,M10.1.0,M4.1.0\nAustralia/Melbourne;AEST-10AEDT,M10.1.0,M4.1.0/3\nAustralia/Perth;AWST-8\nAustralia/Sydney;AEST-10AEDT,M10.1.0,M4.1.0/3\nEurope/Amsterdam;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Andorra;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Astrakhan;<+04>-4\nEurope/Athens;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Belgrade;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Berlin;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Bratislava;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Brussels;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Bucharest;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Budapest;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Busingen;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Chisinau;EET-2EEST,M3.5.0,M10.5.0/3\nEurope/Copenhagen;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Dublin;IST-1GMT0,M10.5.0,M3.5.0/1\nEurope/Gibraltar;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Guernsey;GMT0BST,M3.5.0/1,M10.5.0\nEurope/Helsinki;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Isle_of_Man;GMT0BST,M3.5.0/1,M10.5.0\nEurope/Istanbul;<+03>-3\nEurope/Jersey;GMT0BST,M3.5.0/1,M10.5.0\nEurope/Kaliningrad;EET-2\nEurope/Kiev;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Kirov;<+03>-3\nEurope/Lisbon;WET0WEST,M3.5.0/1,M10.5.0\nEurope/Ljubljana;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/London;GMT0BST,M3.5.0/1,M10.5.0\nEurope/Luxembourg;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Madrid;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Malta;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Mariehamn;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Minsk;<+03>-3\nEurope/Monaco;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Moscow;MSK-3\nEurope/Oslo;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Paris;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Podgorica;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Prague;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Riga;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Rome;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Samara;<+04>-4\nEurope/San_Marino;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Sarajevo;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Saratov;<+04>-4\nEurope/Simferopol;MSK-3\nEurope/Skopje;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Sofia;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Stockholm;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Tallinn;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Tirane;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Ulyanovsk;<+04>-4\nEurope/Uzhgorod;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Vaduz;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Vatican;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Vienna;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Vilnius;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Volgograd;<+03>-3\nEurope/Warsaw;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Zagreb;CET-1CEST,M3.5.0,M10.5.0/3\nEurope/Zaporozhye;EET-2EEST,M3.5.0/3,M10.5.0/4\nEurope/Zurich;CET-1CEST,M3.5.0,M10.5.0/3\nIndian/Antananarivo;EAT-3\nIndian/Chagos;<+06>-6\nIndian/Christmas;<+07>-7\nIndian/Cocos;<+0630>-6:30\nIndian/Comoro;EAT-3\nIndian/Kerguelen;<+05>-5\nIndian/Mahe;<+04>-4\nIndian/Maldives;<+05>-5\nIndian/Mauritius;<+04>-4\nIndian/Mayotte;EAT-3\nIndian/Reunion;<+04>-4\nPacific/Apia;<+13>-13<+14>,M9.5.0/3,M4.1.0/4\nPacific/Auckland;NZST-12NZDT,M9.5.0,M4.1.0/3\nPacific/Bougainville;<+11>-11\nPacific/Chatham;<+1245>-12:45<+1345>,M9.5.0/2:45,M4.1.0/3:45\nPacific/Chuuk;<+10>-10\nPacific/Easter;<-06>6<-05>,M9.1.6/22,M4.1.6/22\nPacific/Efate;<+11>-11\nPacific/Enderbury;<+13>-13\nPacific/Fakaofo;<+13>-13\nPacific/Fiji;<+12>-12<+13>,M11.2.0,M1.2.3/99\nPacific/Funafuti;<+12>-12\nPacific/Galapagos;<-06>6\nPacific/Gambier;<-09>9\nPacific/Guadalcanal;<+11>-11\nPacific/Guam;ChST-10\nPacific/Honolulu;HST10\nPacific/Kiritimati;<+14>-14\nPacific/Kosrae;<+11>-11\nPacific/Kwajalein;<+12>-12\nPacific/Majuro;<+12>-12\nPacific/Marquesas;<-0930>9:30\nPacific/Midway;SST11\nPacific/Nauru;<+12>-12\nPacific/Niue;<-11>11\nPacific/Norfolk;<+11>-11<+12>,M10.1.0,M4.1.0/3\nPacific/Noumea;<+11>-11\nPacific/Pago_Pago;SST11\nPacific/Palau;<+09>-9\nPacific/Pitcairn;<-08>8\nPacific/Pohnpei;<+11>-11\nPacific/Port_Moresby;<+10>-10\nPacific/Rarotonga;<-10>10\nPacific/Saipan;ChST-10\nPacific/Tahiti;<-10>10\nPacific/Tarawa;<+12>-12\nPacific/Tongatapu;<+13>-13\nPacific/Wake;<+12>-12\nPacific/Wallis;<+12>-12\nEtc/GMT;GMT0\nEtc/GMT-0;GMT0\nEtc/GMT-1;<+01>-1\nEtc/GMT-2;<+02>-2\nEtc/GMT-3;<+03>-3\nEtc/GMT-4;<+04>-4\nEtc/GMT-5;<+05>-5\nEtc/GMT-6;<+06>-6\nEtc/GMT-7;<+07>-7\nEtc/GMT-8;<+08>-8\nEtc/GMT-9;<+09>-9\nEtc/GMT-10;<+10>-10\nEtc/GMT-11;<+11>-11\nEtc/GMT-12;<+12>-12\nEtc/GMT-13;<+13>-13\nEtc/GMT-14;<+14>-14\nEtc/GMT0;GMT0\nEtc/GMT+0;GMT0\nEtc/GMT+1;<-01>1\nEtc/GMT+2;<-02>2\nEtc/GMT+3;<-03>3\nEtc/GMT+4;<-04>4\nEtc/GMT+5;<-05>5\nEtc/GMT+6;<-06>6\nEtc/GMT+7;<-07>7\nEtc/GMT+8;<-08>8\nEtc/GMT+9;<-09>9\nEtc/GMT+10;<-10>10\nEtc/GMT+11;<-11>11\nEtc/GMT+12;<-12>12\nEtc/UCT;UTC0\nEtc/UTC;UTC0\nEtc/Greenwich;GMT0\nEtc/Universal;UTC0\nEtc/Zulu;UTC0";
static const char* timezone_data = "America/New_York;EST5EDT,M3.2.0,M11.1.0\n";

// ====================================================================
// STATIC VARIABLES
// ====================================================================

static EXT_RAM_ATTR struct tm tm_local = {};
static EXT_RAM_ATTR struct tm tm_gmt = {};
static EXT_RAM_ATTR time_t now = 0;
static EXT_RAM_ATTR bool timezone_set = false;

static EXT_RAM_ATTR bool is_sntp_initialized = false;
static EXT_RAM_ATTR bool is_sntp_running = false;
static EXT_RAM_ATTR int sntp_sync_interval = 0;
static EXT_RAM_ATTR bool sntp_success = false;
static EXT_RAM_ATTR int64_t last_try_time = 0;
static EXT_RAM_ATTR char server_address[TIME_SERVER_STR_MAX_SIZE] = {};

static EXT_RAM_ATTR int64_t last_update_time = 0;

#if IDF_VERSION_MAJOR_MINOR <= 40
static EXT_RAM_ATTR esp_timer_handle_t sync_timer = NULL;
#endif

// ====================================================================
// STATIC PROTOTYPES
// ====================================================================

static bool is_leap_year(int year);
static int get_num_leap_years(int start_year, int end_year);
static int get_days_in_month(int month, int year);
static void time_updater();
static time_t time_get_timestamp(struct tm time_info, uint16_t base_year);
static void time_format_timeinfo(struct tm time_info, const char* format, char* ret_str, bool gmt);
static void time_time_sync_cb(struct timeval* tv);

#if IDF_VERSION_MAJOR_MINOR <= 40
static void IRAM_ATTR sync_timer_cb(void* arg);
#endif

// ====================================================================
// GLOBAL FUNCTIONS
// ====================================================================

// SNTP
// ------------------------------------------------------------------------------------------------------------------------

bool time_init(int sync_interval)
{
    if(is_sntp_initialized) return true;
    if(sync_interval < 15) sync_interval = 15;

    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    if(strlen(server_address) == 0) {
        LOGI("Setting sntp server address to default: %s", DEFAULT_SNTP_SERVER_ADDR);
        sntp_setservername(0, DEFAULT_SNTP_SERVER_ADDR);
    } else {
        LOGI("Setting sntp server address to: %s", server_address);
        sntp_setservername(0, server_address);
    }

    sntp_set_time_sync_notification_cb(time_time_sync_cb);
    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
#if IDF_VERSION_MAJOR_MINOR > 40
    sntp_set_sync_interval(sync_interval*1000);
#else
    const esp_timer_create_args_t sync_timer_args = {
        .callback = &sync_timer_cb,
        .name = "sync_timer_cb"
    };
    esp_timer_create(&sync_timer_args, &sync_timer);
    esp_timer_start_periodic(sync_timer, (uint64_t)sync_interval*1000000);
#endif

    sntp_init();

    sntp_sync_interval = sync_interval;
    is_sntp_initialized = true;
    is_sntp_running = true;
    return true;
}

bool time_get_gmt_offset(float* offset)
{
    // in fractional hours
    char offset_str[6] = {};
    size_t r = strftime(offset_str,6,"%z",&tm_local);

    if(r == 0)
    {
        return false;
    }

    int sign = offset_str[0] == '-' ? -1 : +1;

    int hours = 0;
    int min = 0;

    hours = 10*(int)('0' - offset_str[1]);
    hours += 1*(int)('0' - offset_str[2]);

    if(r > 3)
    {
        min = 10*(int)('0' - offset_str[3]);
        min += 1*(int)('0' - offset_str[4]);
    }

    *offset = sign * (hours + (min / 60.0f));

    return true;

}

bool time_set_timezone(char* timezone)
{
    int ret = setenv("TZ", timezone, 1);
    if(ret != 0) {
        LOGE("Failed to set TZ environment variable, errno: %d", errno);
        return false;
    }
    tzset();
    timezone_set = true;
    if(TIME_UPDATED_ONCE()) time_updater();
    return true;
}

bool time_set_timezone_id(char* timezone_id)
{
    if(STR_EMPTY(timezone_id)) return false;

    char* p = (char*)timezone_data;

    char tz_id[40] = {};
    char tz_str[40] = {};

    int idx = 0;
    char* dest = tz_id;

    // printf("searching for timezone\n");
    for(;;)
    {

        if(*p == '\n' || !*p)
        {
            if(strlen(tz_id) != 0 && strlen(tz_str) != 0)
            {

                if(strncmp(tz_id, timezone_id, strlen(timezone_id)) == 0 && strlen(tz_id) == strlen(timezone_id))
                {
                    // printf("found timezone!\n");
                    // printf("%s - %s - %s\n", timezone_id, tz_id, tz_str);
                    return time_set_timezone(tz_str);
                }
            }

            idx = 0;
            memset(tz_id, 0, 40);
            memset(tz_str, 0, 40);
            dest = tz_id;
            if(!*p) break;
            p++;
            continue;
        }

        if(*p == ';')
        {
            dest = tz_str;
            idx = 0;
            p++;
            continue;
        }

        dest[idx++] = *p;
        p++;
    }

    // printf("failed to find timezone\n");
    return false;
}

void time_set_sntp_server(char* server)
{

    if(strlen(server) == 0 || strlen(server) > TIME_SERVER_STR_MAX_SIZE) {
        LOGE("Invalid server address");
        return;
    }

    memset(server_address, 0, TIME_SERVER_STR_MAX_SIZE);
    strcpy(server_address, server);

    if(is_sntp_initialized) {
        if(is_sntp_running) {
            time_stop();
            LOGI("Setting sntp server address to: %s", server_address);
            sntp_setservername(0, server_address);
            LOGI("Syncing SNTP");
            sntp_init();
            is_sntp_running = true;
        } else {
            LOGI("Setting sntp server address to: %s", server_address);
            sntp_setservername(0, server_address);
        }
    }
}

void time_set_sync_interval(int sync_interval)
{
    if(sync_interval < 15) sync_interval = 15;
    sntp_sync_interval = sync_interval;
    if(is_sntp_initialized) {
        LOGI("Setting sync interval");
#if IDF_VERSION_MAJOR_MINOR > 40
        sntp_set_sync_interval(sntp_sync_interval*1000);
        if(is_sntp_running) {
            LOGI("Syncing SNTP");
            sntp_restart();
        }
#else
        if(is_sntp_running)
        {
            esp_timer_stop(sync_timer);
            esp_timer_start_periodic(sync_timer, (uint64_t)sntp_sync_interval*1000000);
        }
#endif
    }
}

int64_t time_update_get_last_try_time()
{
    return last_try_time;
}

// trigger connection to the sntp server
bool time_update_time(int timeout_ms)
{
    if(!is_sntp_initialized) return false;

    sntp_success = false;
    time(&now);

    LOGI("Syncing SNTP");

#if IDF_VERSION_MAJOR_MINOR > 40
    sntp_init();
    sntp_restart();
#else
    sntp_stop();
    sntp_init();
#endif

    last_try_time = esp_timer_get_time();

#if IDF_VERSION_MAJOR_MINOR <= 40
    // start the timer again
    if(!is_sntp_running)
        esp_timer_start_periodic(sync_timer, (uint64_t)sntp_sync_interval*1000000);
#endif
    is_sntp_running = true;

    if(timeout_ms <= 0) return false;

    for(;;) {
        if(sntp_success) return true;
        delay(10);
        timeout_ms -= 10;
        if(timeout_ms <= 0) return false;
    }
    return false;
}

int time_since_last_update()
{
    if(!TIME_UPDATED_ONCE()) return -1;
    return (esp_timer_get_time() - last_update_time) / 1000000;
}

int64_t time_last_update_time_us()
{
    return last_update_time;
}

bool time_success()
{
    return sntp_success;
}

void time_stop()
{
    if(!is_sntp_initialized) return;
    if(!is_sntp_running) return;
    LOGI("Stopping SNTP");
    sntp_stop();
#if IDF_VERSION_MAJOR_MINOR <= 40
    esp_timer_stop(sync_timer);
#endif
    is_sntp_running = false;
}

// now
// ------------------------------------------------------------------------------------------------------------------------

time_t time_now(uint16_t base_year) {
    if(!TIME_UPDATED_ONCE()) return 0;
    time_updater();
    return time_get_timestamp(tm_gmt, base_year);
}

time_t time_now_local(uint16_t base_year)
{
    if(!TIME_UPDATED_ONCE()) return 0;
    time_updater();
    return time_get_timestamp(tm_local, base_year);
}

void time_now_str(const char* format, char* ret_str)
{
    time_updater();
    time_format_timeinfo(tm_gmt, format, ret_str, true);
}

void time_now_str_local(const char* format, char* ret_str)
{
    time_updater();
    time_format_timeinfo(tm_local, format, ret_str, false);
}

void time_print_now()
{
    char str[TIME_STR_MAX_SIZE] = {};
    time_now_str((char*)TIME_FORMAT_DEFAULT, str);
    LOGI("Current time: %s", str);
}

void time_print_now_local()
{
    char str[TIME_STR_MAX_SIZE] = {};
    time_now_str_local((char*)TIME_FORMAT_DEFAULT, str);
    LOGI("Current time: %s", str);
}

struct tm time_now_tm()
{
    struct tm _empty = {};
    if(!TIME_UPDATED_ONCE()) return _empty;
    time_updater();
    return tm_gmt;
}

struct tm time_now_tm_local()
{
    struct tm _empty = {};
    if(!TIME_UPDATED_ONCE()) return _empty;
    time_updater();
    return tm_local;
}


// time strings
// ------------------------------------------------------------------------------------------------------------------------

// Note: timestamp must be utc
void time_str_from_timestamp(long long timestamp, uint16_t base_year, const char* format, char* ret_str, bool gmt_format)
{
    struct tm time_info = time_tm_from_timestamp(timestamp, base_year);
    // if(!gmt_format) {
    //     // takes the utc tm stuct and makes it local
    //     time_t ts = time_get_timestamp(time_info, 1970);
    //     localtime_r(&ts, &time_info);
    //     mktime(&time_info);
    // }
    time_format_timeinfo(time_info, format, ret_str, gmt_format);
}


void time_str_from_tm(struct tm time_info, const char* format, char* ret_str, bool gmt_format)
{
    time_format_timeinfo(time_info, format, ret_str, gmt_format);
}


// tm and timestamp conversions
// ------------------------------------------------------------------------------------------------------------------------


// timestamp to tm struct
struct tm time_tm_from_timestamp(long long timestamp, uint16_t base_year)
{
    long long _timestamp = (long long)timestamp;

    int year = base_year;
    int prior_subtracted_amount = 0;

    for(;;) {

        if(_timestamp < 0) {
            _timestamp += prior_subtracted_amount;
            year--;
            break;
        } else if(_timestamp == 0)
            break;

        if(is_leap_year(year++)) {
            _timestamp -= SECONDS_IN_YEAR_LEAP;
            prior_subtracted_amount = SECONDS_IN_YEAR_LEAP;
        } else {
            _timestamp -= SECONDS_IN_YEAR;
            prior_subtracted_amount = SECONDS_IN_YEAR;
        }
    }

    // days
    int days = 0;
    for(;;) {

        if(_timestamp < 0) {
            _timestamp += SECONDS_IN_DAY;
            days--;
            break;
        } else if(_timestamp == 0)
            break;

        _timestamp -= SECONDS_IN_DAY;
        days++;
    }

    // hours
    int hours = 0;
    for(;;) {

        if(_timestamp < 0) {
            _timestamp += SECONDS_IN_HOUR;
            hours--;
            break;
        } else if(_timestamp == 0)
            break;

        _timestamp -= SECONDS_IN_HOUR;
        hours++;
    }
    
    // minutes
    int minutes = 0;
    for(;;) {

        if(_timestamp < 0) {
            _timestamp += SECONDS_IN_MINUTE;
            minutes--;
            break;
        } else if(_timestamp == 0)
            break;

        _timestamp -= SECONDS_IN_MINUTE;
        minutes++;
    }

    int day_of_month = 0;
    int month = 1;
    int rem_days = days+1; //remaining days (days is 0 based)
    for(int m = 1; m < 13; ++m) {
        int days_in_month = get_days_in_month(m, year);
        rem_days -= days_in_month;
        if(rem_days <= 0) {
            day_of_month = rem_days + days_in_month;
            month = m;
            break;
        } else {
            continue;
        }
    }

    int seconds = _timestamp;

    LOGD("Years: %d, Month: %d, Day_of_month: %d, Days: %d, Hours: %d, Minutes: %d, Seconds: %d",year,month,day_of_month,days,hours,minutes,seconds);

    struct tm _timeinfo = {};

    _timeinfo.tm_sec    = seconds;
    _timeinfo.tm_min    = minutes;
    _timeinfo.tm_hour   = hours;
    _timeinfo.tm_mday   = day_of_month;
    _timeinfo.tm_mon    = month - 1;
    _timeinfo.tm_year   = year - 1900;
    _timeinfo.tm_wday   = 0;
    _timeinfo.tm_yday   = days;

    return _timeinfo;
}

// tm to timestamp
time_t time_timestamp_from_tm(struct tm time_info, uint16_t base_year)
{
    return time_get_timestamp(time_info, base_year);
}



// ====================================================================
// STATIC FUNCTIONS
// ====================================================================

#if IDF_VERSION_MAJOR_MINOR <= 40
static void IRAM_ATTR sync_timer_cb(void* arg)
{
    time_update_time(0);
}
#endif


static bool is_leap_year(int year)
{
    if(year % 4 > 0) return false;
    else if(year % 100 > 0) return true;
    else if(year % 400 > 0) return false;
    else return true;
}

// doesn't include end year in count
static int get_num_leap_years(int start_year, int end_year)
{
    int num_leap_years = 0;
    for(int i = start_year; i < end_year; ++i) {
        if(is_leap_year(i)) num_leap_years++;
    }
    return num_leap_years;
}

static int get_days_in_month(int month, int year)
{
    switch (month) {
        case 1:  return 31;
        case 2:  {if(is_leap_year(year)) return 29; else return 28; }
        case 3:  return 31;
        case 4:  return 30;
        case 5:  return 31;
        case 6:  return 30;
        case 7:  return 31;
        case 8:  return 31;
        case 9:  return 30;
        case 10: return 31;
        case 11: return 30;
        case 12: return 31;
    }
    return 0;
}

static void time_updater()
{
    time(&now);
    localtime_r(&now, &tm_local);
    mktime(&tm_local); // should fill out is_dst field
    struct tm* gmtm = gmtime(&now);
    memcpy(&tm_gmt, gmtm, sizeof(tm_gmt));
}


static time_t time_get_timestamp(struct tm time_info, uint16_t base_year)
{

    // get # seconds since base_year-01-01 00:00:00
    int current_year = time_info.tm_year + 1900;
    int num_leap_years = get_num_leap_years(base_year, current_year);

    uint64_t years_since_year = current_year - base_year;
    uint64_t seconds_years = (SECONDS_IN_YEAR*(years_since_year - num_leap_years)) + (SECONDS_IN_YEAR_LEAP*num_leap_years);

    uint64_t seconds_days    = time_info.tm_yday * SECONDS_IN_DAY;
    uint64_t seconds_hours   = time_info.tm_hour * SECONDS_IN_HOUR;
    uint64_t seconds_minutes = time_info.tm_min  * SECONDS_IN_MINUTE;
    uint64_t seconds_seconds = time_info.tm_sec;

    return (seconds_years + seconds_days + seconds_hours + seconds_minutes + seconds_seconds);
}

static void time_format_timeinfo(struct tm time_info, const char* format, char* ret_str, bool gmt)
{
    //if gmt == false, then %z shouldn't be used, rather +0000 should be part of the format
    if(strcmp(format, TIME_FORMAT_IOT) == 0) {
        strftime(ret_str,TIME_STR_MAX_SIZE,"%Y-%m-%dT%H:%M:%S%z",&time_info); //TODO
        int len = strlen(ret_str);
        // insert colon into timestring
        ret_str[len] = ret_str[len-1];
        ret_str[len-1] = ret_str[len-2];
        ret_str[len-2] = ':';
        if(gmt)
        {
            ret_str[len] = '0';
            ret_str[len-1] = '0';
            ret_str[len-3] = '0';
            ret_str[len-4] = '0';
        }
    } else {
        strftime(ret_str, TIME_STR_MAX_SIZE, format, &time_info);
    }
}


// callback
static void time_time_sync_cb(struct timeval* tv)
{
    time_updater();
    sntp_success = true;
    last_update_time = esp_timer_get_time();
    last_try_time = esp_timer_get_time();
    char str[TIME_STR_MAX_SIZE] = {};
    time_format_timeinfo(tm_gmt, TIME_FORMAT_DEFAULT, str, true);
    if(timezone_set)
    {
        char str_local[TIME_STR_MAX_SIZE] = {};
        time_format_timeinfo(tm_local, TIME_FORMAT_DEFAULT, str_local, false);
        LOGI("SNTP Success: %s (Local: %s)", str, str_local);
    }
    else
    {
        LOGI("SNTP Success: %s", str);
    }
}
