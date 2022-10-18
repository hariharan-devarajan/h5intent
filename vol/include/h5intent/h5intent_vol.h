/*-------------------------------------------------------------------------
*
* Created: h5intent_vol.h
* Oct 2022
* Hariharan Devarajan <hariharandev1@llnl.gov>
*
* Purpose:Defines the H5intent VOL Plugin
*
*-------------------------------------------------------------------------
*/

#ifndef H5INTENT_H5INTENT_VOL_H
#define H5INTENT_H5INTENT_VOL_H
#include "H5public.h"
#include "H5Rpublic.h"

#ifdef __cplusplus
extern "C" {
#endif
#define H5INTENT 555
/**
 * This method implements the setting of vol driver inside application's fapl.
 *
 * @param fapl_id
 * @return vol_id
 */
H5_DLL hid_t H5Pset_fapl_h5intent_vol(hid_t fapl_id);

#ifdef __cplusplus
}
#endif
#endif //H5INTENT_H5INTENT_VOL_H