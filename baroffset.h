/*
 * baroffset.h
 *
 *  Created on: 2017-05-12
 *      Author: LyReonn
 */

#ifndef BAROFFSET_H_
#define BAROFFSET_H_

#define BAROFFSET_SUCCESS			 0
#define BAROFFSET_FILTER_FAILED		-1
#define BAROFFSET_TOO_MANY_CHN		-2
#define BAROFFSET_UPBEAT			-3

int BarOffset(SMFILE *smf);

#endif /* BAROFFSET_H_ */
