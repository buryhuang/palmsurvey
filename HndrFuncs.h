#ifndef __HNDR_FUNCS_H__
#define __HNDR_FUNCS_H__

#ifdef CONFIG_HANDERA

extern void HndrMoveObject( FormType * frmP, UInt16 objI, Int16 x_diff, Int16 y_diff, Boolean draw );
extern void HndrCenterForm( FormType * frmP, Boolean draw );

#endif /* CONFIG_HANDERA */

#endif /* __HNDR_FUNCS_H__ */
