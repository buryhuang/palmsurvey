#ifndef __PROJECTS_CONFIG_H__
#define __PROJECTS_CONFIG_H__


/* support for a colored version */
/* undefine CONFIG_COLOR for b/w version */
//#undef CONFIG_COLOR
#define CONFIG_COLOR


/* supprot for system os version below 3.5 */
/* undefine the CONFIG_OS_BELOW_35 for a version that requires os ver >= 3.5 */
//#undef CONFIG_OS_BELOW_35
#define CONFIG_OS_BELOW_35


/* support for the extended about dialog box */
/* undefine the CONFIG_EXT_ABOUT for a version with a simple about dialog */
//#undef CONFIG_EXT_ABOUT
#define CONFIG_EXT_ABOUT


/* supprot for helpstrings */
/* undefine the CONFIG_HLP_STRS for a smaller version */
//#undef CONFIG_HLP_STRS
#define CONFIG_HLP_STRS


/* suppor for additional features */
/* undef the CONFIG_ADDITIONAL for a smaller version without */
//#undef CONFIG_ADDITIONAL
#define CONFIG_ADDITIONAL


/* support for the alltodos dialog on the mainform
 * undef the CONFIG_ALLDOTOS_DLG for a smaller version */
//#undef CONFIG_ALLTODOS_DLG
#define CONFIG_ALLTODOS_DLG


/* support for the default categories dialog */
/* if you don't want to define default categories and want to have a smaller PRC undefine this */
//#undef CONFIG_DEF_CATEGORY
#define CONFIG_DEF_CATEGORY


/* support for high-resolution on a sony */
/* undefine the CONFIG_SONY to build a version without this support */
#undef CONFIG_SONY
//#define CONFIG_SONY


/* support for high-resolution on a HandEra330 */
/* undifine the CONFIG_HANDERA to build a version without this support */
#undef CONFIG_HANDERA
//#define CONFIG_HANDERA


/* support for jog dial on hadera and sony */
#undef CONFIG_JOGDIAL
//#define CONFIG_JOGDIAL

#endif /* __PROJECTS_CONFIG_H__ */
