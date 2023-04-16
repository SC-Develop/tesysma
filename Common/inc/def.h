/*
 * @file def.h  - https://github.com/SC-Develop/tesysma
 *
 *
 * @author: Ing. Salvatore Cerami
 *
 * @copyright (c) 2023 (MIT) Ing. Salvatore Cerami - dev.salvatore.cerami@gmail.com - https://github.com/sc-develop/
 */

#ifndef INC_DEF_H_
#define INC_DEF_H_

/**
 * @def _BUILD_DAY_ get build day from __DATE__  predefined macro
 * @brief
 *
 */
#define _BUILD_DAY_ (\
    __DATE__[4] == '?' ? 1 \
    : ((__DATE__[4] == ' ' ? 0 : \
    ((__DATE__[4] - '0') * 10)) + __DATE__[5] - '0'))

/**
 * @def _BUILD_MONTH_
 * @brief get build month from __DATE__ predefined macro
 *
 */
#define _BUILD_MONTH_ (\
  __DATE__ [2] == 'n' ? (__DATE__ [1] == 'a' ? 1 : 6) \
: __DATE__ [2] == 'b' ? 2 \
: __DATE__ [2] == 'r' ? (__DATE__ [0] == 'M' ? 3 : 4) \
: __DATE__ [2] == 'y' ? 5 \
: __DATE__ [2] == 'l' ? 7 \
: __DATE__ [2] == 'g' ? 8 \
: __DATE__ [2] == 'p' ? 9 \
: __DATE__ [2] == 't' ? 10 \
: __DATE__ [2] == 'v' ? 11 \
: 12)

/**
 * @def _BUILD_YEAR_
 * @brief get build year from __DATE__ predefined macro
 *
 */
#define _BUILD_YEAR_ ( \
((__DATE__[7] - '0') * 1000) + \
((__DATE__[8] - '0') * 100) + \
((__DATE__[9] - '0') * 10) + \
(__DATE__[10] - '0') )

//----- Versione del FW  ---------------------------------- //

#define	AC_VERSION_DAY		_BUILD_DAY_
#define	AC_VERSION_MONTH	_BUILD_MONTH_
#define	AC_VERSION_YEAR		(_BUILD_YEAR_ - 2000)

#endif /* INC_DEF_H_ */
